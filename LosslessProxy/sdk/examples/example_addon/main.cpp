// Example Addon for LosslessProxy v0.3.0
// Demonstrates the plugin SDK usage.
//
// Build: compile as a DLL and place in addons/ExampleAddon/ExampleAddon.dll
// Optionally create an addon.json manifest in the same folder.

#include <lsproxy/addon_sdk.h>
#include "imgui.h"

static IHost* g_host = nullptr;

LSPROXY_EXPORT void AddonInitialize(IHost* host, ImGuiContext* ctx,
                                    void* allocFunc, void* freeFunc, void* userData) {
    // Set ImGui context and allocator (required for shared ImGui usage)
    ImGui::SetCurrentContext(ctx);
    ImGui::SetAllocatorFunctions((ImGuiMemAllocFunc)allocFunc, (ImGuiMemFreeFunc)freeFunc, userData);

    g_host = host;
    g_host->Log(LSPROXY_LOG_INFO, "ExampleAddon initialized!");

    // Subscribe to an event
    g_host->SubscribeEvent(LSPROXY_EVENT_HOST_SHUTDOWN, [](uint32_t, const void*, uint32_t, void*) {
        // Cleanup on host shutdown
    }, nullptr);
}

LSPROXY_EXPORT void AddonShutdown() {
    if (g_host) {
        g_host->Log(LSPROXY_LOG_INFO, "ExampleAddon shutting down");
    }
    g_host = nullptr;
}

LSPROXY_EXPORT uint32_t GetAddonCapabilities() {
    return LSPROXY_CAP_HAS_SETTINGS;
}

LSPROXY_EXPORT void AddonRenderSettings() {
    ImGui::Text("Example Addon Settings");
    ImGui::Separator();
    static bool demoOption = false;
    ImGui::Checkbox("Enable demo feature", &demoOption);
    ImGui::TextDisabled("This is an example addon for the LosslessProxy SDK.");
}

LSPROXY_EXPORT const char* GetAddonName() { return "Example Addon"; }
LSPROXY_EXPORT const char* GetAddonVersion() { return "1.0.0"; }
LSPROXY_EXPORT const char* GetAddonAuthor() { return "LosslessProxy Team"; }
LSPROXY_EXPORT const char* GetAddonDescription() { return "A minimal example addon demonstrating the plugin SDK."; }
