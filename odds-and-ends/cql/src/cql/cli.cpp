// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_manager.hpp"
#include "../../../headers/project_utils.hpp"

namespace cql::cli {

// CLI interface for interactive use
void run_cli() {
    Logger::getInstance().log(LogLevel::INFO, "CQL Interactive Mode");
    Logger::getInstance().log(LogLevel::INFO, "Type 'exit' to quit, 'help' for command list");

    std::string line;
    std::string current_query;
    TemplateManager template_manager;

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
                      << "  template setvar NAME=VAL - Set a template variable\n"
                      << "  template use NAME       - Use a template with current variables\n"
                      << "  template dir [PATH]     - Show or set templates directory\n"
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
        // Template management commands
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
            
            // Add/update the variable declaration in the current query
            std::stringstream new_query;
            bool variable_updated = false;
            
            // If the query is empty, just add the variable
            if (current_query.empty()) {
                new_query << "@variable \"" << name << "\" \"" << value << "\"";
                variable_updated = true;
            } else {
                // Check if variable already exists in the query
                std::istringstream iss(current_query);
                std::string line;
                std::regex var_regex("@variable\\s+\"" + name + "\"\\s+\"([^\"]*)\"");
                
                while (std::getline(iss, line)) {
                    std::smatch match;
                    if (std::regex_match(line, match, var_regex)) {
                        // Update the existing variable
                        new_query << "@variable \"" << name << "\" \"" << value << "\"";
                        variable_updated = true;
                    } else {
                        new_query << line;
                    }
                    new_query << std::endl;
                }
                
                // If the variable wasn't found, add it at the beginning
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
                // Extract variables from the current query
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
                
                // Instantiate the template with the variables
                current_query = template_manager.instantiate_template(name, variables);
                Logger::getInstance().log(LogLevel::INFO, "Template instantiated: ", name);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to use template: ", e.what());
            }
        } else if (line == "template dir") {
            // Show the current templates directory
            std::cout << "Templates directory: " 
                     << template_manager.get_templates_directory() << std::endl;
        } else if (line.substr(0, 13) == "template dir ") {
            // Set the templates directory
            std::string dir = line.substr(13);
            try {
                template_manager.set_templates_directory(dir);
                Logger::getInstance().log(LogLevel::INFO, "Templates directory set to: ", dir);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to set templates directory: ", e.what());
            }
        } else if (line == "categories") {
            // List categories
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
            // Create a new category
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
        } else {
            // Add line to the current query
            if (!current_query.empty()) {
                current_query += "\n";
            }
            current_query += line;
        }
    }
}

// Process a query file
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