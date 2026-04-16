#include "tab_settings.h"
#include "../../config/config_manager.h"
#include "../widgets/toast.h"
#include "imgui.h"

namespace lsproxy {

void RenderTabSettings() {
    auto& config = ConfigManager::Instance();
    auto& global = config.Global();

    ImGui::Dummy(ImVec2(0, 5));
    ImGui::Text("General");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 5));

    // Security level
    static int securityLevel = 0; // 0=Allow All, 1=Warn, 2=Block Untrusted
    try {
        if (global.contains("security_level"))
            securityLevel = global["security_level"].get<int>();
    } catch (...) {}

    ImGui::TextDisabled("Security Level");
    const char* securityItems[] = { "Allow All", "Warn on Untrusted", "Block Untrusted" };
    if (ImGui::Combo("##security", &securityLevel, securityItems, 3)) {
        global["security_level"] = securityLevel;
    }

    ImGui::Dummy(ImVec2(0, 10));

    // Log level
    static int logLevel = 2; // Info
    try {
        if (global.contains("log_level"))
            logLevel = global["log_level"].get<int>();
    } catch (...) {}

    ImGui::TextDisabled("Log Level");
    const char* logItems[] = { "Trace", "Debug", "Info", "Warn", "Error" };
    if (ImGui::Combo("##loglevel", &logLevel, logItems, 5)) {
        global["log_level"] = logLevel;
    }

    ImGui::Dummy(ImVec2(0, 10));

    // Auto-load
    static bool autoLoad = true;
    try {
        if (global.contains("auto_load"))
            autoLoad = global["auto_load"].get<bool>();
    } catch (...) {}

    if (ImGui::Checkbox("Auto-load addons on startup", &autoLoad)) {
        global["auto_load"] = autoLoad;
    }

    ImGui::Dummy(ImVec2(0, 20));

    // Save button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.15f, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.2f, 1));
    if (ImGui::Button("Save Settings", ImVec2(200, 35))) {
        config.Save();
        widgets::ToastShow("Settings saved", widgets::ToastType::Success);
    }
    ImGui::PopStyleColor(2);
}

} // namespace lsproxy
