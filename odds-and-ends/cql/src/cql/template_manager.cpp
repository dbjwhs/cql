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
        
        // Extract parent template if any
        auto parent = extract_parent_template(content);
        
        // create and return the metadata
        TemplateMetadata metadata{
            .name = name,
            .description = extract_description(content),
            .variables = extract_variables(content),
            .last_modified = ss.str(),
            .parent = parent
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
    // load the template content with inheritance support
    std::string content = load_template_with_inheritance(name);
    
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
        // Create standard directory structure
        ensure_standard_directories();
        
        // Create README file
        create_readme_file();
        Logger::getInstance().log(LogLevel::INFO, "Created template directory structure");
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to initialize template structure: ", e.what());
    }
}

bool TemplateManager::repair_template_directory() {
    try {
        // Ensure main directory exists
        if (!fs::exists(m_templates_dir)) {
            fs::create_directories(m_templates_dir);
        }
        
        // Ensure standard directory structure
        ensure_standard_directories();
        
        // Recreate README if missing
        std::string readme_path = (fs::path(m_templates_dir) / "README.txt").string();
        if (!fs::exists(readme_path)) {
            create_readme_file();
        }
        
        Logger::getInstance().log(LogLevel::INFO, "Repaired template directory structure");
        return validate_template_directory();
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to repair template directory: ", e.what());
        return false;
    }
}

std::vector<std::string> TemplateManager::extract_variables(const std::string& content) {
    // Get all declared variables
    auto declared_vars = cql::util::extract_regex_group_values(
        content,
        "@variable\\s+\"([^\"]+)\"\\s+\"[^\"]*\"",
        1
    );
    
    // Get all referenced variables
    auto referenced_vars = cql::util::extract_regex_group_values(
        content,
        "\\$\\{([^}]+)\\}",
        1
    );
    
    // Combine both sets into a vector
    std::vector<std::string> variables;
    variables.insert(variables.end(), declared_vars.begin(), declared_vars.end());
    
    // Add referenced variables that aren't already in the list
    for (const auto& var : referenced_vars) {
        if (declared_vars.find(var) == declared_vars.end()) {
            variables.push_back(var);
        }
    }
    
    return variables;
}

std::string TemplateManager::extract_description(const std::string& content) {
    // Try to extract a description from @description directive
    auto descriptions = cql::util::extract_regex_group_values(
        content,
        "@description\\s+\"([^\"]*)\"",
        1
    );
    
    if (!descriptions.empty()) {
        return *descriptions.begin();
    }
    
    // If no description directive found, return the first line as a fallback
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
    
    // Extract all @variable declarations with their values
    auto variable_matches = cql::util::extract_regex_matches(
        content,
        "@variable\\s+\"([^\"]*)\"\\s+\"([^\"]*)\"",
        2
    );
    
    for (const auto& match : variable_matches) {
        if (match.size() > 2) {
            std::string name = match[1];
            std::string value = match[2];
            variables[name] = value;
            Logger::getInstance().log(LogLevel::INFO, "Found variable declaration: ", name, "=", value);
        }
    }
    
    return variables;
}

// Extract parent template name if this template inherits from another
std::optional<std::string> TemplateManager::extract_parent_template(const std::string& content) {
    auto parents = cql::util::extract_regex_group_values(
        content,
        "@inherit\\s+\"([^\"]*)\"",
        1
    );
    
    if (!parents.empty()) {
        std::string parent = *parents.begin();
        Logger::getInstance().log(LogLevel::INFO, "Found parent template: ", parent);
        return parent;
    }
    
    return std::nullopt;
}

// Create a new template that inherits from another
void TemplateManager::create_inherited_template(const std::string& name, 
                                               const std::string& parent_name, 
                                               const std::string& content) {
    // Verify parent template exists
    std::string parent_path = get_template_path(parent_name);
    if (!fs::exists(parent_path)) {
        throw std::runtime_error("Parent template not found: " + parent_name);
    }
    
    // Add inheritance directive if not already present
    std::regex inherit_regex("@inherit\\s+\"([^\"]*)\"");
    std::string modified_content = content;
    
    if (!std::regex_search(content, inherit_regex)) {
        // Add @inherit directive at the beginning of the content
        modified_content = "@inherit \"" + parent_name + "\"\n" + content;
    }
    
    // Save the template
    save_template(name, modified_content);
    Logger::getInstance().log(LogLevel::INFO, "Created template '", name, 
                              "' inheriting from '", parent_name, "'");
}

// Get a list of parent templates (inheritance chain)
std::vector<std::string> TemplateManager::get_inheritance_chain(const std::string& name) {
    std::vector<std::string> chain;
    std::set<std::string> visited; // To detect circular inheritance
    
    std::string current = name;
    while (!current.empty()) {
        // Add to chain
        chain.push_back(current);
        
        // Mark as visited
        visited.insert(current);
        
        try {
            // Load the template content
            std::string content = load_template(current);
            
            // Extract parent template name
            auto parent = extract_parent_template(content);
            if (!parent.has_value() || parent.value().empty()) {
                break; // No parent, end of chain
            }
            
            current = parent.value();
            
            // Check for circular inheritance
            if (visited.find(current) != visited.end()) {
                std::string cycle = "Inheritance cycle: ";
                for (const auto& templ : visited) {
                    cycle += templ + " -> ";
                }
                cycle += current;
                
                Logger::getInstance().log(LogLevel::ERROR, 
                    "Circular inheritance detected for template: ", name);
                Logger::getInstance().log(LogLevel::ERROR, 
                    "Error in inheritance chain for template '", name, "': Circular inheritance detected: ", cycle);
                
                throw std::runtime_error("Circular inheritance detected: " + cycle);
            }
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, 
                "Error in inheritance chain for template '", name, "': ", e.what());
            // Rethrow to propagate the error
            throw;
        }
    }
    
    // Reverse to get base template first, followed by derived templates
    std::reverse(chain.begin(), chain.end());
    return chain;
}

// Load a template and merge in inherited content from parent templates
std::string TemplateManager::load_template_with_inheritance(const std::string& name) {
    // Get the inheritance chain
    std::vector<std::string> chain = get_inheritance_chain(name);
    
    if (chain.empty()) {
        throw std::runtime_error("Failed to resolve inheritance chain for template: " + name);
    }
    
    // Start with empty content
    std::string merged_content;
    
    // Process each template in the chain, starting from base class
    for (const auto& template_name : chain) {
        std::string template_content = load_template(template_name);
        
        if (merged_content.empty()) {
            // For the first template (base class), use its content directly
            merged_content = template_content;
        } else {
            // For derived templates, merge with existing content
            merged_content = merge_template_content(merged_content, template_content);
        }
    }
    
    return merged_content;
}

// Merge parent template content with child template content
std::string TemplateManager::merge_template_content(const std::string& parent_content, 
                                                  const std::string& child_content) {
    // Strip @inherit directive from child content
    std::regex inherit_regex("@inherit\\s+\"[^\"]*\"\\s*\n?");
    std::string stripped_child = std::regex_replace(child_content, inherit_regex, "");
    
    // Collect variables from both templates
    auto parent_vars = collect_variables(parent_content);
    auto child_vars = collect_variables(child_content);
    
    // Child variables override parent variables
    std::map<std::string, std::string> merged_vars = parent_vars;
    for (const auto& [name, value] : child_vars) {
        merged_vars[name] = value;
    }
    
    // Create the variable declarations section
    std::string variables_section;
    for (const auto& [name, value] : merged_vars) {
        variables_section += "@variable \"" + name + "\" \"" + value + "\"\n";
    }
    
    // Strip variable declarations from parent content
    std::regex var_regex("@variable\\s+\"[^\"]*\"\\s+\"[^\"]*\"\\s*\n?");
    std::string parent_without_vars = std::regex_replace(parent_content, var_regex, "");
    
    // Strip variable declarations from child content
    std::string child_without_vars = std::regex_replace(stripped_child, var_regex, "");
    
    // Combine the content
    return variables_section + "\n" + parent_without_vars + "\n" + child_without_vars;
}

// extract example usage from template content
std::string TemplateManager::extract_example(const std::string& content) {
    // look for @example directive in the content
    std::regex example_regex("@example\\s+\"([^\"]*)\"");
    std::smatch match;
    
    if (std::regex_search(content, match, example_regex) && match.size() > 1) {
        return match[1].str();
    }
    
    // if no explicit example is found, try to extract the first query in the content
    std::regex query_regex("(SELECT|INSERT|UPDATE|DELETE|CREATE|ALTER|DROP|WITH)[^;]+;");
    if (std::regex_search(content, match, query_regex) && match.size() > 0) {
        return match[0].str();
    }
    
    return "No example available";
}

// generate documentation for a template
std::string TemplateManager::generate_template_documentation(const std::string& name) {
    try {
        // get the template metadata
        TemplateMetadata metadata = get_template_metadata(name);
        
        // load the template content
        std::string content = load_template(name);
        
        // format the template documentation using common helper
        return format_template_markdown(metadata, content);
    } catch (const std::exception& e) {
        return "Error generating documentation: " + std::string(e.what());
    }
}

// generate documentation for all templates
std::string TemplateManager::generate_all_template_documentation() {
    try {
        // get all templates
        std::vector<std::string> templates = list_templates();
        
        if (templates.empty()) {
            return "# CQL Template Documentation\n\nNo templates found.";
        }
        
        // organize templates by category
        std::map<std::string, std::vector<std::string>> templates_by_category;
        
        for (const auto& templ : templates) {
            std::string category = "uncategorized";
            
            // check if template has a category in its name
            if (templ.find('/') != std::string::npos) {
                category = templ.substr(0, templ.find('/'));
            }
            
            templates_by_category[category].push_back(templ);
        }
        
        // generate documentation
        std::stringstream doc;
        
        // title and index
        doc << "# CQL Template Documentation\n\n";
        doc << "## Overview\n\n";
        
        doc << "Total templates: " << templates.size() << "\n\n";
        
        doc << "### Categories\n\n";
        for (const auto& [category, templ_list] : templates_by_category) {
            doc << "- [" << category << " (" << templ_list.size() << ")](#" << category << ")\n";
        }
        doc << "\n";
        
        // templates table of contents
        doc << "### Templates Index\n\n";
        for (const auto& templ : templates) {
            // create anchor for template name - replace slashes with dashes for anchors
            std::string anchor = templ;
            std::replace(anchor.begin(), anchor.end(), '/', '-');
            
            // if template has .cql extension, remove it for display
            std::string display_name = templ;
            if (display_name.length() > 4 && display_name.substr(display_name.length() - 4) == ".cql") {
                display_name = display_name.substr(0, display_name.length() - 4);
            }
            
            doc << "- [" << display_name << "](#" << anchor << ")\n";
        }
        doc << "\n";
        
        // documentation for each category and its templates
        for (const auto& [category, templ_list] : templates_by_category) {
            doc << "## " << category << "\n\n";
            
            // info about templates in this category
            for (const auto& templ : templ_list) {
                try {
                    // add a separator before each template
                    doc << "<a id=\"" << templ << "\"></a>\n\n";
                    doc << "---\n\n";
                    
                    // get template metadata and content
                    TemplateMetadata metadata = get_template_metadata(templ);
                    std::string content = load_template(templ);
                    
                    // add template documentation using the common formatter
                    doc << format_template_markdown(metadata, content) << "\n\n";
                } catch (const std::exception& e) {
                    doc << "### " << templ << "\n\n";
                    doc << "Error generating documentation: " << e.what() << "\n\n";
                }
            }
        }
        
        // generation timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream timestamp;
        timestamp << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        
        doc << "---\n\n";
        doc << "Documentation generated on " << timestamp.str() << "\n";
        
        return doc.str();
    } catch (const std::exception& e) {
        return "Error generating documentation: " + std::string(e.what());
    }
}

// export documentation to a file
bool TemplateManager::export_documentation(const std::string& output_path, const std::string& format) {
    try {
        // generate the documentation
        std::string doc_content = generate_all_template_documentation();
        
        // convert to requested format if not markdown
        std::string final_content;
        std::string extension;
        
        if (format == "markdown" || format == "md") {
            // no conversion needed
            final_content = doc_content;
            extension = ".md";
        } else if (format == "html") {
            // simple markdown to html conversion
            std::stringstream html;
            
            // html header
            html << "<!DOCTYPE html>\n"
                 << "<html>\n"
                 << "<head>\n"
                 << "    <meta charset=\"UTF-8\">\n"
                 << "    <title>CQL Template Documentation</title>\n"
                 << "    <style>\n"
                 << "        body { font-family: Arial, sans-serif; line-height: 1.6; margin: 20px; }\n"
                 << "        h1 { color: #333366; }\n"
                 << "        h2 { color: #336699; margin-top: 30px; }\n"
                 << "        h3 { color: #0099cc; }\n"
                 << "        pre { background-color: #f5f5f5; padding: 10px; border-radius: 5px; }\n"
                 << "        code { font-family: monospace; }\n"
                 << "        table { border-collapse: collapse; width: 100%; }\n"
                 << "        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n"
                 << "        th { background-color: #f2f2f2; }\n"
                 << "    </style>\n"
                 << "</head>\n"
                 << "<body>\n";
            
            // simple markdown to html conversion for common elements
            std::string line;
            std::istringstream doc_stream(doc_content);
            bool in_code_block = false;
            bool in_table = false;
            
            while (std::getline(doc_stream, line)) {
                // code blocks
                if (line.find("```") == 0) {
                    if (in_code_block) {
                        html << "</code></pre>\n";
                        in_code_block = false;
                    } else {
                        html << "<pre><code>";
                        in_code_block = true;
                    }
                    continue;
                }
                
                if (in_code_block) {
                    // escape html in code blocks
                    std::string escaped = line;
                    escaped = std::regex_replace(escaped, std::regex("&"), "&amp;");
                    escaped = std::regex_replace(escaped, std::regex("<"), "&lt;");
                    escaped = std::regex_replace(escaped, std::regex(">"), "&gt;");
                    html << escaped << "\n";
                    continue;
                }
                
                // headings
                if (line.find("# ") == 0) {
                    html << "<h1>" << line.substr(2) << "</h1>\n";
                } else if (line.find("## ") == 0) {
                    html << "<h2>" << line.substr(3) << "</h2>\n";
                } else if (line.find("### ") == 0) {
                    html << "<h3>" << line.substr(4) << "</h3>\n";
                } 
                // tables
                else if (line.find("|") == 0) {
                    if (!in_table) {
                        html << "<table>\n";
                        in_table = true;
                    }
                    
                    // convert markdown table row to html
                    html << "  <tr>\n";
                    
                    // split by pipe and process each cell
                    size_t pos = 0;
                    size_t start = 1; // skip first pipe
                    std::string cell;
                    
                    while ((pos = line.find("|", start)) != std::string::npos) {
                        cell = line.substr(start, pos - start);
                        
                        // skip separator rows (---|---)
                        if (cell.find_first_not_of("- ") == std::string::npos) {
                            break;
                        }
                        
                        // determine if it's a header row
                        std::string tag = (line.find("---") != std::string::npos) ? "th" : "td";
                        html << "    <" << tag << ">" << cell << "</" << tag << ">\n";
                        
                        start = pos + 1;
                    }
                    
                    // get the last cell
                    if (start < line.length() - 1) {
                        cell = line.substr(start, line.length() - start - 1);
                        std::string tag = (line.find("---") != std::string::npos) ? "th" : "td";
                        html << "    <" << tag << ">" << cell << "</" << tag << ">\n";
                    }
                    
                    html << "  </tr>\n";
                    
                    // if this is a separator row, skip it
                    if (line.find("---") != std::string::npos) {
                        continue;
                    }
                } else if (in_table && line.empty()) {
                    html << "</table>\n";
                    in_table = false;
                }
                // links
                else if (line.find("[") != std::string::npos && line.find("](") != std::string::npos) {
                    std::string processed = line;
                    std::regex link_regex("\\[([^\\]]*)\\]\\(([^\\)]*)\\)");
                    processed = std::regex_replace(processed, link_regex, "<a href=\"$2\">$1</a>");
                    html << "<p>" << processed << "</p>\n";
                }
                // horizontal rule
                else if (line == "---") {
                    html << "<hr>\n";
                }
                // paragraph
                else {
                    if (!line.empty()) {
                        // check for bold/italic text
                        std::string processed = line;
                        processed = std::regex_replace(processed, std::regex("\\*\\*([^\\*]*)\\*\\*"), "<strong>$1</strong>");
                        processed = std::regex_replace(processed, std::regex("\\*([^\\*]*)\\*"), "<em>$1</em>");
                        
                        html << "<p>" << processed << "</p>\n";
                    } else {
                        html << "<br>\n";
                    }
                }
            }
            
            // close any open elements
            if (in_code_block) {
                html << "</code></pre>\n";
            }
            if (in_table) {
                html << "</table>\n";
            }
            
            // html footer
            html << "</body>\n</html>";
            
            final_content = html.str();
            extension = ".html";
        } else if (format == "text" || format == "txt") {
            // simple markdown to plain text conversion
            std::string processed = doc_content;
            
            // remove markdown formatting
            processed = std::regex_replace(processed, std::regex("#+\\s+"), ""); // headers
            processed = std::regex_replace(processed, std::regex("\\*\\*([^\\*]*)\\*\\*"), "$1"); // bold
            processed = std::regex_replace(processed, std::regex("\\*([^\\*]*)\\*"), "$1"); // italic
            processed = std::regex_replace(processed, std::regex("```[^`]*```"), ""); // code blocks
            processed = std::regex_replace(processed, std::regex("\\[([^\\]]*)\\]\\([^\\)]*\\)"), "$1"); // links
            
            final_content = processed;
            extension = ".txt";
        } else {
            // unsupported format, default to markdown
            Logger::getInstance().log(LogLevel::ERROR, 
                "unsupported documentation format '", format, "', defaulting to markdown");
            final_content = doc_content;
            extension = ".md";
        }
        
        // ensure the output file has the correct extension
        std::string final_path = output_path;
        if (fs::path(output_path).extension().empty()) {
            final_path += extension;
        }
        
        // create the directory if it doesn't exist
        fs::path output_dir = fs::path(final_path).parent_path();
        if (!output_dir.empty() && !fs::exists(output_dir)) {
            fs::create_directories(output_dir);
        }
        
        // write to the file
        std::ofstream outfile(final_path);
        if (!outfile.is_open()) {
            throw std::runtime_error("Failed to open output file: " + final_path);
        }
        
        outfile << final_content;
        outfile.close();
        
        Logger::getInstance().log(LogLevel::INFO, 
            "Documentation exported to ", final_path, " in ", format, " format");
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to export documentation: ", e.what());
        return false;
    }
}

void TemplateManager::create_readme_file() {
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
    }
}

void TemplateManager::ensure_standard_directories() {
    // Create common subdirectory if it doesn't exist
    if (!fs::exists(fs::path(m_templates_dir) / "common") || 
        !fs::is_directory(fs::path(m_templates_dir) / "common")) {
        fs::create_directory(fs::path(m_templates_dir) / "common");
    }
    
    // Create user subdirectory if it doesn't exist
    if (!fs::exists(fs::path(m_templates_dir) / "user") || 
        !fs::is_directory(fs::path(m_templates_dir) / "user")) {
        fs::create_directory(fs::path(m_templates_dir) / "user");
    }
}

// Format template documentation as markdown
std::string TemplateManager::format_template_markdown(const TemplateMetadata& metadata, const std::string& content) {
    // Extract example
    std::string example = extract_example(content);
    
    // Format the documentation
    std::stringstream doc;
    
    // Template name as title
    doc << "# " << metadata.name << "\n\n";
    
    // Description
    doc << "## Description\n\n" << metadata.description << "\n\n";
    
    // Last modified date
    doc << "**Last Modified:** " << metadata.last_modified << "\n\n";
    
    // Parent template if any
    if (metadata.parent.has_value() && !metadata.parent.value().empty()) {
        doc << "**Inherits From:** " << metadata.parent.value() << "\n\n";
    }
    
    // Variables section
    doc << "## Variables\n\n";
    if (metadata.variables.empty()) {
        doc << "This template has no variables.\n\n";
    } else {
        doc << "| Name | Description |\n";
        doc << "|------|-------------|\n";
        
        // Extract variable descriptions from content if available
        auto var_desc_matches = cql::util::extract_regex_matches(
            content,
            "@variable_description\\s+\"([^\"]*)\"\\s+\"([^\"]*)\"",
            2
        );
        
        std::map<std::string, std::string> var_descriptions;
        for (const auto& match : var_desc_matches) {
            if (match.size() > 2) {
                var_descriptions[match[1]] = match[2];
            }
        }
        
        for (const auto& var : metadata.variables) {
            std::string desc = var_descriptions.count(var) > 0 ? 
                              var_descriptions[var] : "No description available";
            doc << "| " << var << " | " << desc << " |\n";
        }
        doc << "\n";
    }
    
    // Example usage
    doc << "## Example\n\n";
    doc << "```sql\n" << example << "\n```\n\n";
    
    // Inheritance chain if applicable
    if (metadata.parent.has_value() && !metadata.parent.value().empty()) {
        try {
            auto chain = get_inheritance_chain(metadata.name);
            if (chain.size() > 1) {  // more than just the current template
                doc << "## Inheritance Chain\n\n";
                for (size_t i = 0; i < chain.size(); ++i) {
                    doc << (i + 1) << ". " << chain[i] << "\n";
                }
                doc << "\n";
            }
        } catch (const std::exception& e) {
            doc << "**Note:** Error retrieving inheritance chain: " << e.what() << "\n\n";
        }
    }
    
    // Location info
    doc << "## File Location\n\n";
    doc << "```\n" << get_template_path(metadata.name) << "\n```\n";
    
    return doc.str();
}

} // namespace cql