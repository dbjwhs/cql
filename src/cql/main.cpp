// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <memory>  // For std::unique_ptr
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_validator.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include "../../include/cql/api_client.hpp"

// note the file exists in the cpp-snippets repo, you will need to check this out and have it and cql share the
// same root directory
#include "../../include/cql/project_utils.hpp"

/**
 * @brief Print the help message with usage information
 */
void print_help() {
    std::cout << "Claude Query Language (CQL) Compiler v" << CQL_VERSION_STRING << " (" << CQL_BUILD_TIMESTAMP << ")\n"
              << "Usage: cql [OPTIONS] [INPUT_FILE] [OUTPUT_FILE]\n\n"
              << "Options:\n"
              << "  --help, -h              Show this help information\n"
              << "  --interactive, -i       Run in interactive mode\n"
              << "  --clipboard, -c         Copy output to clipboard instead of writing to a file\n"
              << "  --include-header        Include compiler headers and status messages in output\n"
              << "  --debug-level LEVEL     Set log level (INFO|NORMAL|DEBUG|ERROR|CRITICAL, default: DEBUG)\n"
              << "  --templates, -l         List all available templates\n"
              << "  --template NAME, -T     Use a specific template\n"
              << "  --template NAME --force Use template even with validation errors\n"
              << "  --validate NAME         Validate a specific template\n"
              << "  --validate-all PATH     Validate all templates in the specified path\n"
              << "  --docs NAME             Generate documentation for a template\n"
              << "  --docs-all              Generate documentation for all templates\n"
              << "  --export PATH [format]  Export template documentation to a file\n"
              << "                          (formats: md, html, txt; default: md)\n\n"
              << "API Integration Options:\n"
              << "  --submit                Submit the compiled query to the Claude API\n"
              << "  --model <model_name>    Specify the Claude model to use (default: claude-3-opus)\n"
              << "  --output-dir <directory> Directory to save generated code files\n"
              << "  --overwrite             Overwrite existing files without prompting\n"
              << "  --create-dirs           Create missing directories for output files\n"
              << "  --no-save               Display generated code but don't save to files\n\n"
              << "If INPUT_FILE is provided, it will be processed as a CQL query.\n"
              << "If OUTPUT_FILE is also provided, the compiled query will be written to it.\n"
              << "If --clipboard option is used, the output will be copied to the clipboard.\n";
}


/**
 * @brief Convert string to LogLevel
 *
 * @param level_str String representation of log level
 * @return LogLevel enum value
 */
LogLevel string_to_log_level(const std::string& level_str) {
    if (level_str == "INFO") return LogLevel::INFO;
    if (level_str == "NORMAL") return LogLevel::NORMAL;
    if (level_str == "DEBUG") return LogLevel::DEBUG;
    if (level_str == "ERROR") return LogLevel::ERROR;
    if (level_str == "CRITICAL") return LogLevel::CRITICAL;
    
    // Default to DEBUG if invalid level provided
    std::cerr << "Warning: Invalid log level '" << level_str << "', using DEBUG instead." << std::endl;
    return LogLevel::DEBUG;
}

/**
 * @brief List all available templates
 */
void list_templates() {
    const cql::TemplateManager manager;

    if (const auto templates = manager.list_templates(); templates.empty()) {
        std::cout << "No templates found in " << manager.get_templates_directory() << std::endl;
    } else {
        std::cout << "Available templates:" << std::endl;
        for (const auto& tmpl : templates) {
            // get template metadata for more info
            try {
                auto metadata = manager.get_template_metadata(tmpl);
                std::cout << "  " << tmpl << " - " << metadata.description << std::endl;
            } catch (const std::exception&) {
                // if we can't get metadata, just show the name
                std::cout << "  " << tmpl << std::endl;
            }
        }
    }
}

/**
 * @brief Initialize a template validator with default schema
 *
 * @param manager Template manager to use
 * @return cql::TemplateValidator Initialized validator
 */
cql::TemplateValidator initialize_template_validator(const cql::TemplateManager& manager) {
    cql::TemplateValidator validator(manager);
    for (const auto schema = cql::TemplateValidatorSchema::create_default_schema();
        const auto &rule: schema.get_validation_rules() | std::views::values) {
        validator.add_validation_rule(rule);
    }
    return validator;
}

/**
 * @brief Process variables for template instantiation
 *
 * @param argc Argument count
 * @param argv Argument values
 * @param start_index Starting index in argv
 * @return std::map<std::string, std::string> Map of variable names to values
 */
std::map<std::string, std::string> process_template_variables(const int argc, char* argv[], const int start_index) {
    std::map<std::string, std::string> variables;

    for (int ndx = start_index; ndx < argc; ndx++) {
        std::string arg = argv[ndx];
        if (const size_t pos = arg.find('='); pos != std::string::npos) {
            std::string name = arg.substr(0, pos);
            const std::string value = arg.substr(pos + 1);
            variables[name] = value;
        }
    }
    return variables;
}

/**
 * @brief Check for --force flag in arguments
 *
 * @param argc Argument count
 * @param argv Argument values
 * @param start_index Starting index in argv
 * @return bool True if --force flag is present
 */
bool has_force_flag(const int argc, char* argv[], const int start_index) {
    for (int ndx = start_index; ndx < argc; ndx++) {
        if (std::string arg = argv[ndx]; arg == "--force" || arg == "-f") {
            return true;
        }
    }
    return false;
}

/**
 * @brief Handle submission to the Claude API
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
int handle_submit_command(const int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Input file required for --submit" << std::endl;
        std::cerr << "Usage: cql --submit INPUT_FILE [options]" << std::endl;
        return CQL_ERROR;
    }

    const std::string input_file = argv[2];
    std::string output_dir;
    std::string model;
    bool overwrite = false;
    bool create_dirs = false;
    bool no_save = false;

    // Parse additional options
    for (int ndx = 3; ndx < argc; ndx++) {
        if (std::string arg = argv[ndx]; arg == "--model" && ndx + 1 < argc) {
            model = argv[++ndx];
        } else if (arg == "--output-dir" && ndx + 1 < argc) {
            output_dir = argv[++ndx];
        } else if (arg == "--overwrite") {
            overwrite = true;
        } else if (arg == "--create-dirs") {
            create_dirs = true;
        } else if (arg == "--no-save") {
            no_save = true;
        }
    }

    // Process submission
    if (!cql::cli::process_submit_command(input_file, output_dir, model, overwrite, create_dirs, no_save)) {
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

/**
 * @brief Handle missing variables in templates
 *
 * @param validation_result Validation result
 * @param template_vars Template variables
 * @param variables User-provided variables
 * @return std::vector<std::string> List of missing variable names
 */
std::vector<std::string> handle_missing_variables(const cql::TemplateValidationResult& validation_result,
                             const std::map<std::string, std::string>& template_vars,
                             const std::map<std::string, std::string>& variables) {
    std::vector<std::string> missing_vars;

    // Extract variables from validation issues
    for (const auto var_issues = validation_result.get_issues(cql::TemplateValidationLevel::WARNING); const auto& issue : var_issues) {
        if (issue.get_variable_name().has_value() && issue.to_string().find("not declared") != std::string::npos) {
            if (std::string var_name = issue.get_variable_name().value(); !variables.contains(var_name) && !template_vars.contains(var_name)) {
                missing_vars.push_back(var_name);
            }
        }
    }

    // Warn about missing variables
    if (!missing_vars.empty()) {
        std::cerr << "Warning: The following variables are referenced but not provided:" << std::endl;
        for (const auto& var : missing_vars) {
            std::cerr << "  - " << var << std::endl;
        }
        std::cerr << "These will appear as '${" << missing_vars[0] << "}' in the output." << std::endl;
    }

    return missing_vars;
}

/**
 * @brief Use a specific template with variables
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
int handle_template_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Template name required" << std::endl;
        std::cerr << "Usage: cql --template TEMPLATE_NAME [VAR1=VALUE1 VAR2=VALUE2 ...]" << std::endl;
        return CQL_ERROR;
    }

    std::string template_name = argv[2];
    auto variables = process_template_variables(argc, argv, 3);
    bool force = has_force_flag(argc, argv, 3);

    try {
        cql::TemplateManager manager;
        auto validator = initialize_template_validator(manager);

        // Validate template
        auto validation_result = validator.validate_template(template_name);

        // Handle validation issues
        if (validation_result.has_issues(cql::TemplateValidationLevel::ERROR)) {
            std::cerr << "Warning: Template has validation errors:" << std::endl;
            for (const auto& issue : validation_result.get_issues(cql::TemplateValidationLevel::ERROR)) {
                std::cerr << "  - " << issue.to_string() << std::endl;
            }

            if (!force) {
                std::cerr << "Validation failed. Use --force to ignore errors." << std::endl;
                return CQL_ERROR;
            } else {
                std::cerr << "Proceeding despite validation errors (--force specified)." << std::endl;
            }
        } else if (validation_result.has_issues(cql::TemplateValidationLevel::WARNING)) {
            std::cerr << "Template has validation warnings:" << std::endl;
            for (const auto& issue : validation_result.get_issues(cql::TemplateValidationLevel::WARNING)) {
                std::cerr << "  - " << issue.to_string() << std::endl;
            }
        }

        // Check for missing variables
        std::string template_content = manager.load_template(template_name);
        auto template_vars = cql::TemplateManager::collect_variables(template_content);
        handle_missing_variables(validation_result, template_vars, variables);

        // Instantiate and compile template
        std::string instantiated = manager.instantiate_template(template_name, variables);
        std::string compiled = cql::QueryProcessor::compile(instantiated);

        std::cout << compiled << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error using template: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

/**
 * @brief Display validation results
 *
 * @param result Validation result
 * @param template_name Template name
 */
void display_validation_results(const cql::TemplateValidationResult& result, const std::string& template_name) {
    std::cout << "Validation results for template '" << template_name << "':" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    if (result.has_issues()) {
        std::cout << "Found " << result.count_errors() << " errors, "
                  << result.count_warnings() << " warnings, "
                  << result.count_infos() << " info messages." << std::endl;

        // print errors
        if (result.count_errors() > 0) {
            std::cout << "\nErrors:" << std::endl;
            for (const auto& issue : result.get_issues(cql::TemplateValidationLevel::ERROR)) {
                std::cout << "  - " << issue.to_string() << std::endl;
            }
        }

        // print warnings
        if (result.count_warnings() > 0) {
            std::cout << "\nWarnings:" << std::endl;
            for (const auto& issue : result.get_issues(cql::TemplateValidationLevel::WARNING)) {
                std::cout << "  - " << issue.to_string() << std::endl;
            }
        }

        // print info messages (only if there are no errors or warnings)
        if (result.count_infos() > 0 && result.count_errors() == 0 && result.count_warnings() == 0) {
            std::cout << "\nInfo:" << std::endl;
            for (const auto& issue : result.get_issues(cql::TemplateValidationLevel::INFO)) {
                std::cout << "  - " << issue.to_string() << std::endl;
            }
        }
    } else {
        std::cout << "Template validated successfully with no issues." << std::endl;
    }
}

/**
 * @brief Validate a specific template
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
int handle_validate_command(const int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Template name required" << std::endl;
        std::cerr << "Usage: cql --validate TEMPLATE_NAME" << std::endl;
        return CQL_ERROR;
    }

    const std::string template_name = argv[2];

    try {
        cql::TemplateManager manager;
        auto validator = initialize_template_validator(manager);

        // Validate the template
        const auto result = validator.validate_template(template_name);

        // Display validation results
        display_validation_results(result, template_name);
    } catch (const std::exception& e) {
        std::cerr << "Error validating template: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

/**
 * @brief Validate all templates
 *
 * @param templates_path Path to the templates directory
 * @return int Return code (0 for success, 1 for error)
 */
int handle_validate_all_command(const std::string& templates_path) {
    try {
        cql::TemplateManager manager(templates_path);
        auto validator = initialize_template_validator(manager);
        const auto templates = manager.list_templates();

        if (templates.empty()) {
            std::cout << "No templates found to validate in " << templates_path << std::endl;
            return CQL_NO_ERROR;
        }

        std::cout << "Validating " << templates.size() << " templates from " << templates_path << "..." << std::endl;
        std::cout << "----------------------------" << std::endl;

        int error_count = 0;
        int warning_count = 0;
        int info_count = 0;

        std::vector<std::string> templates_with_errors;
        std::vector<std::string> templates_with_warnings;

        // Validate each template
        for (const auto& tmpl : templates) {
            auto result = validator.validate_template(tmpl);

            // Count issues
            error_count += result.count_errors();
            warning_count += result.count_warnings();
            info_count += result.count_infos();

            // Print progress
            if (result.has_issues(cql::TemplateValidationLevel::ERROR)) {
                templates_with_errors.push_back(tmpl);
                std::cout << "❌ " << tmpl << ": " << result.count_errors() << " errors, "
                          << result.count_warnings() << " warnings" << std::endl;
            } else if (result.has_issues(cql::TemplateValidationLevel::WARNING)) {
                templates_with_warnings.push_back(tmpl);
                std::cout << "⚠️ " << tmpl << ": " << result.count_warnings() << " warnings" << std::endl;
            } else {
                std::cout << "✅ " << tmpl << ": No issues" << std::endl;
            }
        }

        // Print summary
        std::cout << "\nValidation Summary:" << std::endl;
        std::cout << "----------------------------" << std::endl;
        std::cout << "Templates validated: " << templates.size() << std::endl;
        std::cout << "Total issues: " << (error_count + warning_count + info_count) << " ("
                  << error_count << " errors, "
                  << warning_count << " warnings, "
                  << info_count << " info messages)" << std::endl;

        // List templates with errors
        if (!templates_with_errors.empty()) {
            std::cout << "\nTemplates with errors:" << std::endl;
            for (const auto& tmpl : templates_with_errors) {
                std::cout << "  - " << tmpl << std::endl;
            }
            std::cout << "Run 'cql --validate <name>' for details" << std::endl;
        }

        // Print status message
        if (error_count > 0) {
            std::cerr << "Validation found errors." << std::endl;
            return CQL_ERROR;
        } else if (warning_count > 0) {
            std::cout << "Validation successful, but found warnings." << std::endl;
        } else {
            std::cout << "All templates passed validation!" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error validating templates: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

/**
 * @brief Generate documentation for a specific template
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
int handle_docs_command(const int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: template name required" << std::endl;
        std::cerr << "usage: cql --docs TEMPLATE_NAME" << std::endl;
        return CQL_ERROR;
    }

    std::string template_name = argv[2];

    try {
        cql::TemplateManager manager;
        const std::string docs = manager.generate_template_documentation(template_name);
        std::cout << docs << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "error generating template documentation: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

/**
 * @brief Generate documentation for all templates
 *
 * @return int Return code (0 for success, 1 for error)
 */
int handle_docs_all_command() {
    try {
        cql::TemplateManager manager;
        const std::string docs = manager.generate_all_template_documentation();
        std::cout << docs << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "error generating template documentation: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

/**
 * @brief Export documentation to a file
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
int handle_export_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: output path required" << std::endl;
        std::cerr << "usage: cql --export OUTPUT_PATH [FORMAT]" << std::endl;
        return CQL_ERROR;
    }

    std::string output_path = argv[2];
    std::string format = "markdown"; // default format

    // Check if a format is specified
    if (argc > 3) {
        format = argv[3];
    }

    try {
        cql::TemplateManager manager;

        if (manager.export_documentation(output_path, format)) {
            std::cout << "template documentation exported to " << output_path
                      << " in " << format << " format" << std::endl;
        } else {
            std::cerr << "failed to export template documentation" << std::endl;
            return CQL_ERROR;
        }
    } catch (const std::exception& e) {
        std::cerr << "error exporting template documentation: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

/**
 * @brief Process a file with an optional output
 *
 * @param input_file Input file path
 * @param output_file Output file path (optional)
 * @param use_clipboard Copy buffer to clipboard
 * @return int Return code (0 for success, 1 for error)
 */
int handle_file_processing(const std::string& input_file,
    const std::string& output_file,
    const bool use_clipboard = false,
    const bool include_header = false) {
    if (use_clipboard) {
        try {
            Logger::getInstance().log(LogLevel::INFO, "Processing file: ", input_file);
            std::cout << "Processing file: " << input_file << std::endl;

            // Copy to clipboard
            if (const std::string result = cql::QueryProcessor::compile_file(input_file); cql::util::copy_to_clipboard(result)) {
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
    if (!cql::cli::process_file(input_file, output_file, include_header)) {
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

/**
 * @brief Main application entry point
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
/**
 * @brief Process command line arguments to find specific options
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @param option Option to search for
 * @param value Optional pointer to store option value if found
 * @return true if the option was found, false otherwise
 */
bool find_command_line_option(const int argc, char* argv[], const std::string& option, std::string* value = nullptr) {
    for (int ndx = 1; ndx < argc; ++ndx) {
        if (std::string arg = argv[ndx]; arg == option) {
            if (value != nullptr && ndx + 1 < argc) {
                *value = argv[ndx + 1];
            }
            return true;
        }
    }
    return false;
}

/**
 * @brief Find an argument in the command line and remove it along with its value
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @param option Option to find and remove
 * @param value String to store the option value if found
 * @param new_argc New argument count after removal
 * @param new_argv Smart pointer to new argument array after removal
 * @return true if option was found and removed
 */
bool find_and_remove_option(const int argc, char* argv[], const std::string& option,
                           std::string& value, int& new_argc, std::unique_ptr<char*[]>& new_argv) {
    // Allocate a new array for modified arguments using a smart pointer
    new_argv = std::make_unique<char*[]>(argc);
    new_argc = 0;
    
    bool found = false;
    bool skip_next = false;
    
    // Copy program name
    new_argv[new_argc++] = argv[0];
    
    // Process remaining arguments
    for (int ndx = 1; ndx < argc; ++ndx) {
        if (skip_next) {
            skip_next = false;
            continue;
        }
        
        if (std::string(argv[ndx]) == option && ndx + 1 < argc) {
            value = argv[ndx + 1];
            found = true;
            skip_next = true;
        } else {
            new_argv[new_argc++] = argv[ndx];
        }
    }
    return found;
}

int main(const int argc, char* argv[]) {
    // Default debug level
    auto debug_level = LogLevel::DEBUG;
    
    // Check for debug level in arguments
    std::string debug_level_str;
    int new_argc;
    std::unique_ptr<char*[]> new_argv;
    
    // We have a little bit of a chicken-and-egg problem here,
    // I need to set the logging level before processing any args,
    // thus we need to find and remove the debug level option if present.
    // This is the best way I came up with; maybe there is
    // something better?
    const bool has_debug_option = find_and_remove_option(argc, argv, "--debug-level",
                                                  debug_level_str, new_argc, new_argv);
    
    if (has_debug_option) {
        debug_level = string_to_log_level(debug_level_str);
    }
    
    // Initialize logger
    auto& logger = Logger::getInstance();
    
    // Set the logging level
    logger.setToLevelEnabled(debug_level);
    
    // Check if headers should be included (default is clean output)
    bool include_headers = find_command_line_option(has_debug_option ? new_argc : argc, 
                                                    has_debug_option ? new_argv.get() : argv, 
                                                    "--include-header");
    
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

#if 0
    logger.log(LogLevel::INFO, "This is a INFO message");
    logger.log(LogLevel::NORMAL, "This is a NORMAL message");
    logger.log(LogLevel::DEBUG, "This is a DEBUG message");
    logger.log(LogLevel::ERROR, "This is a ERROR message");
    logger.log(LogLevel::CRITICAL, "This is a CRITICAL message");
#endif

    // If the only argument was --debug-level, show help
    if (has_debug_option && new_argc == 1) {
        std::cout << "Log level set to: " << debug_level_str << std::endl;
        std::cout << "No other arguments provided." << std::endl;
        print_help();
        return CQL_NO_ERROR;
    }

    try {
        // Handle a case with no arguments
        if (new_argc <= 1) {
            if (!has_debug_option) {
                std::cout << "No arguments provided." << std::endl;
                print_help();
                std::cout << "\nTo run the application with a file, use: cql input.llm output.txt" << std::endl;
            }
            return CQL_NO_ERROR;
        }

        // Parse the first argument from our modified argument array
        const std::string arg1 = new_argv[1];
        if (include_headers) {
            std::cout << "Received argument: " << arg1 << std::endl;
        }

        // Dispatch to the appropriate handler based on the first argument
        if (arg1 == "--help" || arg1 == "-h") {
            print_help();
        } else if (arg1 == "--interactive" || arg1 == "-i") {
            cql::cli::run_interactive();
        } else if (arg1 == "--submit") {
            return handle_submit_command(new_argc, new_argv.get());
        } else if (arg1 == "--templates" || arg1 == "-l") {
            list_templates();
        } else if (arg1 == "--template" || arg1 == "-T") {
            return handle_template_command(new_argc, new_argv.get());
        } else if (arg1 == "--validate") {
            return handle_validate_command(new_argc, new_argv.get());
        } else if (arg1 == "--validate-all") {
            if (new_argc < 3) {
                std::cerr << "Error: Path required for --validate-all" << std::endl;
                std::cerr << "Usage: cql --validate-all PATH" << std::endl;
                return CQL_ERROR;
            }
            return handle_validate_all_command(new_argv[2]);
        } else if (arg1 == "--docs") {
            return handle_docs_command(new_argc, new_argv.get());
        } else if (arg1 == "--docs-all") {
            return handle_docs_all_command();
        } else if (arg1 == "--export") {
            return handle_export_command(new_argc, new_argv.get());
        } else if (arg1 == "--clipboard" || arg1 == "-c") {
            if (new_argc < 3) {
                std::cerr << "Error: Input file required when using --clipboard option" << std::endl;
                return CQL_ERROR;
            }
            return handle_file_processing(new_argv[2], "", true);
        } else if (arg1.substr(0, 2) == "--") {
            // Unknown option starting with "--"
            std::cerr << "Error: Unknown option: " << arg1 << std::endl;
            std::cerr << "Available options:" << std::endl;
            print_help();
            return CQL_ERROR;
        } else {
            // Assume it's an input file
            std::string output_file;
            bool use_clipboard = false;

            // Check if any of the arguments is --clipboard/-c
            for (int i = 2; i < new_argc; ++i) {
                if (std::string arg = new_argv[i]; arg == "--clipboard" || arg == "-c") {
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
