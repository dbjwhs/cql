// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include "project_utils.hpp"
#include "logger_adapters.hpp"

namespace cql {

/**
 * @class ApplicationController
 * @brief Main application controller that handles initialization and command dispatching
 * 
 * This class encapsulates the main application logic, leaving the main() function
 * with just the minimal task of creating and running the controller.
 */
class ApplicationController {
public:
    /**
     * @brief Run the application with command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return int Return code (0 for success, 1 for error)
     */
    [[nodiscard]] static int run(int argc, char* argv[]);

private:
    /**
     * @brief Convert string to LogLevel
     * @param level_str String representation of log level
     * @return LogLevel enum value
     */
    [[nodiscard]] static LogLevel string_to_log_level(const std::string& level_str);

    /**
     * @brief Handle file processing operations
     * @param input_file Input file path
     * @param output_file Output file path (optional)
     * @param use_clipboard Copy buffer to clipboard
     * @param include_header Include compiler headers and status messages
     * @return int Return code (0 for success, 1 for error)
     */
    [[nodiscard]] static int handle_file_processing(const std::string& input_file,
                                      const std::string& output_file,
                                      bool use_clipboard = false,
                                      bool include_header = false);

    /**
     * @brief Initialize the logger system with appropriate configuration
     * @param log_to_console Enable console logging
     * @param log_file_path Path to log file
     * @param debug_level Minimum log level to use (fallback for file if file_level not specified)
     * @param rotation_max_size Maximum file size for rotation (0 = disabled)
     * @param rotation_max_files Maximum number of rotated files to keep
     * @param timestamp_format Timestamp format for log messages
     * @param console_level Log level for console output (defaults to INFO for clean output)
     * @param file_level Log level for file output (uses debug_level if not specified)
     */
    static void initialize_logger(bool log_to_console,
                                  const std::string& log_file_path,
                                  LogLevel debug_level,
                                  size_t rotation_max_size = 0,
                                  size_t rotation_max_files = 5,
                                  const std::string& timestamp_format = "simple",
                                  std::optional<LogLevel> console_level = std::nullopt,
                                  std::optional<LogLevel> file_level = std::nullopt);

    /**
     * @brief Convert string to TimestampFormat
     * @param format_str String representation of timestamp format
     * @return TimestampFormat enum value
     */
    [[nodiscard]] static cql::adapters::TimestampFormat string_to_timestamp_format(const std::string& format_str);
};

} // namespace cql
