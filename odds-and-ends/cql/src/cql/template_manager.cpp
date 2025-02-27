// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/template_manager.hpp"
#include "../../include/cql/cql.hpp"
#include "../../../headers/project_utils.hpp"

#include <fstream>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace cql {

TemplateManager::TemplateManager() {
    // Default templates directory is in the user's home directory
    const char* home_dir = getenv("HOME");
    if (home_dir) {
        m_templates_dir = std::string(home_dir) + "/.cql/templates";
    } else {
        m_templates_dir = "./cql_templates";
    }
    ensure_templates_directory();
}

TemplateManager::TemplateManager(const std::string& template_dir) 
    : m_templates_dir(template_dir) {
    ensure_templates_directory();
}

void TemplateManager::save_template(const std::string& name, const std::string& content) {
    // Ensure the template has a valid name
    if (name.empty()) {
        throw std::runtime_error("Template name cannot be empty");
    }
    
    // Get the full path
    std::string template_path = get_template_path(name);
    
    try {
        // Write the content to the file
        util::write_file(template_path, content);
        Logger::getInstance().log(LogLevel::INFO, "Template saved: ", name);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to save template: " + std::string(e.what()));
    }
}

std::string TemplateManager::load_template(const std::string& name) {
    // Get the full path
    std::string template_path = get_template_path(name);
    
    // Check if the template exists
    if (!fs::exists(template_path)) {
        throw std::runtime_error("Template not found: " + name);
    }
    
    try {
        // Read the template content
        std::string content = util::read_file(template_path);
        Logger::getInstance().log(LogLevel::INFO, "Template loaded: ", name);
        return content;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load template: " + std::string(e.what()));
    }
}

TemplateManager::TemplateMetadata TemplateManager::get_template_metadata(const std::string& name) {
    // Get the full path
    std::string template_path = get_template_path(name);
    
    // Check if the template exists
    if (!fs::exists(template_path)) {
        throw std::runtime_error("Template not found: " + name);
    }
    
    try {
        // Read the template content
        std::string content = util::read_file(template_path);
        
        // Get the file's last modification time
        auto last_write_time = fs::last_write_time(template_path);
        auto last_write_time_t = std::chrono::system_clock::to_time_t(
            std::chrono::file_clock::to_sys(last_write_time));
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&last_write_time_t), "%Y-%m-%d %H:%M:%S");
        
        // Create and return the metadata
        TemplateMetadata metadata{
            .name = name,
            .description = extract_description(content),
            .variables = extract_variables(content),
            .last_modified = ss.str()
        };
        
        return metadata;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to get template metadata: " + std::string(e.what()));
    }
}

std::vector<std::string> TemplateManager::list_templates() {
    std::vector<std::string> templates;
    
    try {
        // Iterate through all files in the templates directory
        for (const auto& entry : fs::recursive_directory_iterator(m_templates_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".cql") {
                // Get the relative path from the templates directory
                std::string rel_path = entry.path().lexically_relative(m_templates_dir).string();
                templates.push_back(rel_path);
            }
        }
        
        // Sort the templates alphabetically
        std::sort(templates.begin(), templates.end());
        
        return templates;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to list templates: ", e.what());
        return templates; // Return empty list on error
    }
}

bool TemplateManager::delete_template(const std::string& name) {
    // Get the full path
    std::string template_path = get_template_path(name);
    
    // Check if the template exists
    if (!fs::exists(template_path)) {
        Logger::getInstance().log(LogLevel::ERROR, "Template not found: ", name);
        return false;
    }
    
    try {
        // Delete the file
        fs::remove(template_path);
        Logger::getInstance().log(LogLevel::INFO, "Template deleted: ", name);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to delete template: ", e.what());
        return false;
    }
}

std::string TemplateManager::instantiate_template(
    const std::string& name, 
    const std::map<std::string, std::string>& variables
) {
    // Load the template content
    std::string content = load_template(name);
    
    // Create variable declarations at the top of the template
    std::string variables_section;
    for (const auto& [var_name, var_value] : variables) {
        variables_section += "@variable \"" + var_name + "\" \"" + var_value + "\"\n";
    }
    
    // Prepend the variables to the template content
    if (!variables_section.empty()) {
        content = variables_section + "\n" + content;
    }
    
    return content;
}

std::string TemplateManager::get_templates_directory() const {
    return m_templates_dir;
}

void TemplateManager::set_templates_directory(const std::string& dir) {
    m_templates_dir = dir;
    ensure_templates_directory();
}

bool TemplateManager::create_category(const std::string& category) {
    // Create a directory for the category
    std::string category_path = m_templates_dir + "/" + category;
    
    try {
        fs::create_directories(category_path);
        Logger::getInstance().log(LogLevel::INFO, "Category created: ", category);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to create category: ", e.what());
        return false;
    }
}

std::vector<std::string> TemplateManager::list_categories() {
    std::vector<std::string> categories;
    
    try {
        // Iterate through all directories in the templates directory
        for (const auto& entry : fs::directory_iterator(m_templates_dir)) {
            if (entry.is_directory()) {
                categories.push_back(entry.path().filename().string());
            }
        }
        
        // Sort the categories alphabetically
        std::sort(categories.begin(), categories.end());
        
        return categories;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to list categories: ", e.what());
        return categories; // Return empty list on error
    }
}

std::string TemplateManager::get_template_path(const std::string& name) const {
    // Check if the name already has a .cql extension
    std::string filename = name;
    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".cql") {
        filename += ".cql";
    }
    
    // Combine the templates directory with the filename
    return m_templates_dir + "/" + filename;
}

void TemplateManager::ensure_templates_directory() {
    try {
        // Create the templates directory if it doesn't exist
        if (!fs::exists(m_templates_dir)) {
            fs::create_directories(m_templates_dir);
            Logger::getInstance().log(LogLevel::INFO, "Created templates directory: ", m_templates_dir);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create templates directory: " + std::string(e.what()));
    }
}

std::vector<std::string> TemplateManager::extract_variables(const std::string& content) {
    std::vector<std::string> variables;
    std::regex variable_regex(R"(@variable\s+"([^"]+)"\s+"[^"]+")");
    
    std::string::const_iterator search_start(content.cbegin());
    std::smatch match;
    
    // Find all variable declarations
    while (std::regex_search(search_start, content.cend(), match, variable_regex)) {
        if (match.size() > 1) {
            variables.push_back(match[1].str());
        }
        search_start = match.suffix().first;
    }
    
    // Also look for variable references
    std::regex reference_regex(R"(\${([^}]+)})");
    search_start = content.cbegin();
    
    while (std::regex_search(search_start, content.cend(), match, reference_regex)) {
        if (match.size() > 1) {
            std::string var_name = match[1].str();
            if (std::find(variables.begin(), variables.end(), var_name) == variables.end()) {
                variables.push_back(var_name);
            }
        }
        search_start = match.suffix().first;
    }
    
    return variables;
}

std::string TemplateManager::extract_description(const std::string& content) {
    // Try to extract a description from @description directive
    std::regex desc_regex(R"(@description\s+"([^"]+)")");
    std::smatch match;
    
    if (std::regex_search(content, match, desc_regex) && match.size() > 1) {
        return match[1].str();
    }
    
    // If no description directive found, return the first line as a fallback
    size_t eol = content.find('\n');
    if (eol != std::string::npos) {
        return content.substr(0, eol);
    }
    
    return "No description available";
}

} // namespace cql