#include "tab_addons.h"
#include "../../addon/addon_manager.h"
#include "../widgets/addon_card.h"
#include "../widgets/search_bar.h"
#include "../widgets/toast.h"
#include "imgui.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

namespace lsproxy {

static char s_searchBuffer[256] = {};
static int s_selectedIndex = -1;

// Config editor state
static bool s_showConfigEditor = false;
static std::string s_configContent;
static std::wstring s_configPath;
static std::vector<char> s_configBuffer;
static const size_t CONFIG_BUFFER_SIZE = 1024 * 1024;

static void OpenConfigEditor(const std::wstring& path) {
    s_configPath = path;
    std::ifstream file(path);
    if (file.is_open()) {
        std::stringstream buf;
        buf << file.rdbuf();
        s_configContent = buf.str();
        s_configBuffer.resize(CONFIG_BUFFER_SIZE);
        strncpy_s(s_configBuffer.data(), s_configBuffer.size(), s_configContent.c_str(), _TRUNCATE);
        s_showConfigEditor = true;
    }
}

static void SaveConfigEditor() {
    if (!s_configPath.empty()) {
        std::ofstream file(s_configPath);
        if (file.is_open()) {
            file << s_configBuffer.data();
            s_showConfigEditor = false;
            widgets::ToastShow("Config saved", widgets::ToastType::Success);
        }
    }
}

static bool MatchesFilter(const AddonInfo& addon, const char* filter) {
    if (!filter || filter[0] == '\0') return true;
    std::string f(filter);
    std::transform(f.begin(), f.end(), f.begin(), ::tolower);

    std::string name = addon.GetDisplayName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name.find(f) != std::string::npos) return true;

    std::string author = addon.GetDisplayAuthor();
    std::transform(author.begin(), author.end(), author.begin(), ::tolower);
    if (author.find(f) != std::string::npos) return true;

    for (const auto& tag : addon.manifest.tags) {
        std::string t = tag;
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);
        if (t.find(f) != std::string::npos) return true;
    }

    return false;
}

void RenderTabAddons(AddonManager* manager) {
    if (!manager) return;

    auto& addons = manager->GetAddons();

    // Search bar
    widgets::SearchBar("##addon_search", s_searchBuffer, sizeof(s_searchBuffer), "Search addons...");
    ImGui::Dummy(ImVec2(0, 5));

    // Two-column layout: addon list (left) + detail panel (right)
    float detailWidth = 300.0f;
    float availWidth = ImGui::GetContentRegionAvail().x;
    bool showDetail = (availWidth > 650.0f && s_selectedIndex >= 0 && s_selectedIndex < (int)addons.size());
    float listWidth = showDetail ? (availWidth - detailWidth - 10) : availWidth;

    // Left: Addon list
    ImGui::BeginChild("AddonList", ImVec2(listWidth, -1), false);

    if (addons.empty()) {
        ImGui::Dummy(ImVec2(0, 30));
        float textWidth = ImGui::CalcTextSize("No addons found").x;
        ImGui::SetCursorPosX((listWidth - textWidth) * 0.5f);
        ImGui::TextDisabled("No addons found");
        ImGui::Dummy(ImVec2(0, 5));
        float hintWidth = ImGui::CalcTextSize("Place addon folders in the 'addons' directory").x;
        ImGui::SetCursorPosX((listWidth - hintWidth) * 0.5f);
        ImGui::TextDisabled("Place addon folders in the 'addons' directory");
    }

    for (int i = 0; i < (int)addons.size(); i++) {
        if (!MatchesFilter(addons[i], s_searchBuffer)) continue;

        bool wasSelected = (s_selectedIndex == i);
        if (widgets::AddonCard(addons[i], i, wasSelected)) {
            s_selectedIndex = i;

            // Check if enable state changed
            bool currentEnabled = addons[i].enabled;
            // The card updates addon.enabled directly when toggle is clicked
            // We need to detect the change and call ToggleAddon
            // For now, we re-check after card render
        }
        ImGui::Dummy(ImVec2(0, 2));
    }

    ImGui::EndChild();

    // Right: Detail panel
    if (showDetail) {
        ImGui::SameLine();
        ImGui::BeginChild("AddonDetail", ImVec2(detailWidth, -1), true);

        auto& addon = addons[s_selectedIndex];

        // Icon + Name header
        if (addon.iconTexture) {
            float iconSize = 48.0f;
            ImGui::Image((ImTextureID)addon.iconTexture, ImVec2(iconSize, iconSize));
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (iconSize - ImGui::GetTextLineHeight()) * 0.5f);
        }
        ImGui::Text("%s", addon.GetDisplayName().c_str());
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));

        // Version & Author
        ImGui::TextDisabled("Version:");
        ImGui::SameLine();
        ImGui::Text("%s", addon.GetDisplayVersion().c_str());

        ImGui::TextDisabled("Author:");
        ImGui::SameLine();
        ImGui::Text("%s", addon.GetDisplayAuthor().c_str());

        // Status
        ImGui::TextDisabled("Status:");
        ImGui::SameLine();
        if (addon.faulted) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.2f, 0.2f, 1));
            ImGui::Text("Faulted");
            ImGui::PopStyleColor();
        } else if (addon.IsLoaded()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1));
            ImGui::Text("Loaded");
            ImGui::PopStyleColor();
        } else {
            ImGui::TextDisabled("Unloaded");
        }

        // Security
        ImGui::TextDisabled("Security:");
        ImGui::SameLine();
        switch (addon.security) {
            case SecurityVerdict::Trusted:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1));
                ImGui::Text("Trusted");
                ImGui::PopStyleColor();
                break;
            case SecurityVerdict::Tampered:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.2f, 0.2f, 1));
                ImGui::Text("Tampered!");
                ImGui::PopStyleColor();
                break;
            default:
                ImGui::TextDisabled("Unknown");
                break;
        }

        // Description
        if (!addon.manifest.description.empty()) {
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextWrapped("%s", addon.manifest.description.c_str());
        }

        // Tags
        if (!addon.manifest.tags.empty()) {
            ImGui::Dummy(ImVec2(0, 5));
            for (const auto& tag : addon.manifest.tags) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.25f, 0.25f, 1));
                ImGui::SmallButton(tag.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine();
            }
            ImGui::NewLine();
        }

        // Error
        if (!addon.errorMessage.empty()) {
            ImGui::Dummy(ImVec2(0, 5));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.3f, 0.3f, 1));
            ImGui::TextWrapped("Error: %s", addon.errorMessage.c_str());
            ImGui::PopStyleColor();
        }

        // Dependencies
        if (!addon.manifest.dependencies.empty()) {
            ImGui::Dummy(ImVec2(0, 5));
            ImGui::TextDisabled("Dependencies:");
            for (const auto& dep : addon.manifest.dependencies) {
                ImGui::BulletText("%s", dep.c_str());
            }
        }

        // Action buttons
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 5));

        if (addon.capabilities & LSPROXY_CAP_HAS_SETTINGS) {
            if (ImGui::Button("Settings", ImVec2(-1, 30))) {
                addon.showSettings = !addon.showSettings;
            }
            ImGui::Dummy(ImVec2(0, 3));
        }

        if (!addon.configPath.empty()) {
            if (ImGui::Button("Edit Config", ImVec2(-1, 30))) {
                OpenConfigEditor(addon.configPath);
            }
            ImGui::Dummy(ImVec2(0, 3));
        }

        // Restart notice
        bool needsRestart = addon.RequiresRestart();
        if (needsRestart) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1));
            ImGui::TextWrapped("This addon requires restarting Lossless Scaling to apply changes.");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 3));
        }

        // Enable/Disable button
        if (addon.enabled) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.2f, 0.2f, 1));
            if (ImGui::Button("Disable", ImVec2(-1, 30))) {
                manager->ToggleAddon(s_selectedIndex, false);
                if (needsRestart) {
                    widgets::ToastShow("Restart Lossless Scaling to disable this addon", widgets::ToastType::Warning, 5.0f);
                } else {
                    widgets::ToastShow("Addon disabled", widgets::ToastType::Info);
                }
            }
            ImGui::PopStyleColor(2);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.15f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.2f, 1));
            if (ImGui::Button("Enable", ImVec2(-1, 30))) {
                manager->ToggleAddon(s_selectedIndex, true);
                if (needsRestart) {
                    widgets::ToastShow("Restart Lossless Scaling to enable this addon", widgets::ToastType::Warning, 5.0f);
                } else {
                    widgets::ToastShow("Addon enabled", widgets::ToastType::Success);
                }
            }
            ImGui::PopStyleColor(2);
        }

        ImGui::EndChild();
    }

    // Config Editor Window
    if (s_showConfigEditor) {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Config Editor", &s_showConfigEditor)) {
            if (ImGui::Button("Save")) {
                SaveConfigEditor();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                s_showConfigEditor = false;
            }
            ImGui::Separator();
            ImGui::InputTextMultiline("##configedit", s_configBuffer.data(), s_configBuffer.size(),
                                      ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_AllowTabInput);
        }
        ImGui::End();
    }

    // Addon Settings Windows
    for (int i = 0; i < (int)addons.size(); i++) {
        if (addons[i].showSettings) {
            std::string windowName = "Settings: " + addons[i].GetDisplayName();
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(windowName.c_str(), &addons[i].showSettings)) {
                manager->RenderAddonSettings(i);
            }
            ImGui::End();
        }
    }
}

} // namespace lsproxy
