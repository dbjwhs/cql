// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_TEMPLATE_MANAGER_HPP
#define CQL_TEMPLATE_MANAGER_HPP

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <filesystem>

namespace cql {

/**
 * Class for managing query templates
 * Provides functionality to save, load, and manage CQL query templates
 */
class TemplateManager {
public:
    // Constructor sets default template directory
    TemplateManager();
    explicit TemplateManager(const std::string& template_dir);

    // Save a template
    void save_template(const std::string& name, const std::string& content);
    
    // Load a template by name
    std::string load_template(const std::string& name);
    
    // Get template metadata (description, variables, etc.)
    struct TemplateMetadata {
        std::string name;
        std::string description;
        std::vector<std::string> variables;
        std::string last_modified;
    };
    
    TemplateMetadata get_template_metadata(const std::string& name);
    
    // List all available templates
    std::vector<std::string> list_templates();
    
    // Delete a template
    bool delete_template(const std::string& name);
    
    // Create a template from an existing one with variable substitutions
    std::string instantiate_template(const std::string& name, 
                                   const std::map<std::string, std::string>& variables);
    
    // Get/set the templates directory
    std::string get_templates_directory() const;
    void set_templates_directory(const std::string& dir);
    
    // Create a new category for organizing templates
    bool create_category(const std::string& category);
    
    // List all categories
    std::vector<std::string> list_categories();

private:
    // Directory where templates are stored
    std::string m_templates_dir;
    
    // Get the full path for a template
    std::string get_template_path(const std::string& name) const;
    
    // Create templates directory if it doesn't exist
    void ensure_templates_directory();
    
    // Extract variables from a template
    std::vector<std::string> extract_variables(const std::string& content);
    
    // Extract description from a template
    std::string extract_description(const std::string& content);
};

} // namespace cql

#endif // CQL_TEMPLATE_MANAGER_HPP