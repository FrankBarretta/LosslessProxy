#include "logger.h"
#include <chrono>
#include <cstdio>
#include <windows.h>

namespace lsproxy {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    Shutdown();
}

void Logger::Init(const std::wstring& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    m_file.open(logFilePath, std::ios::out | std::ios::app);
    m_initialized = true;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) {
        m_file.close();
    }
    m_initialized = false;
}

void Logger::Log(LogLevel level, const char* source, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    LogV(level, source, fmt, args);
    va_end(args);
}

void Logger::LogV(LogLevel level, const char* source, const char* fmt, va_list args) {
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    LogEntry entry;
    entry.level = level;
    entry.source = source ? source : "";
    entry.message = buffer;
    entry.timestamp = static_cast<uint64_t>(ms);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_entries.push_back(entry);
    if (m_entries.size() > MAX_ENTRIES) {
        m_entries.pop_front();
    }

    if (m_file.is_open()) {
        m_file << "[" << LevelToString(level) << "] [" << entry.source << "] " << buffer << "\n";
        m_file.flush();
    }

    // Also output to debug console
    char dbgBuf[2200];
    snprintf(dbgBuf, sizeof(dbgBuf), "[LosslessProxy][%s][%s] %s\n",
             LevelToString(level), entry.source.c_str(), buffer);
    OutputDebugStringA(dbgBuf);
}

std::vector<LogEntry> Logger::GetEntries(LogLevel minLevel) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<LogEntry> result;
    result.reserve(m_entries.size());
    for (const auto& e : m_entries) {
        if (e.level >= minLevel) {
            result.push_back(e);
        }
    }
    return result;
}

void Logger::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

const char* Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "?";
    }
}

} // namespace lsproxy
