#include "gui_style.h"
#include "imgui.h"

namespace lsproxy {

void SetupModernStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding
    style.WindowRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.ChildRounding = 8.0f;

    // Spacing
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.ScrollbarSize = 12.0f;

    // Border
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    ImVec4* colors = style.Colors;

    // Base palette
    ImVec4 bgDark      = ImVec4(0.08f, 0.07f, 0.07f, 1.00f);
    ImVec4 bgMedium    = ImVec4(0.12f, 0.10f, 0.10f, 1.00f);
    ImVec4 bgLight     = ImVec4(0.16f, 0.14f, 0.14f, 1.00f);
    ImVec4 bgLighter   = ImVec4(0.22f, 0.18f, 0.18f, 1.00f);
    ImVec4 accentRed   = ImVec4(0.90f, 0.18f, 0.18f, 1.00f);
    ImVec4 accentHover = ImVec4(1.00f, 0.28f, 0.28f, 1.00f);
    ImVec4 textWhite   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    ImVec4 textDim     = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    ImVec4 borderCol   = ImVec4(0.25f, 0.20f, 0.20f, 0.40f);

    // Window
    colors[ImGuiCol_WindowBg]        = bgDark;
    colors[ImGuiCol_ChildBg]         = bgLight;
    colors[ImGuiCol_PopupBg]         = bgMedium;
    colors[ImGuiCol_Border]          = borderCol;
    colors[ImGuiCol_BorderShadow]    = ImVec4(0, 0, 0, 0);

    // Title
    colors[ImGuiCol_TitleBg]         = bgDark;
    colors[ImGuiCol_TitleBgActive]   = bgDark;
    colors[ImGuiCol_TitleBgCollapsed]= bgDark;

    // Menu / Header
    colors[ImGuiCol_MenuBarBg]       = bgMedium;
    colors[ImGuiCol_Header]          = bgLighter;
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.28f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.32f, 0.26f, 0.26f, 1.0f);

    // Button
    colors[ImGuiCol_Button]          = bgLighter;
    colors[ImGuiCol_ButtonHovered]   = ImVec4(0.30f, 0.24f, 0.24f, 1.0f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(0.35f, 0.28f, 0.28f, 1.0f);

    // Frame
    colors[ImGuiCol_FrameBg]         = ImVec4(0.14f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.20f, 0.16f, 0.16f, 1.0f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.24f, 0.20f, 0.20f, 1.0f);

    // Tabs
    colors[ImGuiCol_Tab]                = bgMedium;
    colors[ImGuiCol_TabHovered]         = accentRed;
    colors[ImGuiCol_TabSelected]        = accentRed;
    colors[ImGuiCol_TabSelectedOverline]= accentRed;
    colors[ImGuiCol_TabDimmed]          = bgMedium;
    colors[ImGuiCol_TabDimmedSelected]  = bgLighter;

    // Accent elements
    colors[ImGuiCol_CheckMark]       = accentRed;
    colors[ImGuiCol_SliderGrab]      = accentRed;
    colors[ImGuiCol_SliderGrabActive]= accentHover;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]     = bgDark;
    colors[ImGuiCol_ScrollbarGrab]   = bgLighter;
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.28f, 0.28f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive]  = accentRed;

    // Separator
    colors[ImGuiCol_Separator]       = borderCol;
    colors[ImGuiCol_SeparatorHovered]= accentRed;
    colors[ImGuiCol_SeparatorActive] = accentHover;

    // Resize
    colors[ImGuiCol_ResizeGrip]        = ImVec4(0.30f, 0.20f, 0.20f, 0.4f);
    colors[ImGuiCol_ResizeGripHovered] = accentRed;
    colors[ImGuiCol_ResizeGripActive]  = accentHover;

    // Text
    colors[ImGuiCol_Text]            = textWhite;
    colors[ImGuiCol_TextDisabled]    = textDim;
}

} // namespace lsproxy
