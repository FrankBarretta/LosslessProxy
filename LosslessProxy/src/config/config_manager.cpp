#include "config_manager.h"
#include "../log/logger.h"
#include <filesystem>
#include <fstream>
#include <windows.h>

namespace fs = std::filesystem;

namespace lsproxy {

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::Load(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_path = path;
    m_basePath = fs::path(path).parent_path().wstring();

    if (fs::exists(path)) {
        try {
            std::ifstream file(path);
            m_data = nlohmann::json::parse(file);
            m_loaded = true;
            LOG_INFO("Config", "Loaded config from config.json");
            return;
        } catch (const std::exception& e) {
            LOG_ERROR("Config", "Failed to parse config.json: %s", e.what());
        }
    }

    // Try migrating from old INI
    MigrateFromIni();
    m_loaded = true;
}

void ConfigManager::MigrateFromIni() {
    fs::path iniPath = fs::path(m_basePath) / "addons_config.ini";
    if (!fs::exists(iniPath)) {
        m_data = nlohmann::json{{"global", {}}, {"addons", {}}};
        return;
    }

    LOG_INFO("Config", "Migrating from addons_config.ini...");
    m_data = nlohmann::json{{"global", {}}, {"addons", {}}};

    // Scan addons directory for folder names to read INI keys
    fs::path addonsDir = m_basePath;
    if (fs::exists(addonsDir)) {
        for (const auto& entry : fs::directory_iterator(addonsDir)) {
            if (fs::is_directory(entry.path())) {
                std::wstring name = entry.path().filename().wstring();
                int status = GetPrivateProfileIntW(L"Addons", name.c_str(), 1, iniPath.wstring().c_str());

                // Convert wstring name to UTF-8
                int size = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), (int)name.size(), nullptr, 0, nullptr, nullptr);
                std::string utf8Name(size, 0);
                WideCharToMultiByte(CP_UTF8, 0, name.c_str(), (int)name.size(), &utf8Name[0], size, nullptr, nullptr);

                m_data["addons"][utf8Name] = nlohmann::json{{"enabled", status != 0}};
            }
        }
    }

    // Save the migrated config
    try {
        std::ofstream file(m_path);
        file << m_data.dump(2);
        LOG_INFO("Config", "Migration complete, saved config.json");
    } catch (...) {
        LOG_ERROR("Config", "Failed to save migrated config");
    }
}

void ConfigManager::Save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_loaded || m_path.empty()) {
        LOG_WARN("Config", "Save called before config was loaded, ignoring");
        return;
    }
    try {
        std::string json = m_data.dump(2);
        // Only write if we have actual content (protect against wiping the file)
        if (json.size() <= 4) {
            LOG_WARN("Config", "Save aborted: config data is empty");
            return;
        }
        std::ofstream file(m_path, std::ios::trunc);
        file << json;
    } catch (const std::exception& e) {
        LOG_ERROR("Config", "Failed to save config: %s", e.what());
    }
}

std::string ConfigManager::Get(const std::string& addonId, const std::string& key, const std::string& defaultVal) {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        if (m_data.contains("addons") && m_data["addons"].contains(addonId) &&
            m_data["addons"][addonId].contains(key)) {
            return m_data["addons"][addonId][key].get<std::string>();
        }
    } catch (...) {}
    return defaultVal;
}

void ConfigManager::Set(const std::string& addonId, const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data["addons"][addonId][key] = value;
}

bool ConfigManager::IsAddonEnabled(const std::string& addonId, bool defaultVal) {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        if (m_data.contains("addons") && m_data["addons"].contains(addonId) &&
            m_data["addons"][addonId].contains("enabled")) {
            return m_data["addons"][addonId]["enabled"].get<bool>();
        }
    } catch (...) {}
    return defaultVal;
}

void ConfigManager::SetAddonEnabled(const std::string& addonId, bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data["addons"][addonId]["enabled"] = enabled;
}

nlohmann::json& ConfigManager::Global() {
    return m_data["global"];
}

const nlohmann::json& ConfigManager::Global() const {
    return m_data["global"];
}

} // namespace lsproxy
