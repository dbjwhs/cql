// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/application_controller.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/command_line_handler.hpp"
#include "../../include/cql/template_operations.hpp"
#include "../../include/cql/documentation_handler.hpp"
#include "../../include/cql/api_client.hpp"
#include <iostream>

namespace cql {

LogLevel ApplicationController::string_to_log_level(const std::string& level_str) {
    if (level_str == "INFO") return LogLevel::INFO;
    if (level_str == "NORMAL") return LogLevel::NORMAL;
    if (level_str == "DEBUG") return LogLevel::DEBUG;
    if (level_str == "ERROR") return LogLevel::ERROR;
    if (level_str == "CRITICAL") return LogLevel::CRITICAL;
    
    // Default to DEBUG if invalid level provided
    std::cerr << "Warning: Invalid log level '" << level_str << "', using DEBUG instead." << std::endl;
    return LogLevel::DEBUG;
}

int ApplicationController::handle_file_processing(const std::string& input_file,
                                                  const std::string& output_file,
                                                  bool use_clipboard,
                                                  bool include_header) {
    if (use_clipboard) {
        try {
            Logger::getInstance().log(LogLevel::INFO, "Processing file: ", input_file);
            std::cout << "Processing file: " << input_file << std::endl;

            // Copy to clipboard
            if (const std::string result = QueryProcessor::compile_file(input_file); util::copy_to_clipboard(result)) {
                std::cout << "Compiled query copied to clipboard" << std::endl;
                Logger::getInstance().log(LogLevel::INFO, "Compiled query copied to clipboard");
            } else {
                std::cerr << "Failed to copy to clipboard" << std::endl;
                Logger::getInstance().log(LogLevel::ERROR, "Failed to copy to clipboard");
                return CQL_ERROR;
            }
            return CQL_NO_ERROR;
        } catch (const std::exception& e) {
            std::cerr << "Error processing file: " << e.what() << std::endl;
            Logger::getInstance().log(LogLevel::ERROR, "Error processing file: ", e.what());
            return CQL_ERROR;
        }
    }
    if (!cli::process_file(input_file, output_file, include_header)) {
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

int ApplicationController::run(int argc, char* argv[]) {
    // Create command line handler
    CommandLineHandler cmd_handler(argc, argv);
    
    // Default debug level
    auto debug_level = LogLevel::DEBUG;
    
    // Check for debug level in arguments
    std::string debug_level_str;
    if (cmd_handler.find_and_remove_option("--debug-level", debug_level_str)) {
        debug_level = string_to_log_level(debug_level_str);
    }
    
    // Initialize logger
    auto& logger = Logger::getInstance();
    
    // Set the logging level
    logger.setToLevelEnabled(debug_level);
    
    // Check if headers should be included (default is clean output)
    bool include_headers = cmd_handler.has_option("--include-header");
    
    if (include_headers) {
        std::cout << "Starting CQL Compiler v" << CQL_VERSION_STRING << " (" << CQL_BUILD_TIMESTAMP << ")..." << std::endl;
    }
    logger.log(LogLevel::INFO, "Starting CQL Compiler v", CQL_VERSION_STRING, " (", CQL_BUILD_TIMESTAMP, ")...");
    
    // Log the debug level that was set
    const std::string level_name = debug_level == LogLevel::INFO ? "INFO" :
                             debug_level == LogLevel::NORMAL ? "NORMAL" : 
                             debug_level == LogLevel::DEBUG ? "DEBUG" : 
                             debug_level == LogLevel::ERROR ? "ERROR" : "CRITICAL";
    logger.log(LogLevel::INFO, "Log level set to: ", level_name);

    // Get updated argc/argv after removing debug option
    int effective_argc = cmd_handler.get_argc();
    char** effective_argv = cmd_handler.get_argv();
    
    // If the only argument was --debug-level, show help
    if (!debug_level_str.empty() && effective_argc == 1) {
        std::cout << "Log level set to: " << debug_level_str << std::endl;
        std::cout << "No other arguments provided." << std::endl;
        CommandLineHandler::print_help();
        return CQL_NO_ERROR;
    }

    try {
        // Handle a case with no arguments
        if (effective_argc <= 1) {
            std::cout << "No arguments provided." << std::endl;
            CommandLineHandler::print_help();
            std::cout << "\nTo run the application with a file, use: cql input.llm output.txt" << std::endl;
            return CQL_NO_ERROR;
        }

        // Parse the first argument from our modified argument array
        const std::string arg1 = effective_argv[1];
        if (include_headers) {
            std::cout << "Received argument: " << arg1 << std::endl;
        }

        // Dispatch to the appropriate handler based on the first argument
        if (arg1 == "--help" || arg1 == "-h") {
            CommandLineHandler::print_help();
        } else if (arg1 == "--interactive" || arg1 == "-i") {
            cli::run_interactive();
        } else if (arg1 == "--submit") {
            return ApiClient::handle_submit_command(effective_argc, effective_argv);
        } else if (arg1 == "--templates" || arg1 == "-l") {
            TemplateOperations::list_templates();
        } else if (arg1 == "--template" || arg1 == "-T") {
            return TemplateOperations::handle_template_command(effective_argc, effective_argv);
        } else if (arg1 == "--validate") {
            return TemplateOperations::handle_validate_command(effective_argc, effective_argv);
        } else if (arg1 == "--validate-all") {
            if (effective_argc < 3) {
                std::cerr << "Error: Path required for --validate-all" << std::endl;
                std::cerr << "Usage: cql --validate-all PATH" << std::endl;
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
                std::cerr << "Error: Input file required when using --clipboard option" << std::endl;
                return CQL_ERROR;
            }
            return handle_file_processing(effective_argv[2], "", true);
        } else if (arg1.substr(0, 2) == "--") {
            // Unknown option starting with "--"
            std::cerr << "Error: Unknown option: " << arg1 << std::endl;
            std::cerr << "Available options:" << std::endl;
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
        logger.log(LogLevel::ERROR, "Fatal error: ", e.what());
        return CQL_ERROR;
    }

    return CQL_NO_ERROR;
}

} // namespace cql