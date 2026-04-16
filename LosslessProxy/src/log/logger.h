#pragma once
#include <cstdarg>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace lsproxy {

enum class LogLevel : uint8_t { Trace, Debug, Info, Warn, Error };

struct LogEntry {
    LogLevel level;
    std::string source;
    std::string message;
    uint64_t timestamp; // ms since epoch
};

class Logger {
public:
    static Logger& Instance();

    void Init(const std::wstring& logFilePath);
    void Shutdown();

    void Log(LogLevel level, const char* source, const char* fmt, ...);
    void LogV(LogLevel level, const char* source, const char* fmt, va_list args);

    // For UI consumption - returns a snapshot
    std::vector<LogEntry> GetEntries(LogLevel minLevel = LogLevel::Trace) const;
    void Clear();

    static const char* LevelToString(LogLevel level);

private:
    Logger() = default;
    ~Logger();

    static constexpr size_t MAX_ENTRIES = 10000;

    mutable std::mutex m_mutex;
    std::deque<LogEntry> m_entries;
    std::ofstream m_file;
    bool m_initialized = false;
};

// Convenience macros
#define LOG_TRACE(src, fmt, ...) ::lsproxy::Logger::Instance().Log(::lsproxy::LogLevel::Trace, src, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(src, fmt, ...) ::lsproxy::Logger::Instance().Log(::lsproxy::LogLevel::Debug, src, fmt, ##__VA_ARGS__)
#define LOG_INFO(src, fmt, ...)  ::lsproxy::Logger::Instance().Log(::lsproxy::LogLevel::Info, src, fmt, ##__VA_ARGS__)
#define LOG_WARN(src, fmt, ...)  ::lsproxy::Logger::Instance().Log(::lsproxy::LogLevel::Warn, src, fmt, ##__VA_ARGS__)
#define LOG_ERROR(src, fmt, ...) ::lsproxy::Logger::Instance().Log(::lsproxy::LogLevel::Error, src, fmt, ##__VA_ARGS__)

} // namespace lsproxy
