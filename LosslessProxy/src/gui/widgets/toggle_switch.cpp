#include "toggle_switch.h"
#include "imgui_internal.h"
#include <unordered_map>

namespace lsproxy {
namespace widgets {

static std::unordered_map<ImGuiID, float> s_animState;

bool ToggleSwitch(const char* id, bool* value) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID imId = window->GetID(id);

    const float height = ImGui::GetFrameHeight() * 0.8f;
    const float width = height * 1.8f;
    const float radius = height * 0.5f;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size(width, height);
    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, imId)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, imId, &hovered, &held);
    if (pressed) {
        *value = !(*value);
    }

    // Animation
    float& anim = s_animState[imId];
    float target = *value ? 1.0f : 0.0f;
    float speed = g.IO.DeltaTime * 8.0f;
    if (anim < target) anim = (anim + speed > target) ? target : anim + speed;
    else if (anim > target) anim = (anim - speed < target) ? target : anim - speed;

    // Colors
    ImU32 bgColor;
    if (*value) {
        float r = 0.85f + (hovered ? 0.1f : 0.0f);
        bgColor = ImGui::GetColorU32(ImVec4(r, 0.15f, 0.15f, 1.0f));
    } else {
        float v = 0.25f + (hovered ? 0.05f : 0.0f);
        bgColor = ImGui::GetColorU32(ImVec4(v, v, v, 1.0f));
    }

    ImU32 knobColor = ImGui::GetColorU32(ImVec4(0.95f, 0.95f, 0.95f, 1.0f));

    ImDrawList* drawList = window->DrawList;

    // Background pill
    drawList->AddRectFilled(bb.Min, bb.Max, bgColor, radius);

    // Knob
    float knobPadding = 2.0f;
    float knobRadius = radius - knobPadding;
    float knobX = bb.Min.x + radius + anim * (width - height);
    float knobY = bb.Min.y + radius;
    drawList->AddCircleFilled(ImVec2(knobX, knobY), knobRadius, knobColor);

    return pressed;
}

} // namespace widgets
} // namespace lsproxy
