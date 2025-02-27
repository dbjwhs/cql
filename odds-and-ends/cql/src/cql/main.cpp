// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_validator.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include "../../../headers/project_utils.hpp"

int main(int argc, char* argv[]) {
    // Initialize logger
    auto& logger = Logger::getInstance();
    std::cout << "Starting CQL Compiler v1.0..." << std::endl;
    logger.log(LogLevel::INFO, "Claude Query Language (CQL) Compiler v1.0");

    try {
        std::cout << "Parsing command line arguments..." << std::endl;
        // Parse command line arguments
        if (argc > 1) {
            std::string arg1 = argv[1];
            std::cout << "Received argument: " << arg1 << std::endl;

            if (arg1 == "--help" || arg1 == "-h") {
                // Show help information
                std::cout << "Claude Query Language (CQL) Compiler v1.0\n"
                          << "Usage: cql [OPTIONS] [INPUT_FILE] [OUTPUT_FILE]\n\n"
                          << "Options:\n"
                          << "  --help, -h              Show this help information\n"
                          << "  --test, -t              Run the test suite\n"
                          << "  --examples, -e          Show example queries\n"
                          << "  --interactive, -i       Run in interactive mode\n"
                          << "  --copyright             Show copyright example\n"
                          << "  --templates, -l         List all available templates\n"
                          << "  --template NAME, -T     Use a specific template\n"
                          << "  --validate NAME         Validate a specific template\n"
                          << "  --validate-all          Validate all templates\n\n"
                          << "If INPUT_FILE is provided, it will be processed as a CQL query.\n"
                          << "If OUTPUT_FILE is also provided, the compiled query will be written to it.\n";
            } else if (arg1 == "--test" || arg1 == "-t") {
                // Run the test suite
                cql::test::run_tests();
            } else if (arg1 == "--examples" || arg1 == "-e") {
                // Show example queries
                cql::test::query_examples();
            } else if (arg1 == "--interactive" || arg1 == "-i") {
                // Run in interactive mode
                cql::cli::run_cli();
            } else if (arg1 == "--copyright") {
                // Show an example of copyright usage
                std::string copyright_example =
                    "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                    "@language \"C++\"\n"
                    "@description \"implement a thread-safe queue\"\n";

                logger.log(LogLevel::INFO, "Copyright Example DSL:\n", copyright_example);
                
                std::string result = cql::QueryProcessor::compile(copyright_example);
                logger.log(LogLevel::INFO, "\n=== Compiled Query with Copyright ===\n\n", 
                          result, "\n===================");
            } else if (arg1 == "--templates" || arg1 == "-l") {
                // List all templates
                cql::TemplateManager manager;
                auto templates = manager.list_templates();
                
                if (templates.empty()) {
                    std::cout << "No templates found in " << manager.get_templates_directory() << std::endl;
                } else {
                    std::cout << "Available templates:" << std::endl;
                    for (const auto& tmpl : templates) {
                        // Get template metadata for more info
                        try {
                            auto metadata = manager.get_template_metadata(tmpl);
                            std::cout << "  " << tmpl << " - " << metadata.description << std::endl;
                        } catch (const std::exception&) {
                            // If we can't get metadata, just show the name
                            std::cout << "  " << tmpl << std::endl;
                        }
                    }
                }
            } else if (arg1 == "--template" || arg1 == "-T") {
                // Use a specific template
                if (argc < 3) {
                    std::cerr << "Error: Template name required" << std::endl;
                    std::cerr << "Usage: cql --template TEMPLATE_NAME [VAR1=VALUE1 VAR2=VALUE2 ...]" << std::endl;
                    return 1;
                }
                
                std::string template_name = argv[2];
                std::map<std::string, std::string> variables;
                
                // Process variable assignments (VAR=VALUE format)
                for (int i = 3; i < argc; i++) {
                    std::string arg = argv[i];
                    size_t pos = arg.find('=');
                    if (pos != std::string::npos) {
                        std::string name = arg.substr(0, pos);
                        std::string value = arg.substr(pos + 1);
                        variables[name] = value;
                    }
                }
                
                try {
                    cql::TemplateManager manager;
                    std::string instantiated = manager.instantiate_template(template_name, variables);
                    std::string compiled = cql::QueryProcessor::compile(instantiated);
                    
                    std::cout << compiled << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error using template: " << e.what() << std::endl;
                    return 1;
                }
            } else if (arg1 == "--validate") {
                // Validate a specific template
                if (argc < 3) {
                    std::cerr << "Error: Template name required" << std::endl;
                    std::cerr << "Usage: cql --validate TEMPLATE_NAME" << std::endl;
                    return 1;
                }
                
                std::string template_name = argv[2];
                
                try {
                    // Create template manager and validator
                    cql::TemplateManager manager;
                    cql::TemplateValidator validator(manager);
                    
                    // Add schema validation rules
                    auto schema = cql::TemplateValidatorSchema::create_default_schema();
                    for (const auto& [name, rule] : schema.get_validation_rules()) {
                        validator.add_validation_rule(rule);
                    }
                    
                    // Validate the template
                    auto result = validator.validate_template(template_name);
                    
                    // Output validation results
                    std::cout << "Validation results for template '" << template_name << "':" << std::endl;
                    std::cout << "------------------------------------------" << std::endl;
                    
                    if (result.has_issues()) {
                        std::cout << "Found " << result.count_errors() << " errors, "
                                  << result.count_warnings() << " warnings, "
                                  << result.count_infos() << " info messages." << std::endl;
                        
                        // Print errors
                        if (result.count_errors() > 0) {
                            std::cout << "\nErrors:" << std::endl;
                            for (const auto& issue : result.get_issues(cql::TemplateValidationLevel::ERROR)) {
                                std::cout << "  - " << issue.to_string() << std::endl;
                            }
                        }
                        
                        // Print warnings
                        if (result.count_warnings() > 0) {
                            std::cout << "\nWarnings:" << std::endl;
                            for (const auto& issue : result.get_issues(cql::TemplateValidationLevel::WARNING)) {
                                std::cout << "  - " << issue.to_string() << std::endl;
                            }
                        }
                        
                        // Print info messages (only if there are no errors or warnings)
                        if (result.count_infos() > 0 && result.count_errors() == 0 && result.count_warnings() == 0) {
                            std::cout << "\nInfo:" << std::endl;
                            for (const auto& issue : result.get_issues(cql::TemplateValidationLevel::INFO)) {
                                std::cout << "  - " << issue.to_string() << std::endl;
                            }
                        }
                    } else {
                        std::cout << "Template validated successfully with no issues." << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error validating template: " << e.what() << std::endl;
                    return 1;
                }
            } else if (arg1 == "--validate-all") {
                // Validate all templates
                try {
                    // Create template manager and validator
                    cql::TemplateManager manager;
                    cql::TemplateValidator validator(manager);
                    
                    // Add schema validation rules
                    auto schema = cql::TemplateValidatorSchema::create_default_schema();
                    for (const auto& [name, rule] : schema.get_validation_rules()) {
                        validator.add_validation_rule(rule);
                    }
                    
                    // Get list of templates
                    auto templates = manager.list_templates();
                    
                    if (templates.empty()) {
                        std::cout << "No templates found to validate." << std::endl;
                    } else {
                        std::cout << "Validating " << templates.size() << " templates..." << std::endl;
                        std::cout << "----------------------------" << std::endl;
                        
                        int error_count = 0;
                        int warning_count = 0;
                        int info_count = 0;
                        
                        // Keep track of templates with issues
                        std::vector<std::string> templates_with_errors;
                        std::vector<std::string> templates_with_warnings;
                        
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
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error validating templates: " << e.what() << std::endl;
                    return 1;
                }
            } else {
                // Assume it's an input file
                std::string output_file;
                if (argc > 2) {
                    output_file = argv[2];
                }

                if (!cql::cli::process_file(arg1, output_file)) {
                    return 1;
                }
            }
        } else {
            // No arguments, run comprehensive tests and examples
            std::cout << "Running in default mode - tests and examples" << std::endl;
            logger.log(LogLevel::INFO, "Running in default mode - tests and examples");
            std::cout << "Running tests..." << std::endl;
            cql::test::run_tests();
            std::cout << "Running query examples..." << std::endl;
            cql::test::query_examples();

            // Example query with Phase 2 features
            std::string query =
                "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                "@language \"C++\"\n"
                "@description \"implement a thread-safe queue with a maximum size\"\n"
                "@context \"Using C++20 features and RAII principles\"\n"
                "@architecture \"Producer-consumer pattern with monitoring\"\n"
                "@constraint \"Thread-safe for concurrent access\"\n"
                "@security \"Prevent data races and deadlocks\"\n"
                "@complexity \"O(1) for push and pop operations\"\n"
                "@variable \"max_size\" \"1000\"\n"
                "@example \"Basic Usage\" \"\n"
                "ThreadSafeQueue<int> queue(${max_size});\n"
                "queue.push(42);\n"
                "auto value = queue.pop();\n"
                "\"\n"
                "@test \"Test concurrent push operations\"\n"
                "@test \"Test concurrent pop operations\"\n"
                "@test \"Test boundary conditions\"\n";

            std::cout << "\nDefault example:" << std::endl;
            std::cout << "Input query:\n" << query << std::endl;
            logger.log(LogLevel::INFO, "\nDefault example:");
            logger.log(LogLevel::INFO, "Input query:\n", query);

            std::cout << "Compiling default example..." << std::endl;
            std::string result = cql::QueryProcessor::compile(query);
            std::cout << "\n=== Compiled Query ===\n\n" << result << "\n===================" << std::endl;
            logger.log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
        }
    } catch (const std::exception& e) {
        logger.log(LogLevel::ERROR, "Fatal error: ", e.what());
        return 1;
    }
    
    return 0;
}