#include "tab_about.h"
#include "../../../sdk/include/lsproxy/version.h"
#include "imgui.h"
#include <cstdio>

namespace lsproxy {

void RenderTabAbout() {
    ImGui::Dummy(ImVec2(0, 20));

    // Title
    float titleWidth = ImGui::CalcTextSize("LosslessProxy Addon Manager").x;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - titleWidth) * 0.5f);
    ImGui::Text("LosslessProxy Addon Manager");

    ImGui::Dummy(ImVec2(0, 5));

    // Version
    char versionStr[64];
    snprintf(versionStr, sizeof(versionStr), "v%s", LSPROXY_VERSION_STRING);
    float verWidth = ImGui::CalcTextSize(versionStr).x;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - verWidth) * 0.5f);
    ImGui::TextDisabled("%s", versionStr);

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    ImGui::TextWrapped(
        "An addon manager and plugin framework for Lossless Scaling. "
        "Addons can replace shaders, modify behavior, and extend functionality "
        "through the plugin SDK.");

    ImGui::Dummy(ImVec2(0, 15));

    ImGui::TextDisabled("Plugin SDK:");
    ImGui::SameLine();
    ImGui::Text("v%s", LSPROXY_VERSION_STRING);

    ImGui::TextDisabled("ImGui:");
    ImGui::SameLine();
    ImGui::Text("%s", ImGui::GetVersion());

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    ImGui::TextDisabled("For addon development, include the SDK headers:");
    ImGui::Dummy(ImVec2(0, 3));
    ImGui::TextWrapped("#include <lsproxy/addon_sdk.h>");
}

} // namespace lsproxy
