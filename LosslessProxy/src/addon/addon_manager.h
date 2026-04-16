#pragma once
#include "addon_info.h"
#include <mutex>
#include <string>
#include <vector>

struct ImGuiContext;

namespace lsproxy {

class HostImpl;

class AddonManager {
public:
    explicit AddonManager(HostImpl* host);
    ~AddonManager();

    void ScanAddons();
    void LoadAddons();
    void UnloadAddons();
    void ReloadAddons();
    void InitializeAddons(ImGuiContext* ctx);

    // Thread-safe accessors
    std::vector<AddonInfo>& GetAddons();
    std::mutex& GetMutex();

    void ToggleAddon(int index, bool enable);
    void RenderAddonSettings(int index);
    bool InterceptResource(const wchar_t* name, const wchar_t* type,
                           const void** outData, uint32_t* outSize);

    // Load icon textures for all addons that have icon files
    void LoadAddonIcons();

    HostImpl* GetHost() const { return m_host; }

    // Paths
    const std::wstring& GetAddonsPath() const { return m_addonsPath; }

private:
    void LoadAddon(AddonInfo& addon);
    void UnloadAddon(AddonInfo& addon);
    void ParseManifest(AddonInfo& addon, const std::wstring& jsonPath);
    void PopulateFromExports(AddonInfo& addon);

    HostImpl* m_host;
    std::vector<AddonInfo> m_addons;
    std::mutex m_mutex;
    std::wstring m_addonsPath;
    std::wstring m_configPath;
};

} // namespace lsproxy
