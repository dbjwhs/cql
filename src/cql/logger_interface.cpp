// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/logger_interface.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
#else
    #include <unistd.h>
#endif

namespace cql {

// ANSI color codes for terminal output
namespace colors {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
}

DefaultConsoleLogger::DefaultConsoleLogger() {
    // Detect if we're outputting to a terminal for color support
    m_colored_output = isatty(fileno(stdout)) && isatty(fileno(stderr));
}

void DefaultConsoleLogger::log(LogLevel level, const std::string& message) {
    if (!is_level_enabled(level)) {
        return;
    }
    
    std::string formatted_message = format_message(level, message);
    
    // Output to stderr for errors and critical messages, stdout for others
    if (level >= LogLevel::ERROR) {
        std::cerr << formatted_message << std::endl;
        std::cerr.flush();
    } else {
        std::cout << formatted_message << std::endl;
        std::cout.flush();
    }
}

bool DefaultConsoleLogger::is_level_enabled(LogLevel level) const {
    return level >= m_min_level;
}

void DefaultConsoleLogger::flush() {
    std::cout.flush();
    std::cerr.flush();
}

void DefaultConsoleLogger::set_min_level(LogLevel min_level) {
    m_min_level = min_level;
}

void DefaultConsoleLogger::set_colored_output(bool enable) {
    m_colored_output = enable;
}

std::string DefaultConsoleLogger::format_message(LogLevel level, const std::string& message) const {
    std::ostringstream formatted;
    
    // Add timestamp
    formatted << get_timestamp();
    
    // Add colored level indicator
    if (m_colored_output) {
        formatted << " " << get_color_code(level) << "[" << log_level_to_string(level) << "]" << colors::RESET;
    } else {
        formatted << " [" << log_level_to_string(level) << "]";
    }
    
    // Add thread ID (simplified)
    formatted << " [Thread:" << std::this_thread::get_id() << "] ";
    
    // Add the actual message
    formatted << message;
    
    return formatted.str();
}

std::string DefaultConsoleLogger::get_color_code(LogLevel level) const {
    if (!m_colored_output) {
        return "";
    }
    
    switch (level) {
        case LogLevel::DEBUG:    return std::string(colors::DIM) + colors::CYAN;
        case LogLevel::INFO:     return colors::BLUE;
        case LogLevel::NORMAL:   return colors::GREEN;
        case LogLevel::ERROR:    return std::string(colors::BOLD) + colors::YELLOW;
        case LogLevel::CRITICAL: return std::string(colors::BOLD) + colors::RED;
        default:                 return colors::WHITE;
    }
}

std::string DefaultConsoleLogger::get_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto time = std::chrono::system_clock::to_time_t(now);
    
    struct tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &time);
#else
    gmtime_r(&time, &tm_buf);
#endif
    
    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << " UTC";
    return ss.str();
}

CallbackLogger::CallbackLogger(LoggingCallback callback, LevelFilter level_filter)
    : m_callback(std::move(callback)), m_level_filter(std::move(level_filter)) {
    if (!m_callback) {
        throw std::invalid_argument("Logging callback cannot be null");
    }
}

void CallbackLogger::log(LogLevel level, const std::string& message) {
    if (!is_level_enabled(level)) {
        return;
    }
    
    m_callback(level, message.c_str());
}

bool CallbackLogger::is_level_enabled(LogLevel level) const {
    if (m_level_filter) {
        return m_level_filter(level);
    }
    return true; // Default to all levels enabled if no filter provided
}

void CallbackLogger::flush() {
    // Callback loggers don't typically need explicit flushing
    // The user's callback is responsible for any necessary flushing
}

} // namespace cql
