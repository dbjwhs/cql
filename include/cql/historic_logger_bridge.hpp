// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "logger_interface.hpp"
#include <fstream>
#include <mutex>
#include <thread>
#include <filesystem>
#include <iomanip>
#include <chrono>

namespace cql {

/**
 * @brief Bridge adapter that provides the historic Logger formatting 
 *        while using the new pluggable logger system internally
 * 
 * This class maintains the exact formatting and behavior of the historic
 * Logger system while routing calls through the new pluggable architecture.
 * This ensures backward compatibility while gaining pluggable capabilities.
 */
class HistoricLoggerBridge : public LoggerInterface {
public:
    /**
     * @brief Constructor with log file path
     * @param log_file_path Path to the log file (default: "../custom.log")
     */
    explicit HistoricLoggerBridge(const std::string& log_file_path = "../custom.log");
    
    /**
     * @brief Destructor - closes log file if open
     */
    ~HistoricLoggerBridge() override;
    
    void log(LogLevel level, const std::string& message) override;
    bool is_level_enabled(LogLevel level) const override;
    void flush() override;
    
    /**
     * @brief Enable/disable specific log levels
     * @param level The log level to configure
     * @param enabled Whether the level should be enabled
     */
    void set_level_enabled(LogLevel level, bool enabled);
    
    /**
     * @brief Enable/disable file output
     * @param enabled Whether file output should be enabled
     */
    void set_file_output_enabled(bool enabled);
    
    /**
     * @brief Enable/disable stderr output for errors
     * @param enabled Whether stderr output should be enabled
     */
    void set_stderr_enabled(bool enabled);
    
    /**
     * @brief Check if stderr output is enabled
     * @return true if stderr is enabled, false otherwise
     */
    bool is_stderr_enabled() const;

private:
    std::ofstream m_log_file;
    mutable std::mutex m_mutex;
    std::string m_log_file_path;
    
    // Level enablement - matches historic Logger behavior
    bool m_enabled_levels[5] = {true, true, true, true, true}; // INFO, NORMAL, DEBUG, ERROR, CRITICAL
    bool m_file_output_enabled = true;
    bool m_stderr_enabled = true;
    
    /**
     * @brief Convert new LogLevel to historic LogLevel index
     * @param level New system LogLevel
     * @return Index for historic level array
     */
    static size_t log_level_to_index(LogLevel level);
    
    /**
     * @brief Convert new LogLevel to historic string representation
     * @param level New system LogLevel
     * @return String representation matching historic format
     */
    static std::string historic_log_level_to_string(LogLevel level);
    
    /**
     * @brief Get UTC timestamp in historic format
     * @return Formatted timestamp string
     */
    static std::string get_utc_timestamp();
    
    /**
     * @brief Create log prefix in historic format
     * @param level Log level
     * @return Formatted log prefix with timestamp, level, and thread ID
     */
    static std::string create_log_prefix(LogLevel level);
    
    /**
     * @brief Write log message with historic behavior
     * @param level Log level  
     * @param formatted_message Complete formatted message
     */
    void write_log_message_historic(LogLevel level, const std::string& formatted_message);
};

} // namespace cql
