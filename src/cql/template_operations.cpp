// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/template_operations.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include <iostream>
#include <ranges>

namespace cql {

TemplateOperations::TemplateOperations() = default;

void TemplateOperations::list_templates() {
    const TemplateManager manager;

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

TemplateValidator TemplateOperations::initialize_template_validator(const TemplateManager& manager) {
    TemplateValidator validator(manager);
    for (const auto schema = TemplateValidatorSchema::create_default_schema();
        const auto &rule: schema.get_validation_rules() | std::views::values) {
        validator.add_validation_rule(rule);
    }
    return validator;
}

std::map<std::string, std::string> TemplateOperations::process_template_variables(int argc, char* argv[], int start_index) {
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

bool TemplateOperations::has_force_flag(int argc, char* argv[], int start_index) {
    for (int ndx = start_index; ndx < argc; ndx++) {
        if (std::string arg = argv[ndx]; arg == "--force" || arg == "-f") {
            return true;
        }
    }
    return false;
}

std::vector<std::string> TemplateOperations::handle_missing_variables(
    const TemplateValidationResult& validation_result,
    const std::map<std::string, std::string>& template_vars,
    const std::map<std::string, std::string>& variables) {
    
    std::vector<std::string> missing_vars;

    // Extract variables from validation issues
    for (const auto var_issues = validation_result.get_issues(TemplateValidationLevel::WARNING); const auto& issue : var_issues) {
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

int TemplateOperations::handle_template_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Template name required" << std::endl;
        std::cerr << "Usage: cql --template TEMPLATE_NAME [VAR1=VALUE1 VAR2=VALUE2 ...]" << std::endl;
        return CQL_ERROR;
    }

    std::string template_name = argv[2];
    auto variables = process_template_variables(argc, argv, 3);
    bool force = has_force_flag(argc, argv, 3);

    try {
        TemplateManager manager;
        auto validator = initialize_template_validator(manager);

        // Validate template
        auto validation_result = validator.validate_template(template_name);

        // Handle validation issues
        if (validation_result.has_issues(TemplateValidationLevel::ERROR)) {
            std::cerr << "Warning: Template has validation errors:" << std::endl;
            for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                std::cerr << "  - " << issue.to_string() << std::endl;
            }

            if (!force) {
                std::cerr << "Validation failed. Use --force to ignore errors." << std::endl;
                return CQL_ERROR;
            } else {
                std::cerr << "Proceeding despite validation errors (--force specified)." << std::endl;
            }
        } else if (validation_result.has_issues(TemplateValidationLevel::WARNING)) {
            std::cerr << "Template has validation warnings:" << std::endl;
            for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::WARNING)) {
                std::cerr << "  - " << issue.to_string() << std::endl;
            }
        }

        // Check for missing variables
        std::string template_content = manager.load_template(template_name);
        auto template_vars = TemplateManager::collect_variables(template_content);
        handle_missing_variables(validation_result, template_vars, variables);

        // Instantiate and compile template
        std::string instantiated = manager.instantiate_template(template_name, variables);
        std::string compiled = QueryProcessor::compile(instantiated);

        std::cout << compiled << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error using template: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

void TemplateOperations::display_validation_results(const TemplateValidationResult& result, const std::string& template_name) {
    std::cout << "Validation results for template '" << template_name << "':" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    if (result.has_issues()) {
        std::cout << "Found " << result.count_errors() << " errors, "
                  << result.count_warnings() << " warnings, "
                  << result.count_infos() << " info messages." << std::endl;

        // print errors
        if (result.count_errors() > 0) {
            std::cout << "\nErrors:" << std::endl;
            for (const auto& issue : result.get_issues(TemplateValidationLevel::ERROR)) {
                std::cout << "  - " << issue.to_string() << std::endl;
            }
        }

        // print warnings
        if (result.count_warnings() > 0) {
            std::cout << "\nWarnings:" << std::endl;
            for (const auto& issue : result.get_issues(TemplateValidationLevel::WARNING)) {
                std::cout << "  - " << issue.to_string() << std::endl;
            }
        }

        // print info messages (only if there are no errors or warnings)
        if (result.count_infos() > 0 && result.count_errors() == 0 && result.count_warnings() == 0) {
            std::cout << "\nInfo:" << std::endl;
            for (const auto& issue : result.get_issues(TemplateValidationLevel::INFO)) {
                std::cout << "  - " << issue.to_string() << std::endl;
            }
        }
    } else {
        std::cout << "Template validated successfully with no issues." << std::endl;
    }
}

int TemplateOperations::handle_validate_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Error: Template name required" << std::endl;
        std::cerr << "Usage: cql --validate TEMPLATE_NAME" << std::endl;
        return CQL_ERROR;
    }

    const std::string template_name = argv[2];

    try {
        TemplateManager manager;
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

int TemplateOperations::handle_validate_all_command(const std::string& templates_path) {
    try {
        TemplateManager manager(templates_path);
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
        int total_templates = 0;

        for (const auto& template_name : templates) {
            total_templates++;
            std::cout << "\n[" << total_templates << "/" << templates.size() << "] Validating: " << template_name << std::endl;

            try {
                const auto result = validator.validate_template(template_name);

                error_count += result.count_errors();
                warning_count += result.count_warnings();
                info_count += result.count_infos();

                if (result.has_issues()) {
                    display_validation_results(result, template_name);
                } else {
                    std::cout << "✓ Template validated successfully" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "✗ Error validating template: " << e.what() << std::endl;
                error_count++;
            }
        }

        // Summary
        std::cout << "\n=============================" << std::endl;
        std::cout << "Validation Summary:" << std::endl;
        std::cout << "Templates processed: " << total_templates << std::endl;
        std::cout << "Total errors: " << error_count << std::endl;
        std::cout << "Total warnings: " << warning_count << std::endl;
        std::cout << "Total info: " << info_count << std::endl;

        if (error_count > 0) {
            std::cout << "\n⚠️  Some templates have validation errors!" << std::endl;
            return CQL_ERROR;
        } else if (warning_count > 0) {
            std::cout << "\n⚠️  Some templates have validation warnings." << std::endl;
        } else {
            std::cout << "\n✅ All templates validated successfully!" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during validation: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

} // namespace cql