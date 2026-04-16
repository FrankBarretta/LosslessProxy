#pragma once
#include <mutex>
#include <string>
#include "../../third_party/nlohmann/json.hpp"

namespace lsproxy {

class ConfigManager {
public:
    static ConfigManager& Instance();

    void Load(const std::wstring& path);
    void Save();

    // Per-addon config
    std::string Get(const std::string& addonId, const std::string& key, const std::string& defaultVal = "");
    void Set(const std::string& addonId, const std::string& key, const std::string& value);

    // Addon enabled/disabled
    bool IsAddonEnabled(const std::string& addonId, bool defaultVal = true);
    void SetAddonEnabled(const std::string& addonId, bool enabled);

    // Global settings
    nlohmann::json& Global();
    const nlohmann::json& Global() const;

private:
    ConfigManager() = default;
    void MigrateFromIni();

    nlohmann::json m_data;
    std::wstring m_path;
    std::wstring m_basePath; // addons directory
    mutable std::mutex m_mutex;
    bool m_loaded = false;
};

} // namespace lsproxy
