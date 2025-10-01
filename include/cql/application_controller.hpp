// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include "project_utils.hpp"

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
     * @param debug_level Minimum log level to use
     */
    static void initialize_logger(bool log_to_console,
                                  const std::string& log_file_path,
                                  LogLevel debug_level);
};

} // namespace cql
