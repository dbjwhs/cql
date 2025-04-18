// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_validator.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include "../../include/cql/api_client.hpp"

// note file exists in the cpp-snippets repo, you will need to check this out and have it and cql share the
// same root directory
#include "../../cpp-snippets/headers/project_utils.hpp"

// Forward declarations
void print_help();
void show_copyright_example();
void list_templates();
cql::TemplateValidator initialize_template_validator(const cql::TemplateManager& manager);
std::map<std::string, std::string> process_template_variables(int argc, char* argv[], int start_index);
bool has_force_flag(int argc, char* argv[], int start_index);
int handle_submit_command(int argc, char* argv[]);
int handle_template_command(int argc, char* argv[]);
int handle_validate_command(int argc, char* argv[]);
int handle_validate_all_command();
int handle_docs_command(int argc, char* argv[]);
int handle_docs_all_command();
int handle_export_command(int argc, char* argv[]);
int handle_file_processing(const std::string& input_file, const std::string& output_file);

/**
 * @brief Print the help message with usage information
 */
void print_help() {
    std::cout << "Claude Query Language (CQL) Compiler v" << CQL_VERSION_STRING << " (" << CQL_BUILD_TIMESTAMP << ")\n"
              << "Usage: cql [OPTIONS] [INPUT_FILE] [OUTPUT_FILE]\n\n"
              << "Options:\n"
              << "  --help, -h              Show this help information\n"
              << "  --examples, -e          Show example queries\n"
              << "  --interactive, -i       Run in interactive mode\n"
              << "  --copyright             Show copyright example\n"
              << "  --templates, -l         List all available templates\n"
              << "  --template NAME, -T     Use a specific template\n"
              << "  --template NAME --force Use template even with validation errors\n"
              << "  --validate NAME         Validate a specific template\n"
              << "  --validate-all          Validate all templates\n"
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
              << "If OUTPUT_FILE is also provided, the compiled query will be written to it.\n";
}

/**
 * @brief Display the copyright example
 */
void show_copyright_example() {
    auto& logger = Logger::getInstance();
    std::string copyright_example =
        "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
        "@language \"C++\"\n"
        "@description \"implement a thread-safe queue\"\n";

    logger.log(LogLevel::INFO, "Copyright Example DSL:\n", copyright_example);
    
    std::string result = cql::QueryProcessor::compile(copyright_example);
    logger.log(LogLevel::INFO, "\n=== Compiled Query with Copyright ===\n\n", 
              result, "\n===================");
}

/**
 * @brief List all available templates
 */
void list_templates() {
    cql::TemplateManager manager;
    auto templates = manager.list_templates();
    
    if (templates.empty()) {
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
std::map<std::string, std::string> process_template_variables(int argc, char* argv[], int start_index) {
    std::map<std::string, std::string> variables;
    
    for (int ndx = start_index; ndx < argc; ndx++) {
        std::string arg = argv[ndx];
        if (const size_t pos = arg.find('='); pos != std::string::npos) {
            std::string name = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);
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
bool has_force_flag(int argc, char* argv[], int start_index) {
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
int handle_submit_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Input file required for --submit" << std::endl;
        std::cerr << "Usage: cql --submit INPUT_FILE [options]" << std::endl;
        return 1;
    }
    
    std::string input_file = argv[2];
    std::string output_dir;
    std::string model;
    bool overwrite = false;
    bool create_dirs = false;
    bool no_save = false;
    
    // Parse additional options
    for (int ndx = 3; ndx < argc; ndx++) {
        std::string arg = argv[ndx];
        
        if (arg == "--model" && ndx + 1 < argc) {
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
        return 1;
    }
    return 0;
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
        if (issue.get_variable_name().has_value() && 
            issue.to_string().find("not declared") != std::string::npos) {
            std::string var_name = issue.get_variable_name().value();
            if (!variables.contains(var_name) && !template_vars.contains(var_name)) {
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
        return 1;
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
                return 1;
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
        auto template_vars = manager.collect_variables(template_content);
        handle_missing_variables(validation_result, template_vars, variables);
        
        // Instantiate and compile template
        std::string instantiated = manager.instantiate_template(template_name, variables);
        std::string compiled = cql::QueryProcessor::compile(instantiated);
        
        std::cout << compiled << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error using template: " << e.what() << std::endl;
        return 1;
    }
    return 0;
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
int handle_validate_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Template name required" << std::endl;
        std::cerr << "Usage: cql --validate TEMPLATE_NAME" << std::endl;
        return 1;
    }
    
    std::string template_name = argv[2];
    
    try {
        cql::TemplateManager manager;
        auto validator = initialize_template_validator(manager);
        
        // Validate the template
        auto result = validator.validate_template(template_name);
        
        // Display validation results
        display_validation_results(result, template_name);
    } catch (const std::exception& e) {
        std::cerr << "Error validating template: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

/**
 * @brief Validate all templates
 * 
 * @return int Return code (0 for success, 1 for error)
 */
int handle_validate_all_command() {
    try {
        cql::TemplateManager manager;
        auto validator = initialize_template_validator(manager);
        auto templates = manager.list_templates();
        
        if (templates.empty()) {
            std::cout << "No templates found to validate." << std::endl;
            return 0;
        }
        
        std::cout << "Validating " << templates.size() << " templates..." << std::endl;
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
            return 1;
        } else if (warning_count > 0) {
            std::cout << "Validation successful, but found warnings." << std::endl;
        } else {
            std::cout << "All templates passed validation!" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error validating templates: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

/**
 * @brief Generate documentation for a specific template
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
int handle_docs_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: template name required" << std::endl;
        std::cerr << "usage: cql --docs TEMPLATE_NAME" << std::endl;
        return 1;
    }
    
    std::string template_name = argv[2];
    
    try {
        cql::TemplateManager manager;
        std::string docs = manager.generate_template_documentation(template_name);
        std::cout << docs << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "error generating template documentation: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

/**
 * @brief Generate documentation for all templates
 * 
 * @return int Return code (0 for success, 1 for error)
 */
int handle_docs_all_command() {
    try {
        cql::TemplateManager manager;
        std::string docs = manager.generate_all_template_documentation();
        std::cout << docs << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "error generating template documentation: " << e.what() << std::endl;
        return 1;
    }
    return 0;
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
        return 1;
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
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "error exporting template documentation: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

/**
 * @brief Process a file with optional output
 * 
 * @param input_file Input file path
 * @param output_file Output file path (optional)
 * @return int Return code (0 for success, 1 for error)
 */
int handle_file_processing(const std::string& input_file, const std::string& output_file) {
    if (!cql::cli::process_file(input_file, output_file)) {
        return 1;
    }
    return 0;
}

/**
 * @brief Main application entry point
 * 
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
int main(int argc, char* argv[]) {
    // Initialize logger
    auto& logger = Logger::getInstance();
    std::cout << "Starting CQL Compiler v" << CQL_VERSION_STRING << " (" << CQL_BUILD_TIMESTAMP << ")..." << std::endl;
    logger.log(LogLevel::INFO, "Claude Query Language (CQL) Compiler v", CQL_VERSION_STRING, " (", CQL_BUILD_TIMESTAMP, ")");

    try {
        std::cout << "Parsing command line arguments..." << std::endl;
        
        // Handle case with no arguments
        if (argc <= 1) {
            std::cout << "No arguments provided. Please use --help to see available options." << std::endl;
            print_help();
            std::cout << "\nTo run the application with a file, use: cql input.llm output.txt" << std::endl;
            return 0;
        }
        
        // Parse first argument
        std::string arg1 = argv[1];
        std::cout << "Received argument: " << arg1 << std::endl;

        // Dispatch to appropriate handler based on first argument
        if (arg1 == "--help" || arg1 == "-h") {
            print_help();
        } else if (arg1 == "--test" || arg1 == "-t" || arg1 == "--gtest" || arg1 == "-g") {
            std::cout << "Testing functionality has been removed from the main application." << std::endl;
            std::cout << "Please use the dedicated test executable instead." << std::endl;
            std::cout << "For example: ./build/cql_test" << std::endl;
            return 0;
        } else if (arg1 == "--examples" || arg1 == "-e") {
            auto result = cql::test::query_examples();
            if (!result.passed()) {
                std::cerr << "\nError running examples: " << result.get_error_message() << std::endl;
                return 1;
            }
        } else if (arg1 == "--interactive" || arg1 == "-i") {
            cql::cli::run_interactive();
        } else if (arg1 == "--copyright") {
            show_copyright_example();
        } else if (arg1 == "--submit") {
            return handle_submit_command(argc, argv);
        } else if (arg1 == "--templates" || arg1 == "-l") {
            list_templates();
        } else if (arg1 == "--template" || arg1 == "-T") {
            return handle_template_command(argc, argv);
        } else if (arg1 == "--validate") {
            return handle_validate_command(argc, argv);
        } else if (arg1 == "--validate-all") {
            return handle_validate_all_command();
        } else if (arg1 == "--docs") {
            return handle_docs_command(argc, argv);
        } else if (arg1 == "--docs-all") {
            return handle_docs_all_command();
        } else if (arg1 == "--export") {
            return handle_export_command(argc, argv);
        } else if (arg1.substr(0, 2) == "--") {
            // Unknown option starting with "--"
            std::cerr << "Error: Unknown option: " << arg1 << std::endl;
            std::cerr << "Available options:" << std::endl;
            print_help();
            return 1;
        } else {
            // Assume it's an input file
            std::string output_file;
            if (argc > 2) {
                output_file = argv[2];
            }
            return handle_file_processing(arg1, output_file);
        }
    } catch (const std::exception& e) {
        logger.log(LogLevel::ERROR, "Fatal error: ", e.what());
        return 1;
    }
    
    return 0;
}
