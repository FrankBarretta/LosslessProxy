#include "toast.h"
#include <deque>

namespace lsproxy {
namespace widgets {

struct ToastEntry {
    std::string message;
    ToastType type;
    float duration;
    float elapsed;
};

static std::deque<ToastEntry> s_toasts;
static constexpr int MAX_VISIBLE = 5;

void ToastShow(const std::string& message, ToastType type, float duration) {
    s_toasts.push_back({message, type, duration, 0.0f});
    if (s_toasts.size() > 20) s_toasts.pop_front();
}

void ToastRender() {
    float dt = ImGui::GetIO().DeltaTime;
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    float yOffset = 10.0f;
    int visible = 0;

    for (int i = (int)s_toasts.size() - 1; i >= 0 && visible < MAX_VISIBLE; i--) {
        auto& t = s_toasts[i];
        t.elapsed += dt;

        if (t.elapsed >= t.duration) continue;

        // Fade in/out
        float alpha = 1.0f;
        if (t.elapsed < 0.3f) alpha = t.elapsed / 0.3f;
        else if (t.elapsed > t.duration - 0.5f) alpha = (t.duration - t.elapsed) / 0.5f;
        if (alpha < 0.0f) alpha = 0.0f;

        // Color based on type
        ImVec4 bgColor;
        switch (t.type) {
            case ToastType::Success: bgColor = ImVec4(0.15f, 0.65f, 0.15f, 0.9f * alpha); break;
            case ToastType::Warning: bgColor = ImVec4(0.85f, 0.65f, 0.10f, 0.9f * alpha); break;
            case ToastType::Error:   bgColor = ImVec4(0.85f, 0.15f, 0.15f, 0.9f * alpha); break;
            default:                 bgColor = ImVec4(0.20f, 0.20f, 0.20f, 0.9f * alpha); break;
        }

        ImVec2 textSize = ImGui::CalcTextSize(t.message.c_str());
        float padding = 12.0f;
        float toastWidth = textSize.x + padding * 2;
        float toastHeight = textSize.y + padding * 2;

        float x = displaySize.x - toastWidth - 15.0f;
        float y = yOffset;

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        drawList->AddRectFilled(
            ImVec2(x, y), ImVec2(x + toastWidth, y + toastHeight),
            ImGui::GetColorU32(bgColor), 6.0f);
        drawList->AddText(
            ImVec2(x + padding, y + padding),
            ImGui::GetColorU32(ImVec4(1, 1, 1, alpha)),
            t.message.c_str());

        yOffset += toastHeight + 5.0f;
        visible++;
    }

    // Remove expired
    while (!s_toasts.empty() && s_toasts.front().elapsed >= s_toasts.front().duration) {
        s_toasts.pop_front();
    }
}

} // namespace widgets
} // namespace lsproxy
