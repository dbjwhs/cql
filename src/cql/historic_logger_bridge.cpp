// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/historic_logger_bridge.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace cql {

HistoricLoggerBridge::HistoricLoggerBridge(const std::string& log_file_path) 
    : m_log_file_path(log_file_path) {
    
    // Validate path exists (matching historic behavior)
    if (!std::filesystem::exists(std::filesystem::path(log_file_path).parent_path())) {
        throw std::runtime_error("Invalid path provided: " + log_file_path);
    }
    
    // Open log file in append mode (matching historic behavior)
    m_log_file.open(log_file_path, std::ios::app);
    if (!m_log_file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + log_file_path);
    }
}

HistoricLoggerBridge::~HistoricLoggerBridge() {
    if (m_log_file.is_open()) {
        m_log_file.close();
    }
}

void HistoricLoggerBridge::log(LogLevel level, const std::string& message) {
    if (!is_level_enabled(level)) {
        return;
    }
    
    // Create message in historic format: timestamp + level + thread + message
    std::string formatted_message = create_log_prefix(level) + message + "\n";
    write_log_message_historic(level, formatted_message);
}

bool HistoricLoggerBridge::is_level_enabled(LogLevel level) const {
    size_t index = log_level_to_index(level);
    return m_enabled_levels[index];
}

void HistoricLoggerBridge::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_log_file.is_open()) {
        m_log_file.flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

void HistoricLoggerBridge::set_level_enabled(LogLevel level, bool enabled) {
    size_t index = log_level_to_index(level);
    m_enabled_levels[index] = enabled;
}

void HistoricLoggerBridge::set_file_output_enabled(bool enabled) {
    m_file_output_enabled = enabled;
}

void HistoricLoggerBridge::set_stderr_enabled(bool enabled) {
    m_stderr_enabled = enabled;
}

bool HistoricLoggerBridge::is_stderr_enabled() const {
    return m_stderr_enabled;
}

size_t HistoricLoggerBridge::log_level_to_index(LogLevel level) {
    // Map new LogLevel enum to historic index:
    // Historic: INFO=0, NORMAL=1, DEBUG=2, ERROR=3, CRITICAL=4
    // New: DEBUG=0, INFO=1, NORMAL=2, ERROR=3, CRITICAL=4
    switch (level) {
        case LogLevel::DEBUG:    return 2; // Historic DEBUG index
        case LogLevel::INFO:     return 0; // Historic INFO index  
        case LogLevel::NORMAL:   return 1; // Historic NORMAL index
        case LogLevel::ERROR:    return 3; // Historic ERROR index
        case LogLevel::CRITICAL: return 4; // Historic CRITICAL index
        default:                 return 0; // Default to INFO
    }
}

std::string HistoricLoggerBridge::historic_log_level_to_string(LogLevel level) {
    // Return strings matching historic Logger format
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::NORMAL:   return "NORMAL";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string HistoricLoggerBridge::get_utc_timestamp() {
    // Match historic timestamp format exactly
    const auto now = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    struct tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t);
#else
    gmtime_r(&time_t, &tm_buf);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << " UTC";
    return ss.str();
}

std::string HistoricLoggerBridge::create_log_prefix(LogLevel level) {
    // Match historic format exactly: "YYYY-MM-DD HH:MM:SS.mmm UTC [LEVEL] [Thread:id] "
    std::ostringstream prefix;
    prefix << get_utc_timestamp()
           << " [" << historic_log_level_to_string(level) << "]"
           << " [Thread:" << std::this_thread::get_id() << "] ";
    return prefix.str();
}

void HistoricLoggerBridge::write_log_message_historic(LogLevel level, const std::string& formatted_message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Write to file if file output is enabled (matching historic behavior)
    if (m_file_output_enabled && m_log_file.is_open()) {
        m_log_file << formatted_message;
        m_log_file.flush();
    }
    
    // Write to console (matching historic behavior)
    if (level == LogLevel::CRITICAL || level == LogLevel::ERROR) {
        if (m_stderr_enabled) {
            std::cerr << formatted_message;
        }
    } else {
        // INFO, NORMAL, DEBUG go to stdout
        std::cout << formatted_message;
    }
}

} // namespace cql
