// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/application_controller.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/command_line_handler.hpp"
#include "../../include/cql/template_operations.hpp"
#include "../../include/cql/documentation_handler.hpp"
#include "../../include/cql/api_client.hpp"
#include "../../include/cql/meta_prompt_handler.hpp"
#include "../../include/cql/error_context.hpp"
#include "../../include/cql/input_validator.hpp"
#include "../../include/cql/logger_manager.hpp"
#include "../../include/cql/logger_adapters.hpp"
#include "../../include/cql/user_output_manager.hpp"
#include <iostream>

namespace cql {

LogLevel ApplicationController::string_to_log_level(const std::string& level_str) {
    if (level_str == "INFO") return LogLevel::INFO;
    if (level_str == "NORMAL") return LogLevel::NORMAL;
    if (level_str == "DEBUG") return LogLevel::DEBUG;
    if (level_str == "ERROR") return LogLevel::ERROR;
    if (level_str == "CRITICAL") return LogLevel::CRITICAL;

    // Default to DEBUG if invalid level provided
    UserOutputManager::warning("Invalid log level '", level_str, "', using DEBUG instead.");
    return LogLevel::DEBUG;
}

cql::adapters::TimestampFormat ApplicationController::string_to_timestamp_format(const std::string& format_str) {
    if (format_str == "iso8601" || format_str == "ISO8601") {
        return cql::adapters::TimestampFormat::ISO8601;
    }
    if (format_str == "iso8601-local" || format_str == "ISO8601_LOCAL") {
        return cql::adapters::TimestampFormat::ISO8601_LOCAL;
    }
    if (format_str == "simple" || format_str == "SIMPLE") {
        return cql::adapters::TimestampFormat::SIMPLE;
    }
    if (format_str == "epoch" || format_str == "EPOCH_MS") {
        return cql::adapters::TimestampFormat::EPOCH_MS;
    }
    if (format_str == "none" || format_str == "NONE") {
        return cql::adapters::TimestampFormat::NONE;
    }

    // Default to SIMPLE if invalid format provided
    UserOutputManager::warning("Invalid timestamp format '", format_str, "', using SIMPLE instead.");
    return cql::adapters::TimestampFormat::SIMPLE;
}

void ApplicationController::initialize_logger(bool log_to_console,
                                             const std::string& log_file_path,
                                             LogLevel debug_level,
                                             size_t rotation_max_size,
                                             size_t rotation_max_files,
                                             const std::string& timestamp_format,
                                             std::optional<LogLevel> console_level,
                                             std::optional<LogLevel> file_level) {
    auto ts_format = string_to_timestamp_format(timestamp_format);

    // Determine actual log levels to use
    LogLevel actual_file_level = file_level.value_or(debug_level);
    LogLevel actual_console_level = console_level.value_or(LogLevel::INFO);  // Default console to INFO

    if (log_to_console) {
        // Use multi-logger for both file and console
        auto multi_logger = std::make_unique<cql::adapters::MultiLogger>();

        // Add file logger with rotation and timestamp configuration
        // Set FileLogger to DEBUG so it accepts all levels - filtering done by LevelFilteredLogger
        auto file_logger = std::make_unique<cql::adapters::FileLogger>(log_file_path);
        file_logger->set_min_level(LogLevel::DEBUG);  // Accept all, filter via wrapper
        file_logger->set_timestamp_format(ts_format);
        if (rotation_max_size > 0) {
            file_logger->enable_rotation(rotation_max_size, rotation_max_files);
        }

        // Wrap file logger with level filter for independent control
        auto filtered_file_logger = std::make_unique<cql::adapters::LevelFilteredLogger>(
            std::move(file_logger), actual_file_level);
        multi_logger->add_logger(std::move(filtered_file_logger));

        // Add console logger with independent level control
        auto console_logger = std::make_unique<cql::DefaultConsoleLogger>();
        auto filtered_console_logger = std::make_unique<cql::adapters::LevelFilteredLogger>(
            std::move(console_logger), actual_console_level);
        multi_logger->add_logger(std::move(filtered_console_logger));

        cql::LoggerManager::initialize(std::move(multi_logger));
    } else {
        // Default: log to file only
        // Set FileLogger to DEBUG so it accepts all levels - filtering done by LevelFilteredLogger
        auto file_logger = std::make_unique<cql::adapters::FileLogger>(log_file_path);
        file_logger->set_min_level(LogLevel::DEBUG);  // Accept all, filter via wrapper
        file_logger->set_timestamp_format(ts_format);
        if (rotation_max_size > 0) {
            file_logger->enable_rotation(rotation_max_size, rotation_max_files);
        }

        // Wrap with level filter for consistency
        auto filtered_file_logger = std::make_unique<cql::adapters::LevelFilteredLogger>(
            std::move(file_logger), actual_file_level);
        cql::LoggerManager::initialize(std::move(filtered_file_logger));
    }
}

int ApplicationController::handle_file_processing(const std::string& input_file,
                                                  const std::string& output_file,
                                                  bool use_clipboard,
                                                  bool include_header) {
    if (use_clipboard) {
        try {
            UserOutputManager::info("Processing file: ", input_file);

            // Copy to clipboard
            if (const std::string result = QueryProcessor::compile_file(input_file); util::copy_to_clipboard(result)) {
                UserOutputManager::success("Compiled query copied to clipboard");
            } else {
                UserOutputManager::error("Failed to copy to clipboard");
                Logger::getInstance().log(LogLevel::ERROR, "Failed to copy to clipboard");
                return CQL_ERROR;
            }
            return CQL_NO_ERROR;
        } catch (const std::exception& e) {
            // Preserve error context with file processing information
            auto contextual_error = ErrorContextBuilder::from(e)
                .operation("processing file")
                .file(input_file)
                .detail("output_file", output_file)
                .detail("use_clipboard", use_clipboard ? "true" : "false")
                .at(__FILE__ ":" + std::to_string(__LINE__))
                .build();

            // Log with full context for debugging
            error_context_utils::log_contextual_exception(contextual_error);

            // Display user-friendly message
            UserOutputManager::error(contextual_error.get_user_summary());
            return CQL_ERROR;
        }
    }
    if (!cli::process_file(input_file, output_file, include_header)) {
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

int ApplicationController::run(int argc, char* argv[]) {
    // Initialize user output early for --version flag
    UserOutputManager::initialize();

    // Check for --version flag early, before any logging
    if (argc > 1) {
        std::string first_arg = argv[1];
        if (first_arg == "--version" || first_arg == "-v") {
            UserOutputManager::info("Claude Query Language (CQL) Compiler v", CQL_VERSION_STRING, " (", CQL_BUILD_TIMESTAMP, ")");
            return CQL_NO_ERROR;
        }
    }
    
    // Create command line handler
    CommandLineHandler cmd_handler(argc, argv);

    // Default debug level
    auto debug_level = LogLevel::NORMAL;

    // Check for debug level in arguments
    std::string debug_level_str;
    if (cmd_handler.find_and_remove_option("--debug-level", debug_level_str)) {
        debug_level = string_to_log_level(debug_level_str);
    }

    // Check for logging configuration flags
    bool log_to_console = cmd_handler.find_and_remove_flag("--log-console");

    std::string log_file_path = "cql.log";  // Default log file
    cmd_handler.find_and_remove_option("--log-file", log_file_path);

    // Parse rotation configuration
    std::string rotation_size_str;
    size_t rotation_max_size = 0;  // 0 = disabled by default
    if (cmd_handler.find_and_remove_option("--log-max-size", rotation_size_str)) {
        try {
            rotation_max_size = std::stoull(rotation_size_str);
        } catch (...) {
            UserOutputManager::warning("Invalid log max size '", rotation_size_str, "', rotation disabled.");
            rotation_max_size = 0;
        }
    }

    std::string rotation_count_str;
    size_t rotation_max_files = 5;  // Default: keep 5 rotated files
    if (cmd_handler.find_and_remove_option("--log-max-files", rotation_count_str)) {
        try {
            rotation_max_files = std::stoull(rotation_count_str);
        } catch (...) {
            UserOutputManager::warning("Invalid log max files '", rotation_count_str, "', using default (5).");
            rotation_max_files = 5;
        }
    }

    // Parse timestamp format
    std::string timestamp_format = "simple";  // Default
    cmd_handler.find_and_remove_option("--log-timestamp", timestamp_format);

    // Parse independent log levels for console and file (Phase 5)
    std::optional<LogLevel> console_level;
    std::string console_level_str;
    if (cmd_handler.find_and_remove_option("--console-level", console_level_str)) {
        console_level = string_to_log_level(console_level_str);
    }

    std::optional<LogLevel> file_level;
    std::string file_level_str;
    if (cmd_handler.find_and_remove_option("--file-level", file_level_str)) {
        file_level = string_to_log_level(file_level_str);
    }

    // Validate and secure the log file path
    try {
        log_file_path = InputValidator::resolve_path_securely(log_file_path);
    } catch (const SecurityValidationError& e) {
        UserOutputManager::error("Security Error: Invalid log file path: ", e.what());
        return CQL_ERROR;
    }

    // Initialize logger based on configuration
    // By default, log to file only. Use --log-console to also log to console.
    // Console defaults to INFO for clean output, file uses debug_level unless overridden.
    initialize_logger(log_to_console, log_file_path, debug_level,
                     rotation_max_size, rotation_max_files, timestamp_format,
                     console_level, file_level);

    // Get logger reference after initialization
    auto& logger = Logger::getInstance();

    // Set the logging level (for backward compatibility with LoggerBridge)
    logger.setToLevelEnabled(debug_level);
    
    // Check if headers should be included (default is clean output)
    bool include_headers = cmd_handler.find_and_remove_flag("--include-header");

    // Check for --env flag to load .env file
    if (cmd_handler.find_and_remove_flag("--env")) {
        try {
            if (util::load_env_file()) {
                // Only show if headers are enabled to keep output clean
                if (include_headers) {
                    UserOutputManager::success("Successfully loaded .env file");
                }
                logger.log(LogLevel::DEBUG, "Environment variables loaded from .env file");
            } else {
                UserOutputManager::warning("Could not load .env file");
                logger.log(LogLevel::DEBUG, "Failed to load .env file - file may not exist");
            }
        } catch (const SecurityValidationError& e) {
            UserOutputManager::error("Security Error: ", e.what());
            logger.log(LogLevel::ERROR, "Security validation failed for .env file: ", e.what());
            return CQL_ERROR;
        } catch (const std::exception& e) {
            UserOutputManager::error("Error loading .env file: ", e.what());
            logger.log(LogLevel::ERROR, "Exception while loading .env file: ", e.what());
            return CQL_ERROR;
        }
    }
    
    if (include_headers) {
        UserOutputManager::info("Starting CQL Compiler v", CQL_VERSION_STRING, " (", CQL_BUILD_TIMESTAMP, ")...");
    }

    // Only log startup info if headers are requested or debug level is explicitly set
    if (include_headers || !debug_level_str.empty()) {
        logger.log(LogLevel::INFO, "Starting CQL Compiler v", CQL_VERSION_STRING, " (", CQL_BUILD_TIMESTAMP, ")...");

        // Log the debug level that was set
        const std::string level_name = debug_level == LogLevel::INFO ? "INFO" :
                                 debug_level == LogLevel::NORMAL ? "NORMAL" :
                                 debug_level == LogLevel::DEBUG ? "DEBUG" :
                                 debug_level == LogLevel::ERROR ? "ERROR" : "CRITICAL";
        logger.log(LogLevel::INFO, "Log level set to: ", level_name);
    }

    // Get updated argc/argv after removing debug option
    int effective_argc = cmd_handler.get_argc();
    char** effective_argv = cmd_handler.get_argv();

    // If the only argument was --debug-level, show help
    if (!debug_level_str.empty() && effective_argc == 1) {
        UserOutputManager::info("Log level set to: ", debug_level_str);
        UserOutputManager::info("No other arguments provided.");
        CommandLineHandler::print_help();
        return CQL_NO_ERROR;
    }

    try {
        // Handle a case with no arguments
        if (effective_argc <= 1) {
            UserOutputManager::info("No arguments provided.");
            CommandLineHandler::print_help();
            UserOutputManager::info("\nTo run the application with a file, use: cql input.llm output.txt");
            return CQL_NO_ERROR;
        }

        // Parse the first argument from our modified argument array
        const std::string arg1 = effective_argv[1];
        if (include_headers) {
            UserOutputManager::info("Received argument: ", arg1);
        }

        // Dispatch to the appropriate handler based on the first argument
        if (arg1 == "--help" || arg1 == "-h") {
            CommandLineHandler::print_help();
        } else if (arg1 == "--interactive" || arg1 == "-i") {
            cli::run_interactive();
        } else if (arg1 == "--submit") {
            return ApiClient::handle_submit_command(effective_argc, effective_argv);
        } else if (arg1 == "--optimize") {
            return MetaPromptHandler::handle_optimize_command(effective_argc, effective_argv);
        } else if (arg1 == "--templates" || arg1 == "-l") {
            TemplateOperations::list_templates();
        } else if (arg1 == "--template" || arg1 == "-T") {
            return TemplateOperations::handle_template_command(effective_argc, effective_argv);
        } else if (arg1 == "--validate") {
            return TemplateOperations::handle_validate_command(effective_argc, effective_argv);
        } else if (arg1 == "--validate-all") {
            if (effective_argc < 3) {
                UserOutputManager::error("Path required for --validate-all");
                UserOutputManager::info("Usage: cql --validate-all PATH");
                return CQL_ERROR;
            }
            return TemplateOperations::handle_validate_all_command(effective_argv[2]);
        } else if (arg1 == "--docs") {
            return DocumentationHandler::handle_docs_command(effective_argc, effective_argv);
        } else if (arg1 == "--docs-all") {
            return DocumentationHandler::handle_docs_all_command();
        } else if (arg1 == "--export") {
            return DocumentationHandler::handle_export_command(effective_argc, effective_argv);
        } else if (arg1 == "--clipboard" || arg1 == "-c") {
            if (effective_argc < 3) {
                UserOutputManager::error("Input file required when using --clipboard option");
                return CQL_ERROR;
            }
            return handle_file_processing(effective_argv[2], "", true);
        } else if (arg1.substr(0, 2) == "--") {
            // Unknown option starting with "--"
            UserOutputManager::error("Unknown option: ", arg1);
            UserOutputManager::info("Available options:");
            CommandLineHandler::print_help();
            return CQL_ERROR;
        } else {
            // Assume it's an input file
            std::string output_file;
            bool use_clipboard = false;

            // Check if any of the arguments is --clipboard/-c
            for (int i = 2; i < effective_argc; ++i) {
                if (std::string arg = effective_argv[i]; arg == "--clipboard" || arg == "-c") {
                    use_clipboard = true;
                    break;
                } else if (output_file.empty() && arg.substr(0, 2) != "--") {
                    // If we have a non-option argument and output_file is empty, treat it as the output file
                    output_file = arg;
                }
            }

            return handle_file_processing(arg1, output_file, use_clipboard, include_headers);
        }
    } catch (const std::exception& e) {
        // Preserve error context for application-level errors
        auto contextual_error = ErrorContextBuilder::from(e)
            .operation("running CQL application")
            .detail("argc", std::to_string(effective_argc))
            .at(__FILE__ ":" + std::to_string(__LINE__))
            .build();

        // Log with full context for debugging
        error_context_utils::log_contextual_exception(contextual_error);

        // Display user-friendly message for fatal errors
        UserOutputManager::error("Fatal error: ", contextual_error.get_user_summary());
        return CQL_ERROR;
    }

    return CQL_NO_ERROR;
}

} // namespace cql
