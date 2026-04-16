#include "addon_card.h"
#include "toggle_switch.h"
#include <cstdio>

namespace lsproxy {
namespace widgets {

// Status dot color
static ImU32 GetStatusColor(const AddonInfo& addon) {
    if (addon.faulted) return IM_COL32(220, 50, 50, 255);   // Red
    if (addon.security == SecurityVerdict::Tampered) return IM_COL32(220, 50, 50, 255);
    if (addon.security == SecurityVerdict::Unknown && addon.enabled) return IM_COL32(220, 180, 50, 255); // Yellow
    if (addon.IsLoaded()) return IM_COL32(80, 200, 80, 255); // Green
    return IM_COL32(120, 120, 120, 255);                      // Grey
}

// First-letter icon with colored background (fallback when no icon texture)
static void DrawLetterIcon(ImDrawList* drawList, ImVec2 pos, float size, const std::string& name) {
    char letter = (!name.empty()) ? (char)toupper(name[0]) : '?';
    float hue = (float)(letter - 'A') / 26.0f;
    ImVec4 color = ImVec4(0, 0, 0, 1);
    ImGui::ColorConvertHSVtoRGB(hue, 0.5f, 0.7f, color.x, color.y, color.z);

    drawList->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size),
                           ImGui::GetColorU32(color), 6.0f);

    char letterStr[2] = {letter, 0};
    ImVec2 textSize = ImGui::CalcTextSize(letterStr);
    drawList->AddText(
        ImVec2(pos.x + (size - textSize.x) * 0.5f, pos.y + (size - textSize.y) * 0.5f),
        IM_COL32(255, 255, 255, 255), letterStr);
}

// Draw addon icon (texture or letter fallback)
static void DrawIcon(ImDrawList* drawList, ImVec2 pos, float size, const AddonInfo& addon) {
    if (addon.iconTexture) {
        // Draw the texture with rounded corners via clip rect + image
        ImVec2 p1(pos.x + size, pos.y + size);
        drawList->AddImageRounded(
            (ImTextureID)addon.iconTexture,
            pos, p1,
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 255),
            6.0f);
    } else {
        DrawLetterIcon(drawList, pos, size, addon.GetDisplayName());
    }
}

bool AddonCard(AddonInfo& addon, int index, bool isSelected) {
    bool clicked = false;
    ImGui::PushID(index);

    ImVec4 childBg = isSelected
        ? ImVec4(0.25f, 0.20f, 0.20f, 1.0f)
        : ImVec4(0.16f, 0.14f, 0.14f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, childBg);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);

    float cardHeight = 70.0f;
    ImGui::BeginChild("Card", ImVec2(-1, cardHeight), true);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // Icon (texture or letter fallback)
    float iconSize = 40.0f;
    DrawIcon(drawList, cursorPos, iconSize, addon);

    // Status dot
    ImVec2 dotPos(cursorPos.x + iconSize - 4, cursorPos.y + iconSize - 4);
    drawList->AddCircleFilled(dotPos, 5.0f, GetStatusColor(addon));
    drawList->AddCircle(dotPos, 5.0f, IM_COL32(0, 0, 0, 150), 12, 1.0f);

    // Text area
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + iconSize + 10);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);

    float startY = ImGui::GetCursorPosY();
    ImGui::Text("%s", addon.GetDisplayName().c_str());
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + iconSize + 10);
    ImGui::TextDisabled("v%s  |  %s", addon.GetDisplayVersion().c_str(), addon.GetDisplayAuthor().c_str());

    // Toggle switch on the right
    float toggleWidth = ImGui::GetFrameHeight() * 1.8f * 0.8f;
    ImGui::SameLine(ImGui::GetWindowWidth() - toggleWidth - 15);
    ImGui::SetCursorPosY(startY + 5);

    char toggleId[32];
    snprintf(toggleId, sizeof(toggleId), "##toggle_%d", index);
    bool enabled = addon.enabled;
    if (ToggleSwitch(toggleId, &enabled)) {
        addon.enabled = enabled;
        clicked = true;
    }

    // Detect card click (not on toggle)
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        clicked = true;
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopID();

    return clicked;
}

} // namespace widgets
} // namespace lsproxy
