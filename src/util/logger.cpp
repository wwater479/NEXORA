#include "logger.h"
#include <ctime>

namespace nexora::util {

Logger::~Logger() {
    Shutdown();
}

void Logger::Init(const std::string& log_file_path, LogLevel min_level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = min_level;
    if (!log_file_path.empty()) {
        file_stream_.open(log_file_path, std::ios::out | std::ios::app);
    }
    initialized_ = true;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.flush();
        file_stream_.close();
    }
    initialized_ = false;
}

void Logger::SetLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

std::string Logger::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default:              return "UNKWN";
    }
}

std::string Logger::FormatTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif
    std::ostringstream ss;
    ss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Logger::Log(LogLevel level, const char* file, int line, const std::string& message) {
    if (level < min_level_) {
        return;
    }

    std::string filename = file;
    size_t last_slash = filename.find_last_of("\\/");
    if (last_slash != std::string::npos) {
        filename = filename.substr(last_slash + 1);
    }

    std::string timestamp = FormatTimestamp();
    std::string level_str = LevelToString(level);

    std::ostringstream out;
    out << "[" << timestamp << "] [" << level_str << "] [" << filename << ":" << line << "] " << message << "\n";
    std::string log_line = out.str();

    std::lock_guard<std::mutex> lock(mutex_);
    if (console_output_) {
        if (level >= LogLevel::Error) {
            std::cerr << log_line;
        } else {
            std::cout << log_line;
        }
    }
    if (file_stream_.is_open()) {
        file_stream_ << log_line;
        file_stream_.flush();
    }
}

} // namespace nexora::util
