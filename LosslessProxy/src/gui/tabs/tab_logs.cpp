#include "tab_logs.h"
#include "../../log/logger.h"
#include "imgui.h"

namespace lsproxy {

static bool s_autoScroll = true;
static int s_levelFilter = (int)LogLevel::Trace;

void RenderTabLogs() {
    // Filter controls
    ImGui::Checkbox("Auto-scroll", &s_autoScroll);
    ImGui::SameLine();

    const char* levels[] = { "Trace", "Debug", "Info", "Warn", "Error" };
    ImGui::SetNextItemWidth(100);
    ImGui::Combo("Min Level", &s_levelFilter, levels, 5);
    ImGui::SameLine();

    if (ImGui::Button("Clear")) {
        Logger::Instance().Clear();
    }

    ImGui::Separator();

    // Log entries
    auto entries = Logger::Instance().GetEntries((LogLevel)s_levelFilter);

    ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGuiListClipper clipper;
    clipper.Begin((int)entries.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& e = entries[i];

            // Color by level
            ImVec4 color;
            switch (e.level) {
                case LogLevel::Trace: color = ImVec4(0.5f, 0.5f, 0.5f, 1); break;
                case LogLevel::Debug: color = ImVec4(0.6f, 0.6f, 0.8f, 1); break;
                case LogLevel::Info:  color = ImVec4(0.8f, 0.8f, 0.8f, 1); break;
                case LogLevel::Warn:  color = ImVec4(1.0f, 0.8f, 0.3f, 1); break;
                case LogLevel::Error: color = ImVec4(1.0f, 0.3f, 0.3f, 1); break;
                default:              color = ImVec4(0.8f, 0.8f, 0.8f, 1); break;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::Text("[%s] [%s] %s",
                       Logger::LevelToString(e.level),
                       e.source.c_str(),
                       e.message.c_str());
            ImGui::PopStyleColor();
        }
    }

    if (s_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

} // namespace lsproxy
