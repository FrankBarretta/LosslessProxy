#include "addon_manager.h"
#include "addon_dependency.h"
#include "addon_security.h"
#include "../gui/icon_loader.h"
#include "../config/config_manager.h"
#include "../event/event_system.h"
#include "../host/host_impl.h"
#include "../log/logger.h"
#include "../../third_party/nlohmann/json.hpp"
#include "imgui.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace lsproxy {

// SEH wrapper helpers - these must not have C++ objects with destructors
static uint32_t SehGetCaps(GetAddonCaps_t func) {
    __try { return func(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

static bool SehInitAddon(AddonInit_t func, IHost* host, ImGuiContext* ctx,
                         void* alloc, void* free, void* ud) {
    __try { func(host, ctx, alloc, free, ud); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

static void SehShutdownAddon(AddonShutdown_t func) {
    __try { func(); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}

static void SehRenderSettings(AddonRenderSettings_t func) {
    __try { func(); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}

static bool SehInterceptResource(AddonInterceptResource_t func,
                                 const wchar_t* name, const wchar_t* type,
                                 const void** outData, uint32_t* outSize) {
    __try { return func(name, type, outData, outSize); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

static const char* SehGetString(const char*(*func)()) {
    __try { return func(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}

// Helper: wstring to UTF-8
static std::string WToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size, nullptr, nullptr);
    return result;
}

AddonManager::AddonManager(HostImpl* host) : m_host(host) {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    fs::path exePath(buffer);
    m_addonsPath = (exePath.parent_path() / "addons").wstring();
    m_configPath = (exePath.parent_path() / "addons" / "config.json").wstring();

    if (!fs::exists(m_addonsPath)) {
        fs::create_directory(m_addonsPath);
    }

    AddonSecurity::LoadTrustedHashes(m_addonsPath);
    ConfigManager::Instance().Load(m_configPath);
}

AddonManager::~AddonManager() {
    UnloadAddons();
    // Release icon textures
    for (auto& addon : m_addons) {
        if (addon.iconTexture) {
            addon.iconTexture->Release();
            addon.iconTexture = nullptr;
        }
    }
}

void AddonManager::ScanAddons() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_addons.clear();

    if (!fs::exists(m_addonsPath)) return;

    for (const auto& entry : fs::directory_iterator(m_addonsPath)) {
        if (!fs::is_directory(entry.path())) continue;

        AddonInfo info;
        info.folderName = entry.path().filename().wstring();
        info.id = WToUtf8(info.folderName);

        fs::path manifestPath = entry.path() / "addon.json";
        if (fs::exists(manifestPath)) {
            ParseManifest(info, manifestPath.wstring());
        }

        bool foundDll = false;

        if (!info.manifest.dll.empty()) {
            fs::path dllPath = entry.path() / info.manifest.dll;
            if (fs::exists(dllPath)) {
                info.dllPath = dllPath.wstring();
                foundDll = true;
            }
        }

        if (!foundDll) {
            fs::path expectedDll = entry.path() / (entry.path().filename().string() + ".dll");
            if (fs::exists(expectedDll)) {
                info.dllPath = expectedDll.wstring();
                foundDll = true;
            }
        }

        if (!foundDll) {
            for (const auto& subEntry : fs::directory_iterator(entry.path())) {
                if (subEntry.path().extension() == ".dll") {
                    info.dllPath = subEntry.path().wstring();
                    foundDll = true;
                    break;
                }
            }
        }

        for (const auto& subEntry : fs::directory_iterator(entry.path())) {
            if (subEntry.path().extension() == ".ini") {
                info.configPath = subEntry.path().wstring();
                break;
            }
        }

        if (!foundDll) continue;

        // Discover icon: manifest "icon" field, or auto-detect icon.png/jpg
        if (!info.manifest.icon.empty()) {
            fs::path iconPath = entry.path() / info.manifest.icon;
            if (fs::exists(iconPath)) info.iconPath = iconPath.wstring();
        }
        if (info.iconPath.empty()) {
            for (const auto& ext : {".png", ".jpg", ".jpeg", ".bmp"}) {
                fs::path iconPath = entry.path() / ("icon" + std::string(ext));
                if (fs::exists(iconPath)) { info.iconPath = iconPath.wstring(); break; }
            }
        }

        info.enabled = ConfigManager::Instance().IsAddonEnabled(info.id, true);
        info.security = AddonSecurity::VerifyDll(info.dllPath, info.id);

        m_addons.push_back(std::move(info));
    }

    AddonDependency::Resolve(m_addons);
    LOG_INFO("AddonManager", "Scanned %zu addons", m_addons.size());
}

void AddonManager::ParseManifest(AddonInfo& addon, const std::wstring& jsonPath) {
    try {
        std::ifstream file(jsonPath);
        auto data = nlohmann::json::parse(file);

        if (data.contains("name")) addon.manifest.name = data["name"].get<std::string>();
        if (data.contains("version")) addon.manifest.version = data["version"].get<std::string>();
        if (data.contains("author")) addon.manifest.author = data["author"].get<std::string>();
        if (data.contains("description")) addon.manifest.description = data["description"].get<std::string>();
        if (data.contains("min_host_version")) addon.manifest.minHostVersion = data["min_host_version"].get<std::string>();
        if (data.contains("dll")) addon.manifest.dll = data["dll"].get<std::string>();
        if (data.contains("icon")) addon.manifest.icon = data["icon"].get<std::string>();

        if (data.contains("dependencies") && data["dependencies"].is_array()) {
            for (const auto& dep : data["dependencies"])
                addon.manifest.dependencies.push_back(dep.get<std::string>());
        }
        if (data.contains("tags") && data["tags"].is_array()) {
            for (const auto& tag : data["tags"])
                addon.manifest.tags.push_back(tag.get<std::string>());
        }

        addon.manifest.parsed = true;
    } catch (const std::exception& e) {
        LOG_WARN("AddonManager", "Failed to parse addon.json for '%s': %s", addon.id.c_str(), e.what());
    }
}

void AddonManager::PopulateFromExports(AddonInfo& addon) {
    if (!addon.hModule) return;

    if (addon.manifest.name.empty() && addon.GetNameFunc) {
        const char* val = SehGetString(addon.GetNameFunc);
        if (val) addon.manifest.name = val;
    }
    if (addon.manifest.version.empty() && addon.GetVersionFunc) {
        const char* val = SehGetString(addon.GetVersionFunc);
        if (val) addon.manifest.version = val;
    }
    if (addon.manifest.author.empty() && addon.GetAuthorFunc) {
        const char* val = SehGetString(addon.GetAuthorFunc);
        if (val) addon.manifest.author = val;
    }
    if (addon.manifest.description.empty() && addon.GetDescFunc) {
        const char* val = SehGetString(addon.GetDescFunc);
        if (val) addon.manifest.description = val;
    }
}

void AddonManager::LoadAddons() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& addon : m_addons) {
        if (addon.enabled && !addon.hModule) {
            LoadAddon(addon);
        }
    }
}

void AddonManager::LoadAddon(AddonInfo& addon) {
    HMODULE hAddon = LoadLibraryExW(addon.dllPath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hAddon) {
        hAddon = LoadLibraryW(addon.dllPath.c_str());
    }

    if (!hAddon) {
        DWORD err = GetLastError();
        addon.errorMessage = "LoadLibrary failed (error " + std::to_string(err) + ")";
        LOG_ERROR("AddonManager", "Failed to load '%s': %s", addon.id.c_str(), addon.errorMessage.c_str());
        return;
    }

    addon.hModule = hAddon;
    addon.errorMessage.clear();
    addon.faulted = false;

    addon.InitFunc = (AddonInit_t)GetProcAddress(hAddon, "AddonInitialize");
    if (!addon.InitFunc)
        addon.InitFunc = (AddonInit_t)GetProcAddress(hAddon, "AddonInit");

    addon.ShutdownFunc = (AddonShutdown_t)GetProcAddress(hAddon, "AddonShutdown");
    addon.RenderSettingsFunc = (AddonRenderSettings_t)GetProcAddress(hAddon, "AddonRenderSettings");
    addon.InterceptResourceFunc = (AddonInterceptResource_t)GetProcAddress(hAddon, "AddonInterceptResource");
    addon.GetNameFunc = (GetAddonName_t)GetProcAddress(hAddon, "GetAddonName");
    addon.GetVersionFunc = (GetAddonVersion_t)GetProcAddress(hAddon, "GetAddonVersion");
    addon.GetAuthorFunc = (GetAddonAuthor_t)GetProcAddress(hAddon, "GetAddonAuthor");
    addon.GetDescFunc = (GetAddonDescription_t)GetProcAddress(hAddon, "GetAddonDescription");

    GetAddonCaps_t getCaps = (GetAddonCaps_t)GetProcAddress(hAddon, "GetAddonCapabilities");
    if (getCaps) {
        addon.capabilities = SehGetCaps(getCaps);
    }

    PopulateFromExports(addon);

    LOG_INFO("AddonManager", "Loaded addon '%s' v%s",
             addon.GetDisplayName().c_str(), addon.GetDisplayVersion().c_str());
}

void AddonManager::InitializeAddons(ImGuiContext* ctx) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ImGuiMemAllocFunc alloc_func;
    ImGuiMemFreeFunc free_func;
    void* user_data;
    ImGui::GetAllocatorFunctions(&alloc_func, &free_func, &user_data);

    for (auto& addon : m_addons) {
        if (!addon.enabled || !addon.hModule || !addon.InitFunc) continue;

        bool success = SehInitAddon(addon.InitFunc, m_host, ctx,
                                    (void*)alloc_func, (void*)free_func, user_data);
        if (!success) {
            addon.faulted = true;
            addon.errorMessage = "Crashed during initialization";
            LOG_ERROR("AddonManager", "Addon '%s' crashed during init!", addon.id.c_str());
        } else {
            LsProxyAddonEventData eventData;
            eventData.addonName = addon.GetDisplayName().c_str();
            eventData.addonVersion = addon.GetDisplayVersion().c_str();
            EventBus::Instance().Publish(LSPROXY_EVENT_ADDON_LOADED, &eventData, sizeof(eventData));
        }
    }
}

void AddonManager::UnloadAddons() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (int i = (int)m_addons.size() - 1; i >= 0; i--) {
        if (m_addons[i].hModule) {
            UnloadAddon(m_addons[i]);
        }
    }
}

void AddonManager::UnloadAddon(AddonInfo& addon) {
    if (!addon.hModule) return;

    if (addon.ShutdownFunc) {
        SehShutdownAddon(addon.ShutdownFunc);
    }

    LsProxyAddonEventData eventData;
    eventData.addonName = addon.GetDisplayName().c_str();
    eventData.addonVersion = addon.GetDisplayVersion().c_str();
    EventBus::Instance().Publish(LSPROXY_EVENT_ADDON_UNLOADED, &eventData, sizeof(eventData));

    FreeLibrary(addon.hModule);
    addon.hModule = nullptr;
    addon.InitFunc = nullptr;
    addon.ShutdownFunc = nullptr;
    addon.RenderSettingsFunc = nullptr;
    addon.InterceptResourceFunc = nullptr;
    addon.GetNameFunc = nullptr;
    addon.GetVersionFunc = nullptr;
    addon.GetAuthorFunc = nullptr;
    addon.GetDescFunc = nullptr;
    addon.capabilities = 0;
    addon.faulted = false;
}

void AddonManager::ReloadAddons() {
    UnloadAddons();
    ScanAddons();
    LoadAddons();
}

void AddonManager::LoadAddonIcons() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& addon : m_addons) {
        if (addon.iconTexture) continue; // Already loaded
        if (addon.iconPath.empty()) continue;

        addon.iconTexture = IconLoader_LoadFromFile(addon.iconPath);
    }
}

void AddonManager::ToggleAddon(int index, bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index < 0 || index >= (int)m_addons.size()) return;

    auto& addon = m_addons[index];
    addon.enabled = enable;

    // If addon requires restart, just save the config - don't hot load/unload
    if (addon.RequiresRestart()) {
        LOG_INFO("AddonManager", "Addon '%s' requires restart to %s",
                 addon.GetDisplayName().c_str(), enable ? "enable" : "disable");
    } else {
        if (enable && !addon.hModule) {
            LoadAddon(addon);
            if (addon.hModule && addon.InitFunc) {
                ImGuiContext* ctx = ImGui::GetCurrentContext();
                if (ctx) {
                    ImGuiMemAllocFunc alloc_func;
                    ImGuiMemFreeFunc free_func;
                    void* user_data;
                    ImGui::GetAllocatorFunctions(&alloc_func, &free_func, &user_data);

                    if (!SehInitAddon(addon.InitFunc, m_host, ctx,
                                      (void*)alloc_func, (void*)free_func, user_data)) {
                        addon.faulted = true;
                        addon.errorMessage = "Crashed during initialization";
                    }
                }
            }
        } else if (!enable && addon.hModule) {
            UnloadAddon(addon);
        }
    }

    ConfigManager::Instance().SetAddonEnabled(addon.id, enable);
    ConfigManager::Instance().Save();
}

void AddonManager::RenderAddonSettings(int index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index < 0 || index >= (int)m_addons.size()) return;

    auto& addon = m_addons[index];
    if (addon.hModule && addon.RenderSettingsFunc) {
        SehRenderSettings(addon.RenderSettingsFunc);
        // Check for fault after return - can't easily detect from here
        // but at least we won't crash the host
    }
}

bool AddonManager::InterceptResource(const wchar_t* name, const wchar_t* type,
                                     const void** outData, uint32_t* outSize) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& addon : m_addons) {
        if (addon.enabled && addon.InterceptResourceFunc && !addon.faulted) {
            if (SehInterceptResource(addon.InterceptResourceFunc, name, type, outData, outSize)) {
                return true;
            }
        }
    }
    return false;
}

std::vector<AddonInfo>& AddonManager::GetAddons() { return m_addons; }
std::mutex& AddonManager::GetMutex() { return m_mutex; }

} // namespace lsproxy
