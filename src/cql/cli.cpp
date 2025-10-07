// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <regex>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_manager.hpp"
#include "../../include/cql/template_validator.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include "../../include/cql/api_client.hpp"
#include "../../include/cql/response_processor.hpp"
#include "../../include/cql/project_utils.hpp"
#include "../../include/cql/user_output_manager.hpp"

namespace cql::cli {

// Helper functions for interactive mode

// Initialize template validator with the default schema
TemplateValidator initialize_template_validator(const TemplateManager& template_manager) {
    TemplateValidator template_validator(template_manager);

    // add schema validation rules
    for (const auto schema = TemplateValidatorSchema::create_default_schema();
         const auto &rule: schema.get_validation_rules() | std::views::values) {
        template_validator.add_validation_rule(rule);
    }

    return template_validator;
}

// Display help menu
void display_help_menu() {
    UserOutputManager::info(
        "Commands:\n"
        "  help                    - Show this help\n"
        "  exit/quit               - Exit the program\n"
        "  clear                   - Clear the current query\n"
        "  show                    - Show the current query\n"
        "  compile                 - Compile the current query\n"
        "  load FILE               - Load query from file\n"
        "  save FILE               - Save compiled query to file\n"
        "\n"
        "Template Commands:\n"
        "  templates               - List all available templates\n"
        "  template save NAME      - Save current query as a template\n"
        "  template load NAME      - Load a template\n"
        "  template info NAME      - Show info about a template\n"
        "  template delete NAME    - Delete a template\n"
        "  template vars NAME      - List variables in a template\n"
        "  template setvar NAME=VAL - Set a template variable\n"
        "  template setvars        - Enter multiple variables interactively\n"
        "  template vars           - Show current variables in memory\n"
        "  template clearvars      - Clear all current variables\n"
        "  template use NAME       - Use a template with current variables\n"
        "  template dir [PATH]     - Show or set templates directory\n"
        "  template inherit CHILD PARENT - Create a template inheriting from another\n"
        "  template parents NAME   - Show inheritance chain for a template\n"
        "  template validate NAME  - Validate a template\n"
        "  template validateall    - Validate all templates\n"
        "  template docs NAME      - Generate documentation for a template\n"
        "  template docsall        - Generate documentation for all templates\n"
        "  template export PATH [format] - Export documentation to a file (formats: md, html, txt)\n"
        "  categories              - List template categories\n"
        "  category create NAME    - Create a new template category\n");
}

// Handle basic query commands (clear, show, compile, load, save)
bool handle_basic_commands(const std::string& line, std::string& current_query) {
    if (line == "clear") {
        current_query.clear();
        Logger::getInstance().log(LogLevel::INFO, "Query cleared");
        return true;
    }
    if (line == "show") {
        if (current_query.empty()) {
            Logger::getInstance().log(LogLevel::INFO, "Current query is empty");
        } else {
            Logger::getInstance().log(LogLevel::INFO, "Current query:\n", current_query);
        }
        return true;
    }
    if (line == "compile") {
        if (current_query.empty()) {
            Logger::getInstance().log(LogLevel::ERROR, "Nothing to compile");
            return true;
        }

        try {
            const std::string result = QueryProcessor::compile(current_query);
            Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n",
                                     result, "\n===================");
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Compilation error: ", e.what());
        }
        return true;
    }
    if (line.substr(0, 5) == "load ") {
        const std::string filename = line.substr(5);
        try {
            current_query = util::read_file(filename);
            Logger::getInstance().log(LogLevel::INFO, "Loaded query from ", filename);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to load file: ", e.what());
        }
        return true;
    }
    if (line.substr(0, 5) == "save ") {
        const std::string filename = line.substr(5);
        try {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::ERROR, "Nothing to save");
                return true;
            }

            QueryProcessor::save_compiled(current_query, filename);
            Logger::getInstance().log(LogLevel::INFO, "Saved compiled query to ", filename);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to save file: ", e.what());
        }
        return true;
    }
    return false;
}

// List templates and categories
bool handle_list_commands(const std::string& line, const TemplateManager& template_manager) {
    if (line == "templates") {
        try {
            if (const auto templates = template_manager.list_templates(); templates.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "No templates found");
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Available templates:");
                for (const auto& tmpl : templates) {
                    UserOutputManager::info("  ", tmpl);
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error listing templates: ", e.what());
        }
        return true;
    }
    if (line == "categories") {
        try {
            if (const auto categories = template_manager.list_categories(); categories.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "No categories found");
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Available categories:");
                for (const auto& category : categories) {
                    UserOutputManager::info("  ", category);
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error listing categories: ", e.what());
        }
        return true;
    }
    if (line.substr(0, 16) == "category create ") {
        const std::string category = line.substr(16);
        try {
            if (template_manager.create_category(category)) {
                Logger::getInstance().log(LogLevel::INFO, "Category created: ", category);
            } else {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to create category: ", category);
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error creating category: ", e.what());
        }
        return true;
    }
    return false;
}

// Template directory commands
bool handle_template_dir_commands(const std::string& line, TemplateManager& template_manager) {
    if (line == "template dir") {
        UserOutputManager::info("Templates directory: ", template_manager.get_templates_directory());
        return true;
    }
    if (line.substr(0, 13) == "template dir ") {
        std::string dir = line.substr(13);
        try {
            template_manager.set_templates_directory(dir);
            Logger::getInstance().log(LogLevel::INFO, "Templates directory set to: ", dir);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to set templates directory: ", e.what());
        }
        return true;
    }
    return false;
}

// Handle template save/load/delete/info commands
bool handle_template_basic_commands(const std::string& line,
                                   std::string& current_query,
                                   TemplateManager& template_manager,
                                   TemplateValidator& template_validator) {
    if (line.substr(0, 14) == "template save ") {
        std::string name = line.substr(14);
        try {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::ERROR, "Cannot save empty template");
                return true;
            }

            // validate the template before saving

            // check for validation errors
            if (auto validation_result = template_validator.validate_content(current_query);
                validation_result.has_issues(TemplateValidationLevel::ERROR)) {
                Logger::getInstance().log(LogLevel::ERROR, "Template validation failed with errors:");
                for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                    UserOutputManager::info("  - ", issue.to_string());
                }

                std::cout << "Do you want to save the template anyway? (y/n): ";
                std::string response;
                std::getline(std::cin, response);

                if (response != "y" && response != "Y") {
                    Logger::getInstance().log(LogLevel::INFO, "Template save cancelled");
                    return true;
                }
            } else if (validation_result.has_issues(TemplateValidationLevel::WARNING)) {
                Logger::getInstance().log(LogLevel::NORMAL, "Template has validation warnings:");
                for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::WARNING)) {
                    UserOutputManager::info("  - ", issue.to_string());
                }
            }

            template_manager.save_template(name, current_query);
            Logger::getInstance().log(LogLevel::INFO, "Query saved as template: ", name);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to save template: ", e.what());
        }
        return true;
    }
    if (line.substr(0, 14) == "template load ") {
        std::string name = line.substr(14);
        try {
            current_query = template_manager.load_template(name);
            Logger::getInstance().log(LogLevel::INFO, "Template loaded: ", name);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to load template: ", e.what());
        }
        return true;
    }
    if (line.substr(0, 14) == "template info ") {
        std::string name = line.substr(14);
        try {
            auto [template_name, template_description, template_variables, template_last_modified, template_parent]
                = template_manager.get_template_metadata(name);
            UserOutputManager::info("Template: ", template_name);
            UserOutputManager::info("Description: ", template_description);
            UserOutputManager::info("Last modified: ", template_last_modified);

            // show the parent template if it exists
            if (template_parent.has_value() && !template_parent.value().empty()) {
                UserOutputManager::info("Inherits from: ", template_parent.value());
            }

            if (!template_variables.empty()) {
                UserOutputManager::info("Variables:");
                for (const auto& var : template_variables) {
                    UserOutputManager::info("  ${", var, "}");
                }
            } else {
                UserOutputManager::info("No variables found");
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to get template info: ", e.what());
        }
        return true;
    }
    if (line.substr(0, 16) == "template delete ") {
        std::string name = line.substr(16);
        try {
            if (template_manager.delete_template(name)) {
                Logger::getInstance().log(LogLevel::INFO, "Template deleted: ", name);
            } else {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to delete template: ", name);
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error deleting template: ", e.what());
        }
        return true;
    }

    return false;
}

// Handle variable management commands
bool handle_variable_commands(const std::string& line,
                             std::string& current_query,
                             std::map<std::string, std::string>& current_variables,
                             TemplateManager& template_manager) {
    if (line.substr(0, 16) == "template setvar ") {
        std::string var_def = line.substr(16);
        size_t equals_pos = var_def.find('=');

        if (equals_pos == std::string::npos) {
            Logger::getInstance().log(LogLevel::ERROR, "Invalid variable format. Use NAME=VALUE");
            return true;
        }

        std::string name = var_def.substr(0, equals_pos);
        std::string value = var_def.substr(equals_pos + 1);

        // add/update the variable in both current query- and memory -
        // first update in memory for future template usage
        current_variables[name] = value;

        // then update in the current query if it exists
        std::stringstream new_query;
        bool variable_updated = false;

        // if the query is empty, add the variable
        if (current_query.empty()) {
            new_query << "@variable \"" << name << "\" \"" << value << "\"";
            current_query = new_query.str();
            variable_updated = true;
        } else {
            // check if a variable already exists in the query
            std::istringstream iss(current_query);
            std::string get_line;
            std::regex var_regex("@variable\\s+\"" + name + "\"\\s+\"([^\"]*)\"");

            while (std::getline(iss, get_line)) {
                if (std::smatch match; std::regex_match(get_line, match, var_regex)) {
                    // update the existing variable
                    new_query << "@variable \"" << name << "\" \"" << value << "\"";
                    variable_updated = true;
                } else {
                    new_query << get_line;
                }
                new_query << std::endl;
            }

            // if the variable wasn't found, add it at the beginning
            if (!variable_updated) {
                std::string var_declaration = "@variable \"";
                var_declaration.append(name);
                var_declaration.append("\" \"");
                var_declaration.append(value);
                var_declaration.append("\"\n");
                current_query.append(var_declaration);
                current_query.append(current_query);
            } else {
                current_query = new_query.str();
            }
        }

        if (variable_updated) {
            Logger::getInstance().log(LogLevel::INFO, "Variable updated: ", name, "=", value);
        } else {
            Logger::getInstance().log(LogLevel::INFO, "Variable added: ", name, "=", value);
        }
        return true;
    }
    if (line == "template vars") {
        // show current variables in memory
        if (current_variables.empty()) {
            Logger::getInstance().log(LogLevel::INFO, "No variables currently defined");
        } else {
            Logger::getInstance().log(LogLevel::INFO, "Current variables:");
            for (const auto& [name, value] : current_variables) {
                UserOutputManager::info("  ", name, " = \"", value, "\"");
            }
        }
        return true;
    }
    if (line == "template clearvars") {
        // clear all current variables
        current_variables.clear();
        Logger::getInstance().log(LogLevel::INFO, "All variables cleared");
        return true;
    }
    if (line.substr(0, 14) == "template vars ") {
        // list variables in a specific template
        std::string template_name = line.substr(14);
        try {
            if (auto metadata = template_manager.get_template_metadata(template_name); metadata.variables.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "No variables found in template: ", template_name);
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Variables in template: ", template_name);

                // get template content to extract default values if available
                std::string content = template_manager.load_template(template_name);
                auto variables_with_values = cql::TemplateManager::collect_variables(content);

                for (const auto& var_name : metadata.variables) {
                    std::string default_value;
                    if (variables_with_values.contains(var_name))
                        default_value = variables_with_values[var_name];
                    else
                        default_value = "(no default)";
                    UserOutputManager::info("  ", var_name, " = \"", default_value, "\"");
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error listing template variables: ", e.what());
        }
        return true;
    }
    if (line == "template setvars") {
        // interactive mode for setting multiple variables
        Logger::getInstance().log(LogLevel::INFO, "Enter variables in NAME=VALUE format (empty line to finish):");

        while (true) {
            std::string var_line;
            std::cout << "var> ";
            std::getline(std::cin, var_line);

            if (var_line.empty()) {
                break;
            }

            size_t equals_pos = var_line.find('=');
            if (equals_pos == std::string::npos) {
                Logger::getInstance().log(LogLevel::ERROR, "Invalid format. Use NAME=VALUE");
                continue;
            }

            std::string name = var_line.substr(0, equals_pos);
            std::string value = var_line.substr(equals_pos + 1);

            // add to current variables
            current_variables[name] = value;
            Logger::getInstance().log(LogLevel::INFO, "Variable set: ", name, "=", value);
        }

        Logger::getInstance().log(LogLevel::INFO, "Finished setting variables");
        return true;
    }
    return false;
}

// Handle template use command
bool handle_template_use(const std::string& line, std::string& current_query
    , std::map<std::string, std::string>& current_variables, TemplateManager& template_manager
    , TemplateValidator& template_validator) {
    if (line.substr(0, 13) == "template use ") {
        std::string name = line.substr(13);
        try {
            // first validate the template
            auto validation_result = template_validator.validate_template(name);

            // check for critical errors
            if (validation_result.has_issues(TemplateValidationLevel::ERROR)) {
                Logger::getInstance().log(LogLevel::ERROR, "Template validation failed with errors:");
                for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                    UserOutputManager::info("  - ", issue.to_string());
                }

                std::cout << "Do you want to use this template anyway? (y/n): ";
                std::string response;
                std::getline(std::cin, response);

                if (response != "y" && response != "Y") {
                    Logger::getInstance().log(LogLevel::INFO, "Template use cancelled");
                    return true;
                }
            } else if (validation_result.has_issues(TemplateValidationLevel::WARNING)) {
                Logger::getInstance().log(LogLevel::NORMAL, "Template has validation warnings:");
                for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::WARNING)) {
                    UserOutputManager::info("  - ", issue.to_string());
                }
            }

            // extract variables from the current query
            std::map<std::string, std::string> variables;
            std::istringstream iss(current_query);
            std::string curr_line;
            std::regex var_regex("@variable\\s+\"([^\"]*)\"\\s+\"([^\"]*)\"");

            while (std::getline(iss, curr_line)) {
                if (std::smatch match; std::regex_match(curr_line, match, var_regex) && match.size() > 2) {
                    variables[match[1].str()] = match[2].str();
                }
            }

            // combine current variables with template variables will override memory variables
            // if there are duplicates
            std::map<std::string, std::string> combined_variables = current_variables;
            for (const auto& [name, value] : variables) {
                combined_variables[name] = value;
            }

            // check for missing variables
            std::string template_content = template_manager.load_template(name);
            auto template_vars = cql::TemplateManager::collect_variables(template_content);

            // get all variables used in the template from a validation result
            auto referenced_vars = validation_result.get_issues(TemplateValidationLevel::INFO);
            std::vector<std::string> missing_vars;

            // check for variables that are referenced but not declared
            for (const auto& issue : referenced_vars) {
                // if the issue is about a variable, check if it's available
                if (issue.get_variable_name().has_value()) {
                    if (std::string var_name = issue.get_variable_name().value();
                        !combined_variables.contains(var_name) &&!template_vars.contains(var_name)) {
                        missing_vars.push_back(var_name);
                    }
                }
            }

            // if there are missing variables, prompt the user to enter them
            if (!missing_vars.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "Template is missing values for these variables:");

                for (const auto& var : missing_vars) {
                    std::cout << "  Enter value for '" << var << "': ";
                    std::string value;
                    std::getline(std::cin, value);
                    combined_variables[var] = value;
                }
            }

            // instantiate the template with the variables
            current_query = template_manager.instantiate_template(name, combined_variables);
            Logger::getInstance().log(LogLevel::INFO, "Template instantiated: ", name);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to use template: ", e.what());
        }
        return true;
    }
    return false;
}

// Handle template inheritance commands
bool handle_template_inheritance(const std::string& line, std::string& current_query, TemplateManager& template_manager
    , TemplateValidator& template_validator) {
    if (line.substr(0, 17) == "template inherit ") {
        std::string params = line.substr(17);
        size_t space_pos = params.find(' ');
        if (space_pos == std::string::npos) {
            Logger::getInstance().log(LogLevel::ERROR,
                "Invalid format. Use: template inherit CHILD_NAME PARENT_NAME");
            return true;
        }

        std::string child_name = params.substr(0, space_pos);
        std::string parent_name = params.substr(space_pos + 1);

        try {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::ERROR, "Cannot create inherited template with empty content");
                return true;
            }

            // add inherit directive if not already present
            if (std::regex inherit_regex("@inherit\\s+\"([^\"]*)\""); !std::regex_search(current_query, inherit_regex)) {
                // add @inherit directive at the beginning of the content
                const std::string saved_query = current_query;
                current_query.append(parent_name);
                current_query.append("\"\n");
                current_query.append(saved_query);
            }

            // validate the template before saving
            auto validation_result = template_validator.validate_content(current_query);

            // check for inheritance errors
            bool has_inheritance_error = false;
            for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                if (issue.to_string().find("inherit") != std::string::npos
                    || issue.to_string().find("circular") != std::string::npos) {
                    has_inheritance_error = true;
                    break;
                }
            }

            // if there are inheritance errors, don't allow override
            if (has_inheritance_error) {
                Logger::getInstance().log(LogLevel::ERROR, "Template inheritance validation failed:");
                for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                    if (issue.to_string().find("inherit") != std::string::npos ||
                        issue.to_string().find("circular") != std::string::npos) {
                        UserOutputManager::info("  - ", issue.to_string());
                    }
                }
                return true;
            }

            // for other validation errors, prompt for confirmation
            if (validation_result.has_issues(TemplateValidationLevel::ERROR)) {
                Logger::getInstance().log(LogLevel::ERROR, "Template validation failed with errors:");
                for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                    UserOutputManager::info("  - ", issue.to_string());
                }

                std::cout << "Do you want to save the template anyway? (y/n): ";
                std::string response;
                std::getline(std::cin, response);

                if (response != "y" && response != "Y") {
                    Logger::getInstance().log(LogLevel::INFO, "Template save cancelled");
                    return true;
                }
            } else if (validation_result.has_issues(TemplateValidationLevel::WARNING)) {
                Logger::getInstance().log(LogLevel::NORMAL, "Template has validation warnings:");
                for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::WARNING)) {
                    UserOutputManager::info("  - ", issue.to_string());
                }
            }

            template_manager.create_inherited_template(child_name, parent_name, current_query);
            Logger::getInstance().log(LogLevel::INFO, "Created template '", child_name,
                                     "' inheriting from '", parent_name, "'");
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to create inherited template: ", e.what());
        }
        return true;
    }
    if (line.substr(0, 17) == "template parents ") {
        std::string template_name = line.substr(17);
        try {
            if (auto chain = template_manager.get_inheritance_chain(template_name); chain.size() <= 1) {
                Logger::getInstance().log(LogLevel::INFO, "Template '", template_name,
                                         "' does not inherit from any other template");
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Inheritance chain for '", template_name, "':");

                for (size_t i = 0; i < chain.size(); ++i) {
                    // the first template is the base, the last is the current template
                    if (i == 0) {
                        UserOutputManager::info("  Base: ", chain[i]);
                    } else if (i == chain.size() - 1) {
                        UserOutputManager::info("  Current: ", chain[i]);
                    } else {
                        UserOutputManager::info("  Parent ", std::to_string(i), ": ", chain[i]);
                    }
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error getting inheritance chain: ", e.what());
        }
        return true;
    }
    return false;
}

// Handle template validation commands
bool handle_template_validation(const std::string& line, const TemplateManager& template_manager
    , TemplateValidator& template_validator) {
    if (line.substr(0, 18) == "template validate ") {
        const std::string template_name = line.substr(18);
        try {
            const auto result = template_validator.validate_template(template_name);

            // output validation results
            UserOutputManager::info("Validation results for template '", template_name, "':");
            UserOutputManager::info("------------------------------------------");

            if (result.has_issues()) {
                UserOutputManager::info("Found ", std::to_string(result.count_errors()), " errors, ",
                          std::to_string(result.count_warnings()), " warnings, ",
                          std::to_string(result.count_infos()), " info messages.");

                // print errors
                if (result.count_errors() > 0) {
                    UserOutputManager::info("\nErrors:");
                    for (const auto& issue : result.get_issues(TemplateValidationLevel::ERROR)) {
                        UserOutputManager::info("  - ", issue.to_string());
                    }
                }

                // print warnings
                if (result.count_warnings() > 0) {
                    UserOutputManager::info("\nWarnings:");
                    for (const auto& issue : result.get_issues(TemplateValidationLevel::WARNING)) {
                        UserOutputManager::info("  - ", issue.to_string());
                    }
                }

                // print info messages
                if (result.count_infos() > 0) {
                    UserOutputManager::info("\nInfo:");
                    for (const auto& issue : result.get_issues(TemplateValidationLevel::INFO)) {
                        UserOutputManager::info("  - ", issue.to_string());
                    }
                }
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Template validated successfully with no issues");
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error validating template: ", e.what());
        }
        return true;
    }
    if (line == "template validateall") {
        try {
            if (const auto templates = template_manager.list_templates(); templates.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "No templates found to validate");
            } else {
                UserOutputManager::info("Validating ", std::to_string(templates.size()), " templates...");
                UserOutputManager::info("----------------------------");

                size_t error_count = 0;
                size_t warning_count = 0;
                size_t info_count = 0;

                // keep track of templates with issues
                std::vector<std::string> templates_with_errors;
                std::vector<std::string> templates_with_warnings;

                for (const auto& tmpl : templates) {
                    auto result = template_validator.validate_template(tmpl);

                    // count issues
                    error_count += result.count_errors();
                    warning_count += result.count_warnings();
                    info_count += result.count_infos();

                    // print progress
                    if (result.has_issues(TemplateValidationLevel::ERROR)) {
                        templates_with_errors.push_back(tmpl);
                        UserOutputManager::info("❌ ", tmpl, ": ", std::to_string(result.count_errors()), " errors, ",
                                  std::to_string(result.count_warnings()), " warnings");
                    } else if (result.has_issues(TemplateValidationLevel::WARNING)) {
                        templates_with_warnings.push_back(tmpl);
                        UserOutputManager::info("⚠️ ", tmpl, ": ", std::to_string(result.count_warnings()), " warnings");
                    } else {
                        UserOutputManager::info("✅ ", tmpl, ": No issues");
                    }
                }

                // print summary
                UserOutputManager::info("\nValidation Summary:");
                UserOutputManager::info("----------------------------");
                UserOutputManager::info("Templates validated: ", std::to_string(templates.size()));
                UserOutputManager::info("Total issues: ", std::to_string(error_count + warning_count + info_count), " (",
                          std::to_string(error_count), " errors, ",
                          std::to_string(warning_count), " warnings, ",
                          std::to_string(info_count), " info messages)");

                // list templates with errors
                if (!templates_with_errors.empty()) {
                    UserOutputManager::info("\nTemplates with errors:");
                    for (const auto& tmpl : templates_with_errors) {
                        UserOutputManager::info("  - ", tmpl);
                    }
                    UserOutputManager::info("Run 'template validate <n>' for details");
                }

                if (error_count > 0) {
                    Logger::getInstance().log(LogLevel::ERROR,
                        "Validation found ", error_count, " errors in ", templates_with_errors.size(), " template(s)");
                } else if (warning_count > 0) {
                    Logger::getInstance().log(LogLevel::NORMAL,
                        "Validation found ", warning_count, " warnings in ", templates_with_warnings.size(), " template(s)");
                } else {
                    Logger::getInstance().log(LogLevel::INFO, "All templates validated successfully");
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "Error validating templates: ", e.what());
        }
        return true;
    }
    return false;
}

// Handle template documentation commands
bool handle_template_documentation(const std::string& line, const TemplateManager& template_manager) {
    if (line.substr(0, 14) == "template docs ") {
        const std::string template_name = line.substr(14);
        try {
            std::string docs = template_manager.generate_template_documentation(template_name);

            UserOutputManager::info("\n===== Template Documentation =====\n");
            UserOutputManager::info(docs);
            UserOutputManager::info("\n==================================");

            Logger::getInstance().log(LogLevel::INFO, "generated documentation for template: ", template_name);
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "error generating template documentation: ", e.what());
        }
        return true;
    }
    if (line == "template docsall") {
        try {
            std::string docs = template_manager.generate_all_template_documentation();

            UserOutputManager::info("\n===== Template Documentation =====\n");
            UserOutputManager::info("this is a preview of the documentation. use 'template export' to save to a file.");
            UserOutputManager::info("\n", docs.substr(0, 1000), "...");
            UserOutputManager::info("\n(documentation truncated for display. use 'template export' to view full documentation)");
            UserOutputManager::info("\n==================================");

            Logger::getInstance().log(LogLevel::INFO, "generated documentation for all templates");
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "error generating template documentation: ", e.what());
        }
        return true;
    }
    if (line.substr(0, 16) == "template export ") {
        std::string params = line.substr(16);
        std::string output_path;
        std::string format = "markdown"; // default format

        // check if a format is specified
        size_t space_pos = params.find(' ');
        if (space_pos != std::string::npos) {
            output_path = params.substr(0, space_pos);
            format = params.substr(space_pos + 1);
        } else {
            output_path = params;
        }

        try {
            // export the documentation
            if (template_manager.export_documentation(output_path, format)) {
                Logger::getInstance().log(LogLevel::INFO,
                    "template documentation exported to ", output_path, " in ", format, " format");
            } else {
                Logger::getInstance().log(LogLevel::ERROR, "failed to export template documentation");
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "error exporting template documentation: ", e.what());
        }
        return true;
    }
    return false;
}

/**
 * @brief Command pattern implementation with map-based dispatch
 *
 * Shared context for all command handlers, containing references
 * to all state needed for command execution.
 */
struct InteractiveContext {
    std::string& current_query;                        ///< Current query being built
    TemplateManager& template_manager;                 ///< Template management system
    TemplateValidator& template_validator;             ///< Template validation system
    std::map<std::string, std::string>& current_variables; ///< Template variables
};

/**
 * @brief Command handler type for the command pattern
 *
 * Each handler accepts the input string and context and returns
 * true if the command was handled, false otherwise.
 */
using CommandHandler = std::function<bool(const std::string&, InteractiveContext&)>;

/**
 * @brief Command matcher for exact string matches
 *
 * @param input User input to check
 * @param command Command string to match against
 * @return true if input exactly matches the command
 */
bool exact_match(const std::string& input, const std::string& command) {
    return input == command;
}

/**
 * @brief Command matcher for prefix string matches
 *
 * Used for commands with arguments (e.g., "load filename.txt")
 *
 * @param input User input to check
 * @param prefix Command prefix to match against
 * @return true if input starts with the specified prefix
 */
bool prefix_match(const std::string& input, const std::string& prefix) {
    return input.size() >= prefix.size() && input.substr(0, prefix.size()) == prefix;
}

/**
 * @brief The main interactive interface for using CQL
 *
 * Implements a REPL (Read-Eval-Print Loop) using the command pattern
 * for extensible command handling with minimal conditional nesting.
 */
void run_interactive() {
    Logger::getInstance().log(LogLevel::INFO, "CQL Interactive Mode");
    Logger::getInstance().log(LogLevel::INFO, "Type 'exit' to quit, 'help' for command list");

    std::string line;                // Current input line
    std::string current_query;       // Query being constructed
    TemplateManager template_manager;// Template manager instance

    // Create template validator with default schema
    TemplateValidator template_validator = initialize_template_validator(template_manager);

    // Store variables in memory for template instantiation
    std::map<std::string, std::string> current_variables;

    // Create interactive context for command handlers
    InteractiveContext context = {
        current_query,              // Shared query content
        template_manager,           // Shared template manager
        template_validator,         // Shared validator
        current_variables           // Shared variables
    };

    /**
     * @brief Registry of command matchers and handlers
     *
     * Each entry is a pair containing:
     * 1. A matcher function that determines if input matches a command
     * 2. A handler function that implements the command's behavior
     */
    std::vector<std::pair<std::function<bool(const std::string&)>, CommandHandler>> commands;

    /**
     * @brief Help command definition
     *
     * Displays the list of available commands.
     */
    commands.emplace_back(
        // Matcher checks for the exact "help" command
        [](const std::string& input) { return exact_match(input, "help"); },

        // Handler displays the help menu (ignoring parameters)
        [](const std::string& /*input*/, InteractiveContext& /*ctx*/) {
            display_help_menu();
            return true;
        }
    );

    /**
     * @brief Basic query management commands
     *
     * Includes: clear, show, compile, load, save
     */
    commands.emplace_back(
        // Matcher for basic query commands
        [](const std::string& input) {
            return exact_match(input, "clear")   ||
                   exact_match(input, "show")    ||
                   exact_match(input, "compile") ||
                   prefix_match(input, "load ")  ||
                   prefix_match(input, "save ");
        },
        // Handler delegates to basic_commands implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_basic_commands(input, ctx.current_query);
        }
    );

    /**
     * @brief Template listing and category commands
     *
     * Includes: templates, categories, category create
     */
    commands.emplace_back(
        // Matcher for listing commands
        [](const std::string& input) {
            return exact_match(input, "templates") ||
                   exact_match(input, "categories") ||
                   prefix_match(input, "category create ");
        },
        // Handler delegates to list_commands implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_list_commands(input, ctx.template_manager);
        }
    );

    /**
     * @brief Template directory management commands
     *
     * Includes: template dir, template dir [path]
     */
    commands.emplace_back(
        // Matcher for template directory commands
        [](const std::string& input) {
            return exact_match(input, "template dir") ||
                   prefix_match(input, "template dir ");
        },
        // Handler delegates to template_dir_commands implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_template_dir_commands(input, ctx.template_manager);
        }
    );

    /**
     * @brief Basic template management commands
     *
     * Includes: template save, template load, template info, template delete
     */
    commands.emplace_back(
        // Matcher for basic template commands
        [](const std::string& input) {
            return prefix_match(input, "template save ") ||
                   prefix_match(input, "template load ") ||
                   prefix_match(input, "template info ") ||
                   prefix_match(input, "template delete ");
        },
        // Handler delegates to template_basic_commands implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_template_basic_commands(input, ctx.current_query
                , ctx.template_manager, ctx.template_validator);
        }
    );

    /**
     * @brief Template variable management commands
     *
     * Includes: template setvar, template vars, template clearvars,
     * template vars [name], template setvars
     */
    commands.emplace_back(
        // Matcher for variable management commands
        [](const std::string& input) {
            return prefix_match(input, "template setvar ") ||
                   exact_match(input, "template vars") ||
                   exact_match(input, "template clearvars") ||
                   prefix_match(input, "template vars ") ||
                   exact_match(input, "template setvars");
        },
        // Handler delegates to variable_commands implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_variable_commands(input, ctx.current_query, ctx.current_variables, ctx.template_manager);
        }
    );

    /**
     * @brief Template instantiation command
     *
     * Handles: template use [name]
     */
    commands.emplace_back(
        // Matcher for template use command
        [](const std::string& input) { return prefix_match(input, "template use "); },
        // Handler delegates to template_use implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_template_use(input, ctx.current_query, ctx.current_variables
                , ctx.template_manager, ctx.template_validator);
        }
    );

    /**
     * @brief Template inheritance commands
     *
     * Includes: template inherit, template parents
     */
    commands.emplace_back(
        // Matcher for template inheritance commands
        [](const std::string& input) {
            return prefix_match(input, "template inherit ") ||
                   prefix_match(input, "template parents ");
        },
        // Handler delegates to template_inheritance implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_template_inheritance(input, ctx.current_query, ctx.template_manager, ctx.template_validator);
        }
    );

    /**
     * @brief Template validation commands
     *
     * Includes: template validate, template validateall
     */
    commands.emplace_back(
        // Matcher for template validation commands
        [](const std::string& input) {
            return prefix_match(input, "template validate ") ||
                   exact_match(input, "template validateall");
        },
        // Handler delegates to template_validation implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_template_validation(input, ctx.template_manager, ctx.template_validator);
        }
    );

    /**
     * @brief Template documentation commands
     *
     * Includes: template docs, template docsall, template export
     */
    commands.emplace_back(
        // Matcher for template documentation commands
        [](const std::string& input) {
            return prefix_match(input, "template docs ") ||
                   exact_match(input, "template docsall") ||
                   prefix_match(input, "template export ");
        },
        // Handler delegates to template_documentation implementation
        [](const std::string& input, const InteractiveContext& ctx) {
            return handle_template_documentation(input, ctx.template_manager);
        }
    );

    /**
     * @brief Main command processing loop
     *
     * Reads user input, dispatches to appropriate command handler, and
     * updates the current query when input doesn't match any commands.
     */
    while (true) {
        // Prompt and read user input
        std::cout << "> ";
        std::getline(std::cin, line);

        // Special case: exit commands break the loop immediately
        if (line == "exit" || line == "quit") {
            break;
        }

        // Dispatch to command handlers
        bool handled = false;
        for (const auto& [matcher, handler] : commands) {
            // If a matcher returns true, execute its handler
            if (matcher(line)) {
                handled = handler(line, context);
                break;  // Stop after the first matching handler
            }
        }

        // Default behavior: treat input as query content
        if (!handled) {
            // Add the line to the current query
            if (!current_query.empty()) {
                current_query += "\n";  // Add a newline for multi-line queries
            }
            current_query += line;      // Append the new line
        }
    }
}

// process a query file
bool process_file(const std::string& input_file, const std::string& output_file, bool include_header) {
    try {
        // Always show which file is being processed - useful for users
        UserOutputManager::info("Processing file: ", input_file);

        const std::string result = QueryProcessor::compile_file(input_file);

        if (output_file.empty()) {
            if (include_header) {
                // Standard output with headers
                UserOutputManager::info("\nCompiled Query");
                UserOutputManager::info("==============\n");
                UserOutputManager::info(result);
            } else {
                // Clean output: just the query without headers (default)
                UserOutputManager::info(result);
            }
        } else {
            util::write_file(output_file, result);
            if (include_header) {
                UserOutputManager::info("Compiled query written to ", output_file);
                Logger::getInstance().log(LogLevel::INFO, "Compiled query written to ", output_file);
            }
        }
        return true;
    } catch (const std::exception& e) {
        UserOutputManager::error("Error processing file: ", e.what());
        Logger::getInstance().log(LogLevel::ERROR, "Error processing file: ", e.what());
        return false;
    }
}

// Prepare the configuration for API submission
ApiClientConfig prepare_api_config(const std::string& model, const std::string& output_dir
    , const bool overwrite, const bool create_dirs, const bool no_save) {

    // Load the configuration
    ApiClientConfig config = ApiClientConfig::load_from_default_locations();

    // Override with command-line arguments
    if (!model.empty()) {
        config.set_model(model);
        Logger::getInstance().log(LogLevel::INFO, "Using model: ", model);
    }

    if (!output_dir.empty()) {
        config.set_output_directory(output_dir);
        Logger::getInstance().log(LogLevel::INFO, "Output directory: ", output_dir);
    }

    config.set_overwrite_existing_files(overwrite);
    config.set_create_missing_directories(create_dirs);
    config.set_no_save_mode(no_save);

    return config;
}

// Compile a CQL file for API submission
std::string compile_query_for_api(const std::string& input_file) {
    Logger::getInstance().log(LogLevel::INFO, "Compiling query from: ", input_file);
    std::string query_content = util::read_file(input_file);
    std::string compiled_query = QueryProcessor::compile(query_content);
    Logger::getInstance().log(LogLevel::INFO, "Query compiled successfully");
    return compiled_query;
}

// Submit a compiled query to the API
ApiResponse submit_to_api(const std::string& compiled_query, ApiClientConfig config) {
    Logger::getInstance().log(LogLevel::INFO, "Submitting to Claude API...");
    UserOutputManager::info("Submitting to Claude API (model: ", config.get_model(), ")...");

    ApiClient api_client(std::move(config));
    ApiResponse response = api_client.submit_query(compiled_query);

    if (!response.m_success) {
        Logger::getInstance().log(LogLevel::ERROR, "API request failed: ", response.m_error_message);
        UserOutputManager::error("API request failed: ", response.m_error_message);
    } else {
        Logger::getInstance().log(LogLevel::INFO, "API request successful");
        UserOutputManager::info("API request successful");
    }
    return response;
}

// Process API response into files
std::vector<GeneratedFile> process_api_response(const ApiResponse& response, ApiClientConfig config) {
    ResponseProcessor processor(std::move(config));
    std::vector<GeneratedFile> files = processor.process_response(response.m_raw_response);

    Logger::getInstance().log(LogLevel::INFO, "Generated ", files.size(), " files:");
    UserOutputManager::info("Generated ", std::to_string(files.size()), " files:");

    for (const auto& file : files) {
        Logger::getInstance().log(LogLevel::INFO, "- ", file.m_filename);
        UserOutputManager::info("  - ", file.m_filename);
    }
    return files;
}

// Save generated files to disk
void save_generated_files(const std::vector<GeneratedFile>& files, const ApiClientConfig& config) {
    // Skip saving if no-save mode is enabled
    if (config.no_save_mode()) {
        Logger::getInstance().log(LogLevel::INFO, "Files not saved (--no-save option used)");
        UserOutputManager::info("Files not saved (--no-save option used)");
        return;
    }

    // Save each file
    for (const auto& file : files) {
        save_generated_file(file, config.get_output_directory(), config);
    }

    // Log where files were saved
    if (!config.get_output_directory().empty()) {
        Logger::getInstance().log(LogLevel::INFO, "Files saved to ", config.get_output_directory());
        UserOutputManager::info("Files saved to ", config.get_output_directory());
    } else {
        Logger::getInstance().log(LogLevel::INFO, "Files saved to current directory");
        UserOutputManager::info("Files saved to current directory");
    }
}

// process a submit command
bool process_submit_command(const std::string& input_file, const std::string& output_dir, const std::string& model,
    const bool overwrite, const bool create_dirs, const bool no_save) {
    try {
        // Prepare the configuration
        const ApiClientConfig config = prepare_api_config(model, output_dir, overwrite, create_dirs, no_save);

        // Compile the query
        const std::string compiled_query = compile_query_for_api(input_file);

        // Submit to API
        const ApiResponse response = submit_to_api(compiled_query, config);
        if (!response.m_success) {
            return false;
        }

        // Process the response
        const std::vector<GeneratedFile> files = process_api_response(response, config);

        // Save files
        save_generated_files(files, config);

        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error processing submit command: ", e.what());
        UserOutputManager::error("Error processing submit command: ", e.what());
        return false;
    }
}

} // namespace cql::cli
