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
#include "../../../headers/project_utils.hpp"

namespace cql::cli {

// cli interface for interactive use
void run_cli() {
    Logger::getInstance().log(LogLevel::INFO, "CQL Interactive Mode");
    Logger::getInstance().log(LogLevel::INFO, "Type 'exit' to quit, 'help' for command list");

    std::string line;
    std::string current_query;
    TemplateManager template_manager;
    
    // create template validator with default schema
    TemplateValidator template_validator(template_manager);
    auto schema = TemplateValidatorSchema::create_default_schema();
    
    // add schema validation rules
    for (const auto& [name, rule] : schema.get_validation_rules()) {
        template_validator.add_validation_rule(rule);
    }
    
    // store variables in memory for template instantiation
    std::map<std::string, std::string> current_variables;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line == "exit" || line == "quit") {
            break;
        } else if (line == "help") {
            std::cout << "Commands:\n"
                      << "  help                    - Show this help\n"
                      << "  exit/quit               - Exit the program\n"
                      << "  clear                   - Clear the current query\n"
                      << "  show                    - Show the current query\n"
                      << "  compile                 - Compile the current query\n"
                      << "  load FILE               - Load query from file\n"
                      << "  save FILE               - Save compiled query to file\n"
                      << "\n"
                      << "Template Commands:\n"
                      << "  templates               - List all available templates\n"
                      << "  template save NAME      - Save current query as a template\n"
                      << "  template load NAME      - Load a template\n"
                      << "  template info NAME      - Show info about a template\n"
                      << "  template delete NAME    - Delete a template\n"
                      << "  template vars NAME      - List variables in a template\n"
                      << "  template setvar NAME=VAL - Set a template variable\n"
                      << "  template setvars        - Enter multiple variables interactively\n"
                      << "  template vars           - Show current variables in memory\n"
                      << "  template clearvars      - Clear all current variables\n"
                      << "  template use NAME       - Use a template with current variables\n"
                      << "  template dir [PATH]     - Show or set templates directory\n"
                      << "  template inherit CHILD PARENT - Create a template inheriting from another\n"
                      << "  template parents NAME   - Show inheritance chain for a template\n"
                      << "  template validate NAME  - Validate a template\n"
                      << "  template validateall    - Validate all templates\n"
                      << "  template docs NAME      - Generate documentation for a template\n"
                      << "  template docsall        - Generate documentation for all templates\n"
                      << "  template export PATH [format] - Export documentation to a file (formats: md, html, txt)\n"
                      << "  categories              - List template categories\n"
                      << "  category create NAME    - Create a new template category\n";
        } else if (line == "clear") {
            current_query.clear();
            Logger::getInstance().log(LogLevel::INFO, "Query cleared");
        } else if (line == "show") {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "Current query is empty");
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Current query:\n", current_query);
            }
        } else if (line == "compile") {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::ERROR, "Nothing to compile");
                continue;
            }

            try {
                std::string result = QueryProcessor::compile(current_query);
                Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", 
                                         result, "\n===================");
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Compilation error: ", e.what());
            }
        } else if (line.substr(0, 5) == "load ") {
            std::string filename = line.substr(5);
            try {
                current_query = util::read_file(filename);
                Logger::getInstance().log(LogLevel::INFO, "Loaded query from ", filename);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to load file: ", e.what());
            }
        } else if (line.substr(0, 5) == "save ") {
            std::string filename = line.substr(5);
            try {
                if (current_query.empty()) {
                    Logger::getInstance().log(LogLevel::ERROR, "Nothing to save");
                    continue;
                }

                QueryProcessor::save_compiled(current_query, filename);
                Logger::getInstance().log(LogLevel::INFO, "Saved compiled query to ", filename);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to save file: ", e.what());
            }
        } 
        // template management commands
        else if (line == "templates") {
            try {
                auto templates = template_manager.list_templates();
                if (templates.empty()) {
                    Logger::getInstance().log(LogLevel::INFO, "No templates found");
                } else {
                    Logger::getInstance().log(LogLevel::INFO, "Available templates:");
                    for (const auto& tmpl : templates) {
                        std::cout << "  " << tmpl << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Error listing templates: ", e.what());
            }
        } else if (line.substr(0, 14) == "template save ") {
            std::string name = line.substr(14);
            try {
                if (current_query.empty()) {
                    Logger::getInstance().log(LogLevel::ERROR, "Cannot save empty template");
                    continue;
                }

                // validate the template before saving
                auto validation_result = template_validator.validate_content(current_query);
                
                // check for validation errors
                if (validation_result.has_issues(TemplateValidationLevel::ERROR)) {
                    Logger::getInstance().log(LogLevel::ERROR, "Template validation failed with errors:");
                    for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                        std::cout << "  - " << issue.to_string() << std::endl;
                    }
                    
                    std::cout << "Do you want to save the template anyway? (y/n): ";
                    std::string response;
                    std::getline(std::cin, response);
                    
                    if (response != "y" && response != "Y") {
                        Logger::getInstance().log(LogLevel::INFO, "Template save cancelled");
                        continue;
                    }
                } else if (validation_result.has_issues(TemplateValidationLevel::WARNING)) {
                    Logger::getInstance().log(LogLevel::NORMAL, "Template has validation warnings:");
                    for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::WARNING)) {
                        std::cout << "  - " << issue.to_string() << std::endl;
                    }
                }

                template_manager.save_template(name, current_query);
                Logger::getInstance().log(LogLevel::INFO, "Query saved as template: ", name);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to save template: ", e.what());
            }
        } else if (line.substr(0, 14) == "template load ") {
            std::string name = line.substr(14);
            try {
                current_query = template_manager.load_template(name);
                Logger::getInstance().log(LogLevel::INFO, "Template loaded: ", name);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to load template: ", e.what());
            }
        } else if (line.substr(0, 14) == "template info ") {
            std::string name = line.substr(14);
            try {
                auto metadata = template_manager.get_template_metadata(name);
                std::cout << "Template: " << metadata.name << std::endl;
                std::cout << "Description: " << metadata.description << std::endl;
                std::cout << "Last modified: " << metadata.last_modified << std::endl;
                
                // show parent template if it exists
                if (metadata.parent.has_value() && !metadata.parent.value().empty()) {
                    std::cout << "Inherits from: " << metadata.parent.value() << std::endl;
                }
                
                if (!metadata.variables.empty()) {
                    std::cout << "Variables:" << std::endl;
                    for (const auto& var : metadata.variables) {
                        std::cout << "  ${" << var << "}" << std::endl;
                    }
                } else {
                    std::cout << "No variables found" << std::endl;
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to get template info: ", e.what());
            }
        } else if (line.substr(0, 16) == "template delete ") {
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
        } else if (line.substr(0, 16) == "template setvar ") {
            std::string var_def = line.substr(16);
            size_t equals_pos = var_def.find('=');
            
            if (equals_pos == std::string::npos) {
                Logger::getInstance().log(LogLevel::ERROR, "Invalid variable format. Use NAME=VALUE");
                continue;
            }
            
            std::string name = var_def.substr(0, equals_pos);
            std::string value = var_def.substr(equals_pos + 1);
            
            // add/update the variable in both current query and memory
            // first update in memory for future template usage
            current_variables[name] = value;
            
            // then update in the current query if it exists
            std::stringstream new_query;
            bool variable_updated = false;
            
            // if the query is empty, just add the variable
            if (current_query.empty()) {
                new_query << "@variable \"" << name << "\" \"" << value << "\"";
                current_query = new_query.str();
                variable_updated = true;
            } else {
                // check if variable already exists in the query
                std::istringstream iss(current_query);
                std::string line;
                std::regex var_regex("@variable\\s+\"" + name + "\"\\s+\"([^\"]*)\"");
                
                while (std::getline(iss, line)) {
                    std::smatch match;
                    if (std::regex_match(line, match, var_regex)) {
                        // update the existing variable
                        new_query << "@variable \"" << name << "\" \"" << value << "\"";
                        variable_updated = true;
                    } else {
                        new_query << line;
                    }
                    new_query << std::endl;
                }
                
                // if the variable wasn't found, add it at the beginning
                if (!variable_updated) {
                    std::string var_declaration = "@variable \"" + name + "\" \"" + value + "\"\n";
                    current_query = var_declaration + current_query;
                } else {
                    current_query = new_query.str();
                }
            }
            
            if (variable_updated) {
                Logger::getInstance().log(LogLevel::INFO, "Variable updated: ", name, "=", value);
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Variable added: ", name, "=", value);
            }
        } else if (line.substr(0, 13) == "template use ") {
            std::string name = line.substr(13);
            try {
                // first validate the template
                auto validation_result = template_validator.validate_template(name);
                
                // check for critical errors
                if (validation_result.has_issues(TemplateValidationLevel::ERROR)) {
                    Logger::getInstance().log(LogLevel::ERROR, "Template validation failed with errors:");
                    for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                        std::cout << "  - " << issue.to_string() << std::endl;
                    }
                    
                    std::cout << "Do you want to use this template anyway? (y/n): ";
                    std::string response;
                    std::getline(std::cin, response);
                    
                    if (response != "y" && response != "Y") {
                        Logger::getInstance().log(LogLevel::INFO, "Template use cancelled");
                        continue;
                    }
                } else if (validation_result.has_issues(TemplateValidationLevel::WARNING)) {
                    Logger::getInstance().log(LogLevel::NORMAL, "Template has validation warnings:");
                    for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::WARNING)) {
                        std::cout << "  - " << issue.to_string() << std::endl;
                    }
                }

                // extract variables from the current query
                std::map<std::string, std::string> variables;
                std::istringstream iss(current_query);
                std::string curr_line;
                std::regex var_regex("@variable\\s+\"([^\"]*)\"\\s+\"([^\"]*)\"");
                
                while (std::getline(iss, curr_line)) {
                    std::smatch match;
                    if (std::regex_match(curr_line, match, var_regex) && match.size() > 2) {
                        variables[match[1].str()] = match[2].str();
                    }
                }
                
                // combine current variables with template variables
                // template variables will override memory variables if there are duplicates
                std::map<std::string, std::string> combined_variables = current_variables;
                for (const auto& [name, value] : variables) {
                    combined_variables[name] = value;
                }
                
                // check for missing variables
                std::string template_content = template_manager.load_template(name);
                auto template_vars = template_manager.collect_variables(template_content);
                
                // get all variables used in the template from validation result
                auto referenced_vars = validation_result.get_issues(TemplateValidationLevel::INFO);
                std::vector<std::string> missing_vars;
                
                // check for variables that are referenced but not declared
                for (const auto& issue : referenced_vars) {
                    // if the issue is about a variable, check if it's available
                    if (issue.get_variable_name().has_value()) {
                        std::string var_name = issue.get_variable_name().value();
                        if (combined_variables.find(var_name) == combined_variables.end() && 
                            template_vars.find(var_name) == template_vars.end()) {
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
        } else if (line == "template dir") {
            // show the current templates directory
            std::cout << "Templates directory: " 
                     << template_manager.get_templates_directory() << std::endl;
        } else if (line.substr(0, 13) == "template dir ") {
            // set the templates directory
            std::string dir = line.substr(13);
            try {
                template_manager.set_templates_directory(dir);
                Logger::getInstance().log(LogLevel::INFO, "Templates directory set to: ", dir);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to set templates directory: ", e.what());
            }
        } else if (line == "categories") {
            // list categories
            try {
                auto categories = template_manager.list_categories();
                if (categories.empty()) {
                    Logger::getInstance().log(LogLevel::INFO, "No categories found");
                } else {
                    Logger::getInstance().log(LogLevel::INFO, "Available categories:");
                    for (const auto& category : categories) {
                        std::cout << "  " << category << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Error listing categories: ", e.what());
            }
        } else if (line.substr(0, 16) == "category create ") {
            // create a new category
            std::string category = line.substr(16);
            try {
                if (template_manager.create_category(category)) {
                    Logger::getInstance().log(LogLevel::INFO, "Category created: ", category);
                } else {
                    Logger::getInstance().log(LogLevel::ERROR, "Failed to create category: ", category);
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Error creating category: ", e.what());
            }
        } else if (line == "template vars") {
            // show current variables in memory
            if (current_variables.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "No variables currently defined");
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Current variables:");
                for (const auto& [name, value] : current_variables) {
                    std::cout << "  " << name << " = \"" << value << "\"" << std::endl;
                }
            }
        } else if (line == "template clearvars") {
            // clear all current variables
            current_variables.clear();
            Logger::getInstance().log(LogLevel::INFO, "All variables cleared");
        } else if (line.substr(0, 14) == "template vars ") {
            // list variables in a specific template
            std::string template_name = line.substr(14);
            try {
                auto metadata = template_manager.get_template_metadata(template_name);
                
                if (metadata.variables.empty()) {
                    Logger::getInstance().log(LogLevel::INFO, "No variables found in template: ", template_name);
                } else {
                    Logger::getInstance().log(LogLevel::INFO, "Variables in template: ", template_name);
                    
                    // get template content to extract default values if available
                    std::string content = template_manager.load_template(template_name);
                    auto variables_with_values = template_manager.collect_variables(content);
                    
                    for (const auto& var_name : metadata.variables) {
                        std::string default_value = variables_with_values.count(var_name) > 0 ? 
                                                   variables_with_values[var_name] : "(no default)";
                        std::cout << "  " << var_name << " = \"" << default_value << "\"" << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Error listing template variables: ", e.what());
            }
        } else if (line == "template setvars") {
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
        } else if (line.substr(0, 17) == "template inherit ") {
            // create a template that inherits from another template
            std::string params = line.substr(17);
            size_t space_pos = params.find(' ');
            if (space_pos == std::string::npos) {
                Logger::getInstance().log(LogLevel::ERROR, 
                    "Invalid format. Use: template inherit CHILD_NAME PARENT_NAME");
                continue;
            }
            
            std::string child_name = params.substr(0, space_pos);
            std::string parent_name = params.substr(space_pos + 1);
            
            try {
                if (current_query.empty()) {
                    Logger::getInstance().log(LogLevel::ERROR, "Cannot create inherited template with empty content");
                    continue;
                }
                
                // add inherit directive if not already present
                std::regex inherit_regex("@inherit\\s+\"([^\"]*)\"");
                if (!std::regex_search(current_query, inherit_regex)) {
                    // add @inherit directive at the beginning of the content
                    current_query = "@inherit \"" + parent_name + "\"\n" + current_query;
                }
                
                // validate the template before saving
                auto validation_result = template_validator.validate_content(current_query);
                
                // check for inheritance errors
                bool has_inheritance_error = false;
                for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                    if (issue.to_string().find("inherit") != std::string::npos || 
                        issue.to_string().find("circular") != std::string::npos) {
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
                            std::cout << "  - " << issue.to_string() << std::endl;
                        }
                    }
                    continue;
                }
                
                // for other validation errors, prompt for confirmation
                if (validation_result.has_issues(TemplateValidationLevel::ERROR)) {
                    Logger::getInstance().log(LogLevel::ERROR, "Template validation failed with errors:");
                    for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::ERROR)) {
                        std::cout << "  - " << issue.to_string() << std::endl;
                    }
                    
                    std::cout << "Do you want to save the template anyway? (y/n): ";
                    std::string response;
                    std::getline(std::cin, response);
                    
                    if (response != "y" && response != "Y") {
                        Logger::getInstance().log(LogLevel::INFO, "Template save cancelled");
                        continue;
                    }
                } else if (validation_result.has_issues(TemplateValidationLevel::WARNING)) {
                    Logger::getInstance().log(LogLevel::NORMAL, "Template has validation warnings:");
                    for (const auto& issue : validation_result.get_issues(TemplateValidationLevel::WARNING)) {
                        std::cout << "  - " << issue.to_string() << std::endl;
                    }
                }
                
                template_manager.create_inherited_template(child_name, parent_name, current_query);
                Logger::getInstance().log(LogLevel::INFO, "Created template '", child_name, 
                                         "' inheriting from '", parent_name, "'");
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to create inherited template: ", e.what());
            }
        } else if (line.substr(0, 17) == "template parents ") {
            // show inheritance chain for a template
            std::string template_name = line.substr(17);
            try {
                auto chain = template_manager.get_inheritance_chain(template_name);
                
                if (chain.size() <= 1) {
                    Logger::getInstance().log(LogLevel::INFO, "Template '", template_name, 
                                             "' does not inherit from any other template");
                } else {
                    Logger::getInstance().log(LogLevel::INFO, "Inheritance chain for '", template_name, "':");
                    
                    for (size_t i = 0; i < chain.size(); ++i) {
                        // first template is the base, last is the current template
                        if (i == 0) {
                            std::cout << "  Base: " << chain[i] << std::endl;
                        } else if (i == chain.size() - 1) {
                            std::cout << "  Current: " << chain[i] << std::endl;
                        } else {
                            std::cout << "  Parent " << i << ": " << chain[i] << std::endl;
                        }
                    }
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Error getting inheritance chain: ", e.what());
            }
        } else if (line.substr(0, 18) == "template validate ") {
            // validate a specific template
            std::string template_name = line.substr(18);
            try {
                auto result = template_validator.validate_template(template_name);
                
                // output validation results
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
                    
                    // print info messages
                    if (result.count_infos() > 0) {
                        std::cout << "\nInfo:" << std::endl;
                        for (const auto& issue : result.get_issues(TemplateValidationLevel::INFO)) {
                            std::cout << "  - " << issue.to_string() << std::endl;
                        }
                    }
                } else {
                    Logger::getInstance().log(LogLevel::INFO, "Template validated successfully with no issues");
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Error validating template: ", e.what());
            }
        } else if (line == "template validateall") {
            // validate all templates
            try {
                auto templates = template_manager.list_templates();
                
                if (templates.empty()) {
                    Logger::getInstance().log(LogLevel::INFO, "No templates found to validate");
                } else {
                    std::cout << "Validating " << templates.size() << " templates..." << std::endl;
                    std::cout << "----------------------------" << std::endl;
                    
                    int error_count = 0;
                    int warning_count = 0;
                    int info_count = 0;
                    
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
                            std::cout << "❌ " << tmpl << ": " << result.count_errors() << " errors, "
                                      << result.count_warnings() << " warnings" << std::endl;
                        } else if (result.has_issues(TemplateValidationLevel::WARNING)) {
                            templates_with_warnings.push_back(tmpl);
                            std::cout << "⚠️ " << tmpl << ": " << result.count_warnings() << " warnings" << std::endl;
                        } else {
                            std::cout << "✅ " << tmpl << ": No issues" << std::endl;
                        }
                    }
                    
                    // print summary
                    std::cout << "\nValidation Summary:" << std::endl;
                    std::cout << "----------------------------" << std::endl;
                    std::cout << "Templates validated: " << templates.size() << std::endl;
                    std::cout << "Total issues: " << (error_count + warning_count + info_count) << " ("
                              << error_count << " errors, " 
                              << warning_count << " warnings, " 
                              << info_count << " info messages)" << std::endl;
                    
                    // list templates with errors
                    if (!templates_with_errors.empty()) {
                        std::cout << "\nTemplates with errors:" << std::endl;
                        for (const auto& tmpl : templates_with_errors) {
                            std::cout << "  - " << tmpl << std::endl;
                        }
                        std::cout << "Run 'template validate <name>' for details" << std::endl;
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
        } else if (line.substr(0, 14) == "template docs ") {
            // generate documentation for a specific template
            std::string template_name = line.substr(14);
            try {
                // generate the documentation
                std::string docs = template_manager.generate_template_documentation(template_name);
                
                // display the documentation
                std::cout << "\n===== Template Documentation =====\n\n";
                std::cout << docs << std::endl;
                std::cout << "\n==================================" << std::endl;
                
                Logger::getInstance().log(LogLevel::INFO, "generated documentation for template: ", template_name);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "error generating template documentation: ", e.what());
            }
        } else if (line == "template docsall") {
            // generate documentation for all templates
            try {
                // generate the documentation
                std::string docs = template_manager.generate_all_template_documentation();
                
                // display the documentation
                std::cout << "\n===== Template Documentation =====\n\n";
                std::cout << "this is a preview of the documentation. use 'template export' to save to a file." << std::endl;
                std::cout << "\n" << docs.substr(0, 1000) << "..." << std::endl;
                std::cout << "\n(documentation truncated for display. use 'template export' to view full documentation)" << std::endl;
                std::cout << "\n==================================" << std::endl;
                
                Logger::getInstance().log(LogLevel::INFO, "generated documentation for all templates");
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "error generating template documentation: ", e.what());
            }
        } else if (line.substr(0, 16) == "template export ") {
            // export documentation to a file
            std::string params = line.substr(16);
            std::string output_path;
            std::string format = "markdown"; // default format
            
            // check if format is specified
            size_t space_pos = params.find(' ');
            if (space_pos != std::string::npos) {
                output_path = params.substr(0, space_pos);
                format = params.substr(space_pos + 1);
            } else {
                output_path = params;
            }
            
            try {
                // export the documentation
                bool success = template_manager.export_documentation(output_path, format);
                
                if (success) {
                    Logger::getInstance().log(LogLevel::INFO, 
                        "template documentation exported to ", output_path, " in ", format, " format");
                } else {
                    Logger::getInstance().log(LogLevel::ERROR, "failed to export template documentation");
                }
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "error exporting template documentation: ", e.what());
            }
        } else {
            // add line to the current query
            if (!current_query.empty()) {
                current_query += "\n";
            }
            current_query += line;
        }
    }
}

// process a query file
bool process_file(const std::string& input_file, const std::string& output_file) {
    try {
        Logger::getInstance().log(LogLevel::INFO, "Processing file: ", input_file);
        std::cout << "Processing file: " << input_file << std::endl;

        std::string result = QueryProcessor::compile_file(input_file);

        if (output_file.empty()) {
            std::cout << "\n=== Compiled Query ===\n\n" 
                     << result << "\n===================" << std::endl;
            Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", 
                                     result, "\n===================");
        } else {
            util::write_file(output_file, result);
            std::cout << "Compiled query written to " << output_file << std::endl;
            Logger::getInstance().log(LogLevel::INFO, "Compiled query written to ", output_file);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error processing file: " << e.what() << std::endl;
        Logger::getInstance().log(LogLevel::ERROR, "Error processing file: ", e.what());
        return false;
    }
}

} // namespace cql::cli
