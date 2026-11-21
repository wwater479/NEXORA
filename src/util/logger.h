#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace nexora::util {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void Init(const std::string& log_file_path, LogLevel min_level = LogLevel::Info);
    void Shutdown();

    void Log(LogLevel level, const char* file, int line, const std::string& message);
    void SetLogLevel(LogLevel level);
    LogLevel GetLogLevel() const { return min_level_; }

    void SetConsoleOutput(bool enable) { console_output_ = enable; }

private:
    Logger() = default;
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string LevelToString(LogLevel level) const;
    std::string FormatTimestamp() const;

    std::mutex mutex_;
    std::ofstream file_stream_;
    LogLevel min_level_ = LogLevel::Info;
    bool console_output_ = true;
    bool initialized_ = false;
};

class LogMessage {
public:
    LogMessage(LogLevel level, const char* file, int line)
        : level_(level), file_(file), line_(line) {}

    ~LogMessage() {
        Logger::Instance().Log(level_, file_, line_, stream_.str());
    }

    template <typename T>
    LogMessage& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

private:
    LogLevel level_;
    const char* file_;
    int line_;
    std::ostringstream stream_;
};

} // namespace nexora::util

#define LOG_DEBUG nexora::util::LogMessage(nexora::util::LogLevel::Debug, __FILE__, __LINE__)
#define LOG_INFO  nexora::util::LogMessage(nexora::util::LogLevel::Info,  __FILE__, __LINE__)
#define LOG_WARN  nexora::util::LogMessage(nexora::util::LogLevel::Warn,  __FILE__, __LINE__)
#define LOG_ERROR nexora::util::LogMessage(nexora::util::LogLevel::Error, __FILE__, __LINE__)
#define LOG_FATAL nexora::util::LogMessage(nexora::util::LogLevel::Fatal, __FILE__, __LINE__)
