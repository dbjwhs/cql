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
#include <set>

namespace fs = std::filesystem;

namespace cql {

TemplateManager::TemplateManager() {
    // default templates directory is in the user's home directory
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
    // ensure the template has a valid name
    if (name.empty()) {
        throw std::runtime_error("Template name cannot be empty");
    }
    
    // ensure templates directory structure is valid
    if (!validate_template_directory()) {
        if (!repair_template_directory()) {
            throw std::runtime_error("Template directory structure is invalid and could not be repaired");
        }
    }
    
    std::string template_path;
    
    // check if the name contains a category specification with category/template format
    if (name.find('/') != std::string::npos) {
        // split the name into category and template name
        std::string category = name.substr(0, name.find('/'));
        
        // ensure the category directory exists
        fs::path category_path = fs::path(m_templates_dir) / category;
        if (!fs::exists(category_path)) {
            try {
                fs::create_directory(category_path);
                Logger::getInstance().log(LogLevel::INFO, "Created category directory: ", category);
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to create category directory: " + std::string(e.what()));
            }
        }
        
        // get the template path including the category
        template_path = get_template_path(name);
    } else {
        // if no category specified, save to the user directory by default
        std::string filename = name;
        if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".cql") {
            filename += ".cql";
        }
        
        template_path = m_templates_dir + "/user/" + filename;
    }
    
    try {
        // ensure parent directory exists (for nested categories)
        fs::path parent_dir = fs::path(template_path).parent_path();
        if (!fs::exists(parent_dir)) {
            fs::create_directories(parent_dir);
        }
        
        // write the content to the file
        util::write_file(template_path, content);
        Logger::getInstance().log(LogLevel::INFO, "Template saved: ", name);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to save template: " + std::string(e.what()));
    }
}

std::string TemplateManager::load_template(const std::string& name) {
    // get the full path
    std::string template_path = get_template_path(name);
    
    // check if the template exists
    if (!fs::exists(template_path)) {
        throw std::runtime_error("Template not found: " + name);
    }
    
    try {
        // read the template content
        std::string content = util::read_file(template_path);
        Logger::getInstance().log(LogLevel::INFO, "Template loaded: ", name);
        return content;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load template: " + std::string(e.what()));
    }
}

TemplateManager::TemplateMetadata TemplateManager::get_template_metadata(const std::string& name) {
    // get the full path
    std::string template_path = get_template_path(name);
    
    // check if the template exists
    if (!fs::exists(template_path)) {
        throw std::runtime_error("Template not found: " + name);
    }
    
    try {
        // read the template content
        std::string content = util::read_file(template_path);
        
        // get the file's last modification time
        auto last_write_time = fs::last_write_time(template_path);
        auto sys_time = std::chrono::file_clock::to_sys(last_write_time);
        auto last_write_time_t = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(sys_time));
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&last_write_time_t), "%Y-%m-%d %H:%M:%S");
        
        // create and return the metadata
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
    
    // ensure templates directory is valid
    if (!validate_template_directory()) {
        if (!repair_template_directory()) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "Failed to list templates: template directory structure is invalid");
            return templates;
        }
    }
    
    try {
        // create a set to keep track of added templates
        std::set<std::string> added_templates;
        
        // first get all templates from the common directory
        if (fs::exists(fs::path(m_templates_dir) / "common")) {
            for (const auto& entry : fs::recursive_directory_iterator(fs::path(m_templates_dir) / "common")) {
                if (entry.is_regular_file() && entry.path().extension() == ".cql") {
                    // format path as common/template
                    fs::path rel_path = entry.path().lexically_relative(m_templates_dir);
                    std::string template_name = rel_path.string();
                    templates.push_back(template_name);
                    added_templates.insert(template_name);
                }
            }
        }
        
        // next get all templates from the user directory
        if (fs::exists(fs::path(m_templates_dir) / "user")) {
            for (const auto& entry : fs::recursive_directory_iterator(fs::path(m_templates_dir) / "user")) {
                if (entry.is_regular_file() && entry.path().extension() == ".cql") {
                    // format path as user/template
                    fs::path rel_path = entry.path().lexically_relative(m_templates_dir);
                    std::string template_name = rel_path.string();
                    templates.push_back(template_name);
                    added_templates.insert(template_name);
                }
            }
        }
        
        // finally get templates from any other directories (legacy support)
        for (const auto& entry : fs::directory_iterator(m_templates_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".cql") {
                // add only if not an internal file
                if (entry.path().filename().string() != "README.txt" && 
                    entry.path().filename().string()[0] != '.') {
                    std::string template_name = entry.path().filename().string();
                    
                    // add only if we haven't encountered this template yet
                    if (added_templates.find(template_name) == added_templates.end()) {
                        templates.push_back(template_name);
                        added_templates.insert(template_name);
                    }
                }
            } else if (entry.is_directory() && 
                      entry.path().filename() != "common" && 
                      entry.path().filename() != "user") {
                // handle custom categories (not common or user)
                for (const auto& sub_entry : fs::recursive_directory_iterator(entry.path())) {
                    if (sub_entry.is_regular_file() && sub_entry.path().extension() == ".cql") {
                        // format path as category/template
                        fs::path rel_path = sub_entry.path().lexically_relative(m_templates_dir);
                        std::string template_name = rel_path.string();
                        
                        // add only if we haven't encountered this template yet
                        if (added_templates.find(template_name) == added_templates.end()) {
                            templates.push_back(template_name);
                            added_templates.insert(template_name);
                        }
                    }
                }
            }
        }
        
        // sort the templates alphabetically
        std::sort(templates.begin(), templates.end(), [](const std::string& a, const std::string& b) {
            // First sort by category, then by template name
            std::string a_category = a.find('/') != std::string::npos ? a.substr(0, a.find('/')) : "";
            std::string b_category = b.find('/') != std::string::npos ? b.substr(0, b.find('/')) : "";
            
            // Put common category first, then user, then others alphabetically
            if (a_category == "common" && b_category != "common") return true;
            if (a_category != "common" && b_category == "common") return false;
            if (a_category == "user" && b_category != "user" && b_category != "common") return true;
            if (a_category != "user" && a_category != "common" && b_category == "user") return false;
            
            // If categories are the same, sort by template name
            if (a_category == b_category) {
                std::string a_name = a.find('/') != std::string::npos ? a.substr(a.find('/') + 1) : a;
                std::string b_name = b.find('/') != std::string::npos ? b.substr(b.find('/') + 1) : b;
                return a_name < b_name;
            }
            
            // Otherwise sort by category
            return a_category < b_category;
        });
        
        return templates;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to list templates: ", e.what());
        return templates; // return empty list on error
    }
}

bool TemplateManager::delete_template(const std::string& name) {
    // get the full path
    std::string template_path = get_template_path(name);
    
    // check if the template exists
    if (!fs::exists(template_path)) {
        Logger::getInstance().log(LogLevel::ERROR, "Template not found: ", name);
        return false;
    }
    
    try {
        // delete the file
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
    // load the template content
    std::string content = load_template(name);
    
    // create variable declarations at the top of the template
    std::string variables_section;
    for (const auto& [var_name, var_value] : variables) {
        variables_section += "@variable \"" + var_name + "\" \"" + var_value + "\"\n";
    }
    
    // prepend the variables to the template content
    if (!variables_section.empty()) {
        content = variables_section + "\n" + content;
    }
    
    // replace all variable references with their values
    std::string result = replace_variables(content, variables);
    
    return result;
}

std::string TemplateManager::get_templates_directory() const {
    return m_templates_dir;
}

void TemplateManager::set_templates_directory(const std::string& dir) {
    m_templates_dir = dir;
    ensure_templates_directory();
}

bool TemplateManager::create_category(const std::string& category) {
    // create a directory for the category
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
        // iterate through all directories in the templates directory
        for (const auto& entry : fs::directory_iterator(m_templates_dir)) {
            if (entry.is_directory()) {
                categories.push_back(entry.path().filename().string());
            }
        }
        
        // sort the categories alphabetically
        std::sort(categories.begin(), categories.end());
        
        return categories;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to list categories: ", e.what());
        return categories; // return empty list on error
    }
}

std::string TemplateManager::get_template_path(const std::string& name) const {
    // check if the name already has a .cql extension
    std::string filename = name;
    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".cql") {
        filename += ".cql";
    }
    
    // check if the name includes a category using category/template format
    if (filename.find('/') != std::string::npos) {
        // combine the templates directory with the category/template path
        return m_templates_dir + "/" + filename;
    }
    
    // first check in user directory (most common case)
    std::string user_path = m_templates_dir + "/user/" + filename;
    if (fs::exists(user_path)) {
        return user_path;
    }
    
    // then check in common directory
    std::string common_path = m_templates_dir + "/common/" + filename;
    if (fs::exists(common_path)) {
        return common_path;
    }
    
    // check if it exists directly under the templates directory (legacy support)
    std::string root_path = m_templates_dir + "/" + filename;
    if (fs::exists(root_path)) {
        return root_path;
    }
    
    // if not found, default to user directory
    return m_templates_dir + "/user/" + filename;
}

void TemplateManager::ensure_templates_directory() {
    try {
        // create the templates directory if it doesn't exist
        if (!fs::exists(m_templates_dir)) {
            fs::create_directories(m_templates_dir);
            Logger::getInstance().log(LogLevel::INFO, "Created templates directory: ", m_templates_dir);
            
            // initialize the directory with standard structure
            initialize_template_structure();
        } else {
            // validate existing directory structure
            if (!validate_template_directory()) {
                Logger::getInstance().log(LogLevel::ERROR, 
                    "Template directory has issues. Attempting repair...");
                
                if (!repair_template_directory()) {
                    Logger::getInstance().log(LogLevel::ERROR, 
                        "Failed to repair template directory: ", m_templates_dir);
                }
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to create templates directory: " + std::string(e.what()));
    }
}

bool TemplateManager::validate_template_directory() const {
    bool valid = true;
    
    try {
        // check if directory exists
        if (!fs::exists(m_templates_dir)) {
            Logger::getInstance().log(LogLevel::ERROR, "Template directory does not exist: ", m_templates_dir);
            return false;
        }
        
        // check if it's actually a directory
        if (!fs::is_directory(m_templates_dir)) {
            Logger::getInstance().log(LogLevel::ERROR, "Template path is not a directory: ", m_templates_dir);
            return false;
        }
        
        // check if it's writable
        fs::path test_file = fs::path(m_templates_dir) / ".write_test";
        try {
            std::ofstream test(test_file);
            if (!test.is_open()) {
                Logger::getInstance().log(LogLevel::ERROR, "Template directory is not writable: ", m_templates_dir);
                valid = false;
            } else {
                test.close();
                fs::remove(test_file);
            }
        } catch (...) {
            Logger::getInstance().log(LogLevel::ERROR, "Failed to write to template directory: ", m_templates_dir);
            valid = false;
        }
        
        // check for required directories
        if (!fs::exists(fs::path(m_templates_dir) / "common") || 
            !fs::is_directory(fs::path(m_templates_dir) / "common")) {
            Logger::getInstance().log(LogLevel::ERROR, "Missing 'common' category in template directory");
            valid = false;
        }
        
        if (!fs::exists(fs::path(m_templates_dir) / "user") || 
            !fs::is_directory(fs::path(m_templates_dir) / "user")) {
            Logger::getInstance().log(LogLevel::ERROR, "Missing 'user' category in template directory");
            valid = false;
        }
        
        return valid;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error validating template directory: ", e.what());
        return false;
    }
}

void TemplateManager::initialize_template_structure() {
    try {
        // create standard subdirectories
        fs::create_directory(fs::path(m_templates_dir) / "common");
        fs::create_directory(fs::path(m_templates_dir) / "user");
        
        // create a README file explaining the structure
        std::string readme_path = (fs::path(m_templates_dir) / "README.txt").string();
        std::ofstream readme(readme_path);
        if (readme.is_open()) {
            readme << "CQL Template Directory Structure\n";
            readme << "===============================\n\n";
            readme << "This directory contains CQL templates organized as follows:\n\n";
            readme << "- common/ : Standard templates that ship with CQL\n";
            readme << "- user/   : User-created templates\n\n";
            readme << "You can also create your own categories as subdirectories.\n";
            readme << "Each template should be a .cql file.\n";
            readme.close();
            
            Logger::getInstance().log(LogLevel::INFO, "Created template directory structure");
        }
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to initialize template structure: ", e.what());
    }
}

bool TemplateManager::repair_template_directory() {
    try {
        // ensure main directory exists
        if (!fs::exists(m_templates_dir)) {
            fs::create_directories(m_templates_dir);
        }
        
        // ensure common subdirectory exists
        if (!fs::exists(fs::path(m_templates_dir) / "common") || 
            !fs::is_directory(fs::path(m_templates_dir) / "common")) {
            fs::create_directory(fs::path(m_templates_dir) / "common");
        }
        
        // ensure user subdirectory exists
        if (!fs::exists(fs::path(m_templates_dir) / "user") || 
            !fs::is_directory(fs::path(m_templates_dir) / "user")) {
            fs::create_directory(fs::path(m_templates_dir) / "user");
        }
        
        // recreate README if missing
        std::string readme_path = (fs::path(m_templates_dir) / "README.txt").string();
        if (!fs::exists(readme_path)) {
            std::ofstream readme(readme_path);
            if (readme.is_open()) {
                readme << "CQL Template Directory Structure\n";
                readme << "===============================\n\n";
                readme << "This directory contains CQL templates organized as follows:\n\n";
                readme << "- common/ : Standard templates that ship with CQL\n";
                readme << "- user/   : User-created templates\n\n";
                readme << "You can also create your own categories as subdirectories.\n";
                readme << "Each template should be a .cql file.\n";
                readme.close();
            }
        }
        
        Logger::getInstance().log(LogLevel::INFO, "Repaired template directory structure");
        return validate_template_directory();
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to repair template directory: ", e.what());
        return false;
    }
}

std::vector<std::string> TemplateManager::extract_variables(const std::string& content) {
    std::vector<std::string> variables;
    std::regex variable_regex("@variable\\s+\"([^\"]+)\"\\s+\"[^\"]*\"");
    
    std::string::const_iterator search_start(content.cbegin());
    std::smatch match;
    
    // find all variable declarations
    while (std::regex_search(search_start, content.cend(), match, variable_regex)) {
        if (match.size() > 1) {
            variables.push_back(match[1].str());
        }
        search_start = match.suffix().first;
    }
    
    // also look for variable references
    std::regex reference_regex("\\$\\{([^}]+)\\}");
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
    // try to extract a description from @description directive
    std::regex desc_regex("@description\\s+\"([^\"]*)\"");
    std::smatch match;
    
    if (std::regex_search(content, match, desc_regex) && match.size() > 1) {
        return match[1].str();
    }
    
    // if no description directive found, return the first line as a fallback
    size_t eol = content.find('\n');
    if (eol != std::string::npos) {
        return content.substr(0, eol);
    }
    
    return "No description available";
}

std::string TemplateManager::replace_variables(
    const std::string& content,
    const std::map<std::string, std::string>& variables
) {
    std::string result = content;
    std::regex variable_ref_regex("\\$\\{([^}]+)\\}");
    
    // first collect all variables from the content (declared with @variable)
    auto all_variables = collect_variables(content);
    
    // add or override with provided variables
    for (const auto& [name, value] : variables) {
        all_variables[name] = value;
    }
    
    // replace all ${var} occurrences with their values
    std::string::const_iterator search_start(result.cbegin());
    std::smatch match;
    std::string output;
    std::size_t last_pos = 0;
    
    while (std::regex_search(search_start, result.cend(), match, variable_ref_regex)) {
        // get the variable name from the match
        std::string var_name = match[1].str();
        
        // get positions for replacement
        std::size_t start_pos = std::distance(result.cbegin(), match[0].first);
        std::size_t end_pos = std::distance(result.cbegin(), match[0].second);
        
        // append text before the variable reference
        output.append(result.substr(last_pos, start_pos - last_pos));
        
        // append the variable value or the original reference if not found
        if (all_variables.find(var_name) != all_variables.end()) {
            output.append(all_variables[var_name]);
            Logger::getInstance().log(LogLevel::INFO, "Replaced variable: ", var_name, " with: ", all_variables[var_name]);
        } else {
            // keep the original reference
            output.append(match[0].str());
            Logger::getInstance().log(LogLevel::ERROR, "Variable not found: ", var_name);
        }
        
        // update positions for next iteration
        last_pos = end_pos;
        search_start = match[0].second;
    }
    
    // append remaining content
    if (last_pos < result.length()) {
        output.append(result.substr(last_pos));
    }
    
    return output;
}

std::map<std::string, std::string> TemplateManager::collect_variables(const std::string& content) {
    std::map<std::string, std::string> variables;
    std::regex variable_decl_regex("@variable\\s+\"([^\"]*)\"\\s+\"([^\"]*)\"");
    
    std::string::const_iterator search_start(content.cbegin());
    std::smatch match;
    
    // extract all @variable declarations
    while (std::regex_search(search_start, content.cend(), match, variable_decl_regex)) {
        if (match.size() > 2) {
            std::string name = match[1].str();
            std::string value = match[2].str();
            variables[name] = value;
            Logger::getInstance().log(LogLevel::INFO, "Found variable declaration: ", name, "=", value);
        }
        search_start = match.suffix().first;
    }
    
    return variables;
}

} // namespace cql