// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/template_manager.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/project_utils.hpp"

#include <fstream>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <set>
#include <utility>

namespace fs = std::filesystem;

namespace cql {

TemplateManager::TemplateManager() {
    // the default templates directory is in the user's home directory
    if (const char* home_dir = getenv("HOME")) {
        m_templates_dir = std::string(home_dir) + "/.llm/templates";
    } else {
        m_templates_dir = "./llm_templates";
    }
    ensure_templates_directory();
}

TemplateManager::TemplateManager(std::string template_dir)
    : m_templates_dir(std::move(template_dir)) {
    ensure_templates_directory();
}

void TemplateManager::save_template(const std::string& name, const std::string& content) const {
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
        const std::string category = name.substr(0, name.find('/'));
        
        // ensure the category directory exists
        if (fs::path category_path = fs::path(m_templates_dir) / category; !fs::exists(category_path)) {
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
        if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".llm") {
            filename += ".llm";
        }
        
        template_path = m_templates_dir + "/user/" + filename;
    }
    
    try {
        // ensure the parent directory exists (for nested categories)
        if (const fs::path parent_dir = fs::path(template_path).parent_path(); !fs::exists(parent_dir)) {
            fs::create_directories(parent_dir);
        }
        
        // write the content to the file
        util::write_file(template_path, content);
        Logger::getInstance().log(LogLevel::INFO, "Template saved: ", name);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to save template: " + std::string(e.what()));
    }
}

std::string TemplateManager::load_template(const std::string& name) const {
    // get the full path
    const std::string template_path = get_template_path(name);
    
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

TemplateManager::TemplateMetadata TemplateManager::get_template_metadata(const std::string& name) const {
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
        
        // extract parent template if any
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

std::vector<std::string> TemplateManager::list_templates() const {
    std::vector<std::string> templates;
    
    // ensure the templates directory is valid
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
        
        // first, get all templates from the common directory
        if (fs::exists(fs::path(m_templates_dir) / "common")) {
            for (const auto& entry : fs::recursive_directory_iterator(fs::path(m_templates_dir) / "common")) {
                if (entry.is_regular_file() && entry.path().extension() == ".llm") {
                    // format a path as common/template
                    fs::path rel_path = entry.path().lexically_relative(m_templates_dir);
                    std::string template_name = rel_path.string();
                    templates.push_back(template_name);
                    added_templates.insert(template_name);
                }
            }
        }
        
        // next, get all templates from the user directory
        if (fs::exists(fs::path(m_templates_dir) / "user")) {
            for (const auto& entry : fs::recursive_directory_iterator(fs::path(m_templates_dir) / "user")) {
                if (entry.is_regular_file() && entry.path().extension() == ".llm") {
                    // format a path as user/template
                    fs::path rel_path = entry.path().lexically_relative(m_templates_dir);
                    std::string template_name = rel_path.string();
                    templates.push_back(template_name);
                    added_templates.insert(template_name);
                }
            }
        }
        
        // finally, get templates from any other directories (legacy support)
        for (const auto& entry : fs::directory_iterator(m_templates_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".llm") {
                // add only if not an internal file
                if (entry.path().filename().string() != "README.txt" && 
                    entry.path().filename().string()[0] != '.') {
                    // add only if we haven't encountered this template yet
                    if (std::string template_name = entry.path().filename().string(); !added_templates.contains(template_name)) {
                        templates.push_back(template_name);
                        added_templates.insert(template_name);
                    }
                }
            } else if (entry.is_directory() && 
                      entry.path().filename() != "common" && 
                      entry.path().filename() != "user") {
                // handle custom categories (not common or user)
                for (const auto& sub_entry : fs::recursive_directory_iterator(entry.path())) {
                    if (sub_entry.is_regular_file() && sub_entry.path().extension() == ".llm") {
                        // format a path as a category / template
                        fs::path rel_path = sub_entry.path().lexically_relative(m_templates_dir);
                        std::string template_name = rel_path.string();
                        
                        // add only if we haven't encountered this template yet
                        if (!added_templates.contains(template_name)) {
                            templates.push_back(template_name);
                            added_templates.insert(template_name);
                        }
                    }
                }
            }
        }
        
        // sort the templates alphabetically
        std::ranges::sort(templates, [](const std::string& a, const std::string& b) {
            // first sort by category, then by template name
            const std::string a_category = a.find('/') != std::string::npos ? a.substr(0, a.find('/')) : "";
            const std::string b_category = b.find('/') != std::string::npos ? b.substr(0, b.find('/')) : "";
            
            // put a common category first, then user, then others alphabetically
            if (a_category == "common" && b_category != "common") return true;
            if (a_category != "common" && b_category == "common") return false;
            if (a_category == "user" && b_category != "user" && b_category != "common") return true;
            if (a_category != "user" && a_category != "common" && b_category == "user") return false;
            
            // if categories are the same, sort by template name
            if (a_category == b_category) {
                const std::string a_name = a.find('/') != std::string::npos ? a.substr(a.find('/') + 1) : a;
                const std::string b_name = b.find('/') != std::string::npos ? b.substr(b.find('/') + 1) : b;
                return a_name < b_name;
            }
            
            // otherwise sort by category
            return a_category < b_category;
        });
        
        return templates;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to list templates: ", e.what());
        return templates; // return an empty list on error
    }
}

bool TemplateManager::delete_template(const std::string& name) const {
    // get the full path
    const std::string template_path = get_template_path(name);
    
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
) const {
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

bool TemplateManager::create_category(const std::string& category) const {
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

std::vector<std::string> TemplateManager::list_categories() const {
    std::vector<std::string> categories;
    
    try {
        // iterate through all directories in the templates directory
        for (const auto& entry : fs::directory_iterator(m_templates_dir)) {
            if (entry.is_directory()) {
                categories.push_back(entry.path().filename().string());
            }
        }
        
        // sort the categories alphabetically
        std::ranges::sort(categories);
        
        return categories;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to list categories: ", e.what());
        return categories; // return an empty list on error
    }
}

std::string TemplateManager::get_template_path(const std::string& name) const {
    // check if the name already has a .llm extension
    std::string filename = name;
    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".llm") {
        filename += ".llm";
    }
    
    // check if the name includes a category using category/template format
    if (filename.find('/') != std::string::npos) {
        // combine the templates directory with the category/template path
        return m_templates_dir + "/" + filename;
    }
    
    // first check in the user directory (the most common case)
    if (std::string user_path = m_templates_dir + "/user/" + filename; fs::exists(user_path)) {
        return user_path;
    }
    
    // then check in the common directory
    if (std::string common_path = m_templates_dir + "/common/" + filename; fs::exists(common_path)) {
        return common_path;
    }
    
    // check if it exists directly under the templates directory (legacy support)
    if (std::string root_path = m_templates_dir + "/" + filename; fs::exists(root_path)) {
        return root_path;
    }
    
    // if not found, default to user directory
    return m_templates_dir + "/user/" + filename;
}

void TemplateManager::ensure_templates_directory() const {
    try {
        // create the templates directory if it doesn't exist
        if (!fs::exists(m_templates_dir)) {
            fs::create_directories(m_templates_dir);
            Logger::getInstance().log(LogLevel::INFO, "Created templates directory: ", m_templates_dir);
            
            // initialize the directory with a standard structure
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
    try {
        bool valid = true;
        // check if the directory exists
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
            if (std::ofstream test(test_file); !test.is_open()) {
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

void TemplateManager::initialize_template_structure() const {
    try {
        // create standard directory structure
        ensure_standard_directories();
        
        // create a readme file
        create_readme_file();
        Logger::getInstance().log(LogLevel::INFO, "Created template directory structure");
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to initialize template structure: ", e.what());
    }
}

bool TemplateManager::repair_template_directory() const {
    try {
        // ensure main directory exists
        if (!fs::exists(m_templates_dir)) {
            fs::create_directories(m_templates_dir);
        }
        
        // ensure standard directory structure
        ensure_standard_directories();
        
        // recreate readme if missing
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
    // get all declared variables
    auto declared_vars = cql::util::extract_regex_group_values(
        content,
        "@variable\\s+\"([^\"]+)\"\\s+\"[^\"]*\"",
        1
    );
    
    // get all referenced variables
    const auto referenced_vars = cql::util::extract_regex_group_values(content, R"(\$\{([^}]+)\})", 1);
    
    // combine both sets into a vector
    std::vector<std::string> variables;
    variables.insert(variables.end(), declared_vars.begin(), declared_vars.end());
    
    // add referenced variables that aren't already in the list
    for (const auto& var : referenced_vars) {
        if (!declared_vars.contains(var)) {
            variables.push_back(var);
        }
    }
    
    return variables;
}

std::string TemplateManager::extract_description(const std::string& content) {
    // try to extract a description from @description directive

    if (auto descriptions = cql::util::extract_regex_group_values(content, "@description\\s+\"([^\"]*)\"", 1); !descriptions.empty()) {
        return *descriptions.begin();
    }
    
    // if no description directive found, return the first line as a fallback
    if (const size_t eol = content.find('\n'); eol != std::string::npos) {
        return content.substr(0, eol);
    }
    
    return "No description available";
}

std::string TemplateManager::replace_variables(
    const std::string& content,
    const std::map<std::string, std::string>& variables) {
    const std::string& result = content;
    const std::regex variable_ref_regex(R"(\$\{([^}]+)\})");
    
    // first, collect all variables from the content (declared with @variable)
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
        if (all_variables.contains(var_name)) {
            output.append(all_variables[var_name]);
            Logger::getInstance().log(LogLevel::INFO, "Replaced variable: ", var_name, " with: ", all_variables[var_name]);
        } else {
            // keep the original reference
            output.append(match[0].str());
            Logger::getInstance().log(LogLevel::ERROR, "Variable not found: ", var_name);
        }
        
        // update positions for the next iteration
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
    
    // extract all @variable declarations with their values
    const auto variable_matches = cql::util::extract_regex_matches(content
        , "@variable\\s+\"([^\"]*)\"\\s+\"([^\"]*)\"", 2);
    
    for (const auto& match : variable_matches) {
        if (match.size() > 2) {
            const std::string& name = match[1];
            const std::string& value = match[2];
            variables[name] = value;
            Logger::getInstance().log(LogLevel::INFO, "Found variable declaration: ", name, "=", value);
        }
    }
    
    return variables;
}

// extract a parent template name if this template inherits from another
std::optional<std::string> TemplateManager::extract_parent_template(const std::string& content) {
    if (const auto parents = cql::util::extract_regex_group_values(content, "@inherit\\s+\"([^\"]*)\"", 1); !parents.empty()) {
        std::string parent = *parents.begin();
        Logger::getInstance().log(LogLevel::INFO, "Found parent template: ", parent);
        return parent;
    }
    
    return std::nullopt;
}

// create a new template that inherits from another
void TemplateManager::create_inherited_template(const std::string& name, const std::string& parent_name, const std::string& content) const {
    // verify parent template exists
    if (const std::string parent_path = get_template_path(parent_name); !fs::exists(parent_path)) {
        throw std::runtime_error("Parent template not found: " + parent_name);
    }
    
    // add inheritance directive if not already present
    const std::regex inherit_regex("@inherit\\s+\"([^\"]*)\"");
    std::string modified_content = content;
    
    if (!std::regex_search(content, inherit_regex)) {
        // add @inherit directive at the beginning of the content
        modified_content = "@inherit \"" + parent_name + "\"\n" + content;
    }
    
    // save the template
    save_template(name, modified_content);
    Logger::getInstance().log(LogLevel::INFO, "Created template '", name, "' inheriting from '", parent_name, "'");
}

// get a list of parent templates (inheritance chain)
std::vector<std::string> TemplateManager::get_inheritance_chain(const std::string& name) const {
    std::vector<std::string> chain;
    std::set<std::string> visited; // to detect circular inheritance
    
    std::string current = name;
    while (!current.empty()) {
        // add to the chain
        chain.push_back(current);
        
        // mark as visited
        visited.insert(current);
        
        try {
            // load the template content
            std::string content = load_template(current);
            
            // extract parent template name
            auto parent = extract_parent_template(content);
            if (!parent.has_value() || parent.value().empty()) {
                break; // no parent, end of a chain
            }
            
            current = parent.value();
            
            // check for circular inheritance
            if (visited.contains(current)) {
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
            // rethrow to propagate the error
            throw;
        }
    }
    
    // reverse to get the base template first, followed by derived templates
    std::ranges::reverse(chain);
    return chain;
}

// load a template and merge in inherited content from parent templates
std::string TemplateManager::load_template_with_inheritance(const std::string& name) const {
    // get the inheritance chain
    const std::vector<std::string> chain = get_inheritance_chain(name);
    
    if (chain.empty()) {
        throw std::runtime_error("Failed to resolve inheritance chain for template: " + name);
    }
    
    // start with empty content
    std::string merged_content;
    
    // process each template in the chain, starting from the base class
    for (const auto& template_name : chain) {
        std::string template_content = load_template(template_name);
        
        if (merged_content.empty()) {
            // for the first template (base class), use its content directly
            merged_content = template_content;
        } else {
            // for derived templates, merge with existing content
            merged_content = merge_template_content(merged_content, template_content);
        }
    }
    
    return merged_content;
}

// merge parent template content with child template content
std::string TemplateManager::merge_template_content(const std::string& parent_content, const std::string& child_content) {
    // strip @inherit directive from child content
    std::regex inherit_regex("@inherit\\s+\"[^\"]*\"\\s*\n?");
    std::string stripped_child = std::regex_replace(child_content, inherit_regex, "");
    
    // collect variables from both templates
    auto parent_vars = collect_variables(parent_content);
    auto child_vars = collect_variables(child_content);
    
    // child variables override parent variables
    std::map<std::string, std::string> merged_vars = parent_vars;
    for (const auto& [name, value] : child_vars) {
        merged_vars[name] = value;
    }
    
    // create the variable declarations section
    std::string variables_section;
    for (const auto& [name, value] : merged_vars) {
        variables_section += "@variable \"" + name + "\" \"" + value + "\"\n";
    }
    
    // strip variable declarations from parent content
    std::regex var_regex("@variable\\s+\"[^\"]*\"\\s+\"[^\"]*\"\\s*\n?");
    std::string parent_without_vars = std::regex_replace(parent_content, var_regex, "");
    
    // strip variable declarations from child content
    std::string child_without_vars = std::regex_replace(stripped_child, var_regex, "");
    
    // combine the content
    return variables_section + "\n" + parent_without_vars + "\n" + child_without_vars;
}

// extract example usage from template content
std::string TemplateManager::extract_example(const std::string& content) {
    // look for @example directive in the content
    const std::regex example_regex("@example\\s+\"([^\"]*)\"");
    std::smatch match;
    
    if (std::regex_search(content, match, example_regex) && match.size() > 1) {
        return match[1].str();
    }
    
    // if no explicit example is found, try to extract the first query in the content
    std::regex query_regex("(SELECT|INSERT|UPDATE|DELETE|CREATE|ALTER|DROP|WITH)[^;]+;");
    if (std::regex_search(content, match, query_regex) && !match.empty()) {
        return match[0].str();
    }
    
    return "No example available";
}

// generate documentation for a template
std::string TemplateManager::generate_template_documentation(const std::string& name) const {
    try {
        // get the template metadata
        const TemplateMetadata metadata = get_template_metadata(name);
        
        // load the template content
        const std::string content = load_template(name);
        
        // format the template documentation using a common helper
        return format_template_markdown(metadata, content);
    } catch (const std::exception& e) {
        return "Error generating documentation: " + std::string(e.what());
    }
}

// generate documentation for all templates
std::string TemplateManager::generate_all_template_documentation() const {
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
            
            // check if a template has a category in its name
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
            std::ranges::replace(anchor, '/', '-');
            
            // if the template has a.llm extension, remove it for display
            std::string display_name = templ;
            if (display_name.length() > 4 && display_name.substr(display_name.length() - 4) == ".llm") {
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
bool TemplateManager::export_documentation(const std::string& output_path, const std::string& format) const {
    try {
        // generate the documentation
        std::string doc_content = generate_all_template_documentation();
        
        // convert to the requested format if not markdown
        std::string final_content;
        std::string extension;
        
        if (format == "markdown" || format == "md") {
            // no conversion needed
            final_content = doc_content;
            extension = ".md";
        } else if (format == "html") {
            // simple markdown to HTML conversion
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
            
            // simple markdown to HTML conversion for common elements
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
                    // escape HTML in code blocks
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
                else if (line.find('|') == 0) {
                    if (!in_table) {
                        html << "<table>\n";
                        in_table = true;
                    }
                    
                    // convert Markdown table row to HTML
                    html << "  <tr>\n";
                    
                    // split by pipe and process each cell
                    size_t pos = 0;
                    size_t start = 1; // skip first pipe
                    std::string cell;
                    
                    while ((pos = line.find('|', start)) != std::string::npos) {
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
                else if (line.find('[') != std::string::npos && line.find("](") != std::string::npos) {
                    std::string processed = line;
                    std::regex link_regex(R"(\[([^\]]*)\]\(([^\)]*)\))");
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
                        processed = std::regex_replace(processed, std::regex(R"(\*\*([^\*]*)\*\*)"), "<strong>$1</strong>");
                        processed = std::regex_replace(processed, std::regex(R"(\*([^\*]*)\*)"), "<em>$1</em>");
                        
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
            
            // HTML footer
            html << "</body>\n</html>";
            
            final_content = html.str();
            extension = ".html";
        } else if (format == "text" || format == "txt") {
            // simple markdown to plain text conversion
            std::string processed = doc_content;
            
            // remove Markdown formatting
            processed = std::regex_replace(processed, std::regex("#+\\s+"), ""); // headers
            processed = std::regex_replace(processed, std::regex(R"(\*\*([^\*]*)\*\*)"), "$1"); // bold
            processed = std::regex_replace(processed, std::regex(R"(\*([^\*]*)\*)"), "$1"); // italic
            processed = std::regex_replace(processed, std::regex("```[^`]*```"), ""); // code blocks
            processed = std::regex_replace(processed, std::regex(R"(\[([^\]]*)\]\([^\)]*\))"), "$1"); // links
            
            final_content = processed;
            extension = ".txt";
        } else {
            // unsupported format, default to Markdown
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
        if (fs::path output_dir = fs::path(final_path).parent_path(); !output_dir.empty() && !fs::exists(output_dir)) {
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

void TemplateManager::create_readme_file() const {
    const std::string readme_path = (fs::path(m_templates_dir) / "README.txt").string();
    if (std::ofstream readme(readme_path); readme.is_open()) {
        readme << "LLM Template Directory Structure\n";
        readme << "===============================\n\n";
        readme << "This directory contains LLM templates organized as follows:\n\n";
        readme << "- common/ : Standard templates that ship with LLM\n";
        readme << "- user/   : User-created templates\n\n";
        readme << "You can also create your own categories as subdirectories.\n";
        readme << "Each template should be a .llm file.\n";
        readme.close();
    }
}

void TemplateManager::ensure_standard_directories() const {
    // create a common subdirectory if it doesn't exist
    if (!fs::exists(fs::path(m_templates_dir) / "common") || 
        !fs::is_directory(fs::path(m_templates_dir) / "common")) {
        fs::create_directory(fs::path(m_templates_dir) / "common");
    }
    
    // create a user subdirectory if it doesn't exist
    if (!fs::exists(fs::path(m_templates_dir) / "user") || 
        !fs::is_directory(fs::path(m_templates_dir) / "user")) {
        fs::create_directory(fs::path(m_templates_dir) / "user");
    }
}

// format template documentation as Markdown
std::string TemplateManager::format_template_markdown(const TemplateMetadata& metadata, const std::string& content) const {
    // extract example
    std::string example = extract_example(content);
    
    // format the documentation
    std::stringstream doc;
    
    // template name as title
    doc << "# " << metadata.name << "\n\n";
    
    // description
    doc << "## Description\n\n" << metadata.description << "\n\n";
    
    // last modified date
    doc << "**Last Modified:** " << metadata.last_modified << "\n\n";
    
    // parent template if any
    if (metadata.parent.has_value() && !metadata.parent.value().empty()) {
        doc << "**Inherits From:** " << metadata.parent.value() << "\n\n";
    }
    
    // variables section
    doc << "## Variables\n\n";
    if (metadata.variables.empty()) {
        doc << "This template has no variables.\n\n";
    } else {
        doc << "| Name | Description |\n";
        doc << "|------|-------------|\n";
        
        // extract variable descriptions from content if available
        auto var_desc_matches = cql::util::extract_regex_matches(content
            , "@variable_description\\s+\"([^\"]*)\"\\s+\"([^\"]*)\"", 2);
        
        std::map<std::string, std::string> var_descriptions;
        for (const auto& match : var_desc_matches) {
            if (match.size() > 2) {
                var_descriptions[match[1]] = match[2];
            }
        }
        
        for (const auto& var : metadata.variables) {
            std::string desc = var_descriptions.contains(var)
                ?
                    var_descriptions[var] : "No description available";
                    doc << "| " << var << " | " << desc << " |\n";
        }
        doc << "\n";
    }
    
    // example usage
    doc << "## Example\n\n";
    doc << "```sql\n" << example << "\n```\n\n";
    
    // inheritance chain if applicable
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
    
    // location info
    doc << "## File Location\n\n";
    doc << "```\n" << get_template_path(metadata.name) << "\n```\n";
    
    return doc.str();
}

} // namespace cql
