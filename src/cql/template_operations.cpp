// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/template_operations.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include "../../include/cql/error_context.hpp"
#include "../../include/cql/user_output_manager.hpp"
#include <iostream>
#include <ranges>

namespace cql {

TemplateOperations::TemplateOperations() = default;

void TemplateOperations::list_templates() {
    const TemplateManager manager;

    if (const auto templates = manager.list_templates(); templates.empty()) {
        UserOutputManager::info("No templates found in ", manager.get_templates_directory());
    } else {
        UserOutputManager::info("Available templates:");
        for (const auto& tmpl : templates) {
            // get template metadata for more info
            try {
                auto metadata = manager.get_template_metadata(tmpl);
                UserOutputManager::info("  ", tmpl, " - ", metadata.description);
            } catch (const std::exception& e) {
                // Preserve error context but don't fail the entire operation
                auto contextual_error = ErrorContextBuilder::from(e)
                    .operation("retrieving template metadata")
                    .template_name(tmpl)
                    .at(__FILE__ ":" + std::to_string(__LINE__))
                    .build();

                // Log the error for debugging but continue with template listing
                error_context_utils::log_contextual_exception(contextual_error);

                // Show template name without metadata
                UserOutputManager::info("  ", tmpl, " (metadata unavailable)");
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
        UserOutputManager::warning("The following variables are referenced but not provided:");
        for (const auto& var : missing_vars) {
            UserOutputManager::warning("  - ", var);
        }
        UserOutputManager::warning("These will appear as '${", missing_vars[0], "}' in the output.");
    }

    return missing_vars;
}

int TemplateOperations::handle_template_command(int argc, char* argv[]) {
    if (argc < 3) {
        UserOutputManager::error("Template name required");
        UserOutputManager::info("Usage: cql --template TEMPLATE_NAME [VAR1=VALUE1 VAR2=VALUE2 ...]");
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
            UserOutputManager::warning("Template has validation errors:");
            for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                UserOutputManager::warning("  - ", issue.to_string());
            }

            if (!force) {
                UserOutputManager::error("Validation failed. Use --force to ignore errors.");
                return CQL_ERROR;
            } else {
                UserOutputManager::warning("Proceeding despite validation errors (--force specified).");
            }
        } else if (validation_result.has_issues(TemplateValidationLevel::WARNING)) {
            UserOutputManager::warning("Template has validation warnings:");
            for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::WARNING)) {
                UserOutputManager::warning("  - ", issue.to_string());
            }
        }

        // Check for missing variables
        std::string template_content = manager.load_template(template_name);
        auto template_vars = TemplateManager::collect_variables(template_content);
        [[maybe_unused]] auto missing_vars = handle_missing_variables(validation_result, template_vars, variables);

        // Instantiate and compile template
        std::string instantiated = manager.instantiate_template(template_name, variables);
        std::string compiled = QueryProcessor::compile(instantiated);

        UserOutputManager::info(compiled);
    } catch (const std::exception& e) {
        UserOutputManager::error("Error using template: ", e.what());
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

void TemplateOperations::display_validation_results(const TemplateValidationResult& result, const std::string& template_name) {
    UserOutputManager::info("Validation results for template '", template_name, "':");
    UserOutputManager::info("------------------------------------------");

    if (result.has_issues()) {
        UserOutputManager::info("Found ", result.count_errors(), " errors, ",
                  result.count_warnings(), " warnings, ",
                  result.count_infos(), " info messages.");

        // print errors
        if (result.count_errors() > 0) {
            UserOutputManager::info("\nErrors:");
            for (const auto& issue : result.get_issues(TemplateValidationLevel::ERROR)) {
                UserOutputManager::error("  - ", issue.to_string());
            }
        }

        // print warnings
        if (result.count_warnings() > 0) {
            UserOutputManager::info("\nWarnings:");
            for (const auto& issue : result.get_issues(TemplateValidationLevel::WARNING)) {
                UserOutputManager::warning("  - ", issue.to_string());
            }
        }

        // print info messages (only if there are no errors or warnings)
        if (result.count_infos() > 0 && result.count_errors() == 0 && result.count_warnings() == 0) {
            UserOutputManager::info("\nInfo:");
            for (const auto& issue : result.get_issues(TemplateValidationLevel::INFO)) {
                UserOutputManager::info("  - ", issue.to_string());
            }
        }
    } else {
        UserOutputManager::success("Template validated successfully with no issues.");
    }
}

int TemplateOperations::handle_validate_command(int argc, char* argv[]) {
    if (argc < 3) {
        UserOutputManager::error("Template name required");
        UserOutputManager::info("Usage: cql --validate TEMPLATE_NAME");
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
        UserOutputManager::error("Error validating template: ", e.what());
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
            UserOutputManager::info("No templates found to validate in ", templates_path);
            return CQL_NO_ERROR;
        }

        UserOutputManager::info("Validating ", templates.size(), " templates from ", templates_path, "...");
        UserOutputManager::info("----------------------------");

        int error_count = 0;
        int warning_count = 0;
        int info_count = 0;
        int total_templates = 0;

        for (const auto& template_name : templates) {
            total_templates++;
            UserOutputManager::info("\n[", total_templates, "/", templates.size(), "] Validating: ", template_name);

            try {
                const auto result = validator.validate_template(template_name);

                error_count += result.count_errors();
                warning_count += result.count_warnings();
                info_count += result.count_infos();

                if (result.has_issues()) {
                    display_validation_results(result, template_name);
                } else {
                    UserOutputManager::success("✓ Template validated successfully");
                }
            } catch (const std::exception& e) {
                UserOutputManager::error("✗ Error validating template: ", e.what());
                error_count++;
            }
        }

        // Summary
        UserOutputManager::info("\n=============================");
        UserOutputManager::info("Validation Summary:");
        UserOutputManager::info("Templates processed: ", total_templates);
        UserOutputManager::info("Total errors: ", error_count);
        UserOutputManager::info("Total warnings: ", warning_count);
        UserOutputManager::info("Total info: ", info_count);

        if (error_count > 0) {
            UserOutputManager::warning("\n⚠️  Some templates have validation errors!");
            return CQL_ERROR;
        } else if (warning_count > 0) {
            UserOutputManager::warning("\n⚠️  Some templates have validation warnings.");
        } else {
            UserOutputManager::success("\n✅ All templates validated successfully!");
        }

    } catch (const std::exception& e) {
        UserOutputManager::error("Error during validation: ", e.what());
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

} // namespace cql
