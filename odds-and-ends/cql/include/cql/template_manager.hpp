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
#include <set>

namespace cql {

// class for managing query templates
// provides functionality to save, load, and manage cql query templates
class TemplateManager {
public:
    // constructor sets default template directory
    TemplateManager();
    explicit TemplateManager(const std::string& template_dir);

    // save a template
    void save_template(const std::string& name, const std::string& content);
    
    // load a template by name
    std::string load_template(const std::string& name);
    
    // get template metadata (description, variables, etc.)
    struct TemplateMetadata {
        std::string name;
        std::string description;
        std::vector<std::string> variables;
        std::string last_modified;
        std::optional<std::string> parent; // parent template name if inheriting
    };
    
    TemplateMetadata get_template_metadata(const std::string& name);
    
    // generate documentation for a template
    std::string generate_template_documentation(const std::string& name);
    
    // generate documentation for all templates
    std::string generate_all_template_documentation();
    
    // export documentation to a file
    bool export_documentation(const std::string& output_path, const std::string& format = "markdown");
    
    // list all available templates
    std::vector<std::string> list_templates();
    
    // delete a template
    bool delete_template(const std::string& name);
    
    // create a template from an existing one with variable substitutions
    std::string instantiate_template(const std::string& name, 
                                   const std::map<std::string, std::string>& variables);
    
    // get/set the templates directory
    std::string get_templates_directory() const;
    void set_templates_directory(const std::string& dir);
    
    // create a new category for organizing templates
    bool create_category(const std::string& category);
    
    // list all categories
    std::vector<std::string> list_categories();
    
    // template inheritance methods
    
    // create a new template that inherits from another
    void create_inherited_template(const std::string& name, const std::string& parent_name, 
                                  const std::string& content);
    
    // load a template and merge in inherited content from parent templates
    std::string load_template_with_inheritance(const std::string& name);
    
    // get a list of parent templates (inheritance chain)
    std::vector<std::string> get_inheritance_chain(const std::string& name);

private:
    // directory where templates are stored
    std::string m_templates_dir;
    
    // get the full path for a template
    std::string get_template_path(const std::string& name) const;
    
    // create templates directory if it doesn't exist and validate structure
    void ensure_templates_directory();
    
    // validate the template directory structure and permissions
    bool validate_template_directory() const;
    
    // initialize standard directory structure
    void initialize_template_structure();
    
    // repair template directory if needed
    bool repair_template_directory();
    
    // create README file with standard content
    void create_readme_file();
    
    // ensure standard directory structure (common/ and user/ directories)
    void ensure_standard_directories();
    
    // format template metadata as markdown
    std::string format_template_markdown(const TemplateMetadata& metadata, const std::string& content);
    
    // extract variables from a template
    std::vector<std::string> extract_variables(const std::string& content);
    
    // extract description from a template
    std::string extract_description(const std::string& content);
    
    // replace variable references with their values
    std::string replace_variables(const std::string& content, 
                                  const std::map<std::string, std::string>& variables);
    
    // extract parent template name if this template inherits from another
    std::optional<std::string> extract_parent_template(const std::string& content);
    
    // merge parent template content with child template content
    std::string merge_template_content(const std::string& parent_content, 
                                      const std::string& child_content);
                                      
    // extract example usage from template content
    std::string extract_example(const std::string& content);

public: // make public to allow direct access from CLI
    // extract and combine all variables from template content and declared variables
    std::map<std::string, std::string> collect_variables(const std::string& content);
};

} // namespace cql

#endif // CQL_TEMPLATE_MANAGER_HPP