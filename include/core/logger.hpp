#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
private:
    static std::mutex log_mutex_;
    static std::ofstream log_file_;
    static LogLevel current_level_;
    static bool console_output_;
    static std::string log_filename_;
    
public:
    static void initialize(const std::string& filename = "perception.log", 
                         LogLevel level = LogLevel::INFO,
                         bool console = true) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        
        log_filename_ = filename;
        current_level_ = level;
        console_output_ = console;
        
        if (!filename.empty()) {
            log_file_.open(filename, std::ios::app);
        }
    }
    
    static void shutdown() {
        std::lock_guard<std::mutex> lock(log_mutex_);
        
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }
    
    static void set_level(LogLevel level) {
        current_level_ = level;
    }
    
    static void set_console_output(bool enabled) {
        console_output_ = enabled;
    }
    
    static void log(LogLevel level, const std::string& message, const std::string& file = "", int line = 0) {
        if (level < current_level_) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(log_mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        oss << " [" << level_to_string(level) << "]";
        
        if (!file.empty()) {
            oss << " [" << file << ":" << line << "]";
        }
        
        oss << " " << message;
        
        std::string log_entry = oss.str();
        
        // Output to console
        if (console_output_) {
            std::cout << log_entry << std::endl;
        }
        
        // Output to file
        if (log_file_.is_open()) {
            log_file_ << log_entry << std::endl;
            log_file_.flush();
        }
    }
    
    static void debug(const std::string& message, const std::string& file = "", int line = 0) {
        log(LogLevel::DEBUG, message, file, line);
    }
    
    static void info(const std::string& message, const std::string& file = "", int line = 0) {
        log(LogLevel::INFO, message, file, line);
    }
    
    static void warning(const std::string& message, const std::string& file = "", int line = 0) {
        log(LogLevel::WARNING, message, file, line);
    }
    
    static void error(const std::string& message, const std::string& file = "", int line = 0) {
        log(LogLevel::ERROR, message, file, line);
    }
    
    static void fatal(const std::string& message, const std::string& file = "", int line = 0) {
        log(LogLevel::FATAL, message, file, line);
    }
    
    static LogLevel level_from_string(const std::string& level_str) {
        if (level_str == "DEBUG") return LogLevel::DEBUG;
        if (level_str == "INFO") return LogLevel::INFO;
        if (level_str == "WARNING") return LogLevel::WARNING;
        if (level_str == "ERROR") return LogLevel::ERROR;
        if (level_str == "FATAL") return LogLevel::FATAL;
        return LogLevel::INFO; // Default
    }
    
private:
    static std::string level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }
};

// Static member definitions
std::mutex Logger::log_mutex_;
std::ofstream Logger::log_file_;
LogLevel Logger::current_level_ = LogLevel::INFO;
bool Logger::console_output_ = true;
std::string Logger::log_filename_ = "perception.log";

// Convenience macros
#define LOG_DEBUG(msg) Logger::debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) Logger::fatal(msg, __FILE__, __LINE__)

// Formatted logging macros (C++17 format style)
#define LOG_DEBUG_F(fmt, ...) Logger::debug(string_format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define LOG_INFO_F(fmt, ...) Logger::info(string_format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define LOG_WARNING_F(fmt, ...) Logger::warning(string_format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define LOG_ERROR_F(fmt, ...) Logger::error(string_format(fmt, __VA_ARGS__), __FILE__, __LINE__)
#define LOG_FATAL_F(fmt, ...) Logger::fatal(string_format(fmt, __VA_ARGS__), __FILE__, __LINE__)

// Simple string formatting helper (for C++17)
template<typename... Args>
std::string string_format(const std::string& format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
    if (size_s <= 0) { return ""; }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);
}
