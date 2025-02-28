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

/**
 * @class TemplateManager
 * @brief Class for managing CQL query templates
 *
 * The TemplateManager provides comprehensive functionality to save, load, and organize
 * CQL query templates. It supports template inheritance, variable substitution, category
 * organization, and documentation generation.
 *
 * Templates are stored in a standard directory structure:
 * - common/ - System templates that come with CQL
 * - user/ - User-created templates
 * - [categories] - Optional category directories
 *
 * Features include:
 * - Template inheritance (child templates can inherit from parents)
 * - Variable substitution (templates can include variables for customization)
 * - Metadata extraction (description, variables, inheritance relationships)
 * - Documentation generation (markdown format)
 * - Category organization
 */
class TemplateManager {
public:
    /**
     * @brief Default constructor 
     *
     * Initializes the template manager with the default template directory
     * (typically ~/.cql/templates or equivalent)
     */
    TemplateManager();
    
    /**
     * @brief Constructor with custom template directory
     *
     * @param template_dir The directory path where templates will be stored
     */
    explicit TemplateManager(const std::string& template_dir);

    /**
     * @brief Save a template to the templates directory
     *
     * @param name The template name (in format "category/template_name")
     * @param content The template content
     * @throws std::runtime_error If the template cannot be saved
     */
    void save_template(const std::string& name, const std::string& content);
    
    /**
     * @brief Load a template by name
     *
     * @param name The template name (in format "category/template_name")
     * @return The template content as a string
     * @throws std::runtime_error If the template cannot be found or loaded
     */
    std::string load_template(const std::string& name);
    
    /**
     * @struct TemplateMetadata
     * @brief Holds metadata information about a template
     */
    struct TemplateMetadata {
        std::string name;                      ///< Template name
        std::string description;               ///< Template description
        std::vector<std::string> variables;    ///< Variable names used in the template
        std::string last_modified;             ///< Last modification timestamp
        std::optional<std::string> parent;     ///< Parent template name (if inheriting)
    };
    
    /**
     * @brief Extract and return metadata about a template
     *
     * @param name The template name
     * @return TemplateMetadata struct with template information
     * @throws std::runtime_error If the template cannot be found or analyzed
     */
    TemplateMetadata get_template_metadata(const std::string& name);
    
    /**
     * @brief Generate documentation for a single template
     *
     * Creates markdown documentation including:
     * - Template name and description
     * - List of variables with descriptions
     * - Inheritance relationships
     * - Example usage
     *
     * @param name The template name
     * @return Markdown formatted documentation string
     * @throws std::runtime_error If the template cannot be found
     */
    std::string generate_template_documentation(const std::string& name);
    
    /**
     * @brief Generate documentation for all available templates
     *
     * @return Markdown formatted documentation covering all templates
     */
    std::string generate_all_template_documentation();
    
    /**
     * @brief Export documentation to a file
     *
     * @param output_path The file path where documentation will be saved
     * @param format The output format (currently supports "markdown" only)
     * @return true if documentation was successfully exported, false otherwise
     */
    bool export_documentation(const std::string& output_path, const std::string& format = "markdown");
    
    /**
     * @brief List all available templates
     *
     * @return Vector of template names (in format "category/template_name")
     */
    std::vector<std::string> list_templates();
    
    /**
     * @brief Delete a template
     *
     * @param name The template name to delete
     * @return true if template was successfully deleted, false otherwise
     */
    bool delete_template(const std::string& name);
    
    /**
     * @brief Create a template instance with variable substitutions
     *
     * @param name The template name
     * @param variables Map of variable names to their values for substitution
     * @return Instantiated template content with variables replaced
     * @throws std::runtime_error If the template cannot be found or variables are missing
     */
    std::string instantiate_template(const std::string& name, 
                                   const std::map<std::string, std::string>& variables);
    
    /**
     * @brief Get the current templates directory
     *
     * @return Path to the templates directory
     */
    std::string get_templates_directory() const;
    
    /**
     * @brief Set a new templates directory
     *
     * @param dir The new directory path
     * @throws std::runtime_error If the directory cannot be created or accessed
     */
    void set_templates_directory(const std::string& dir);
    
    /**
     * @brief Create a new category for organizing templates
     *
     * @param category The category name
     * @return true if category was successfully created, false otherwise
     */
    bool create_category(const std::string& category);
    
    /**
     * @brief List all template categories
     *
     * @return Vector of category names
     */
    std::vector<std::string> list_categories();
    
    //------------------------------------------------------------------------------
    // Template inheritance methods
    //------------------------------------------------------------------------------
    
    /**
     * @brief Create a new template that inherits from a parent template
     *
     * @param name The name for the new template
     * @param parent_name The parent template name to inherit from
     * @param content The content for the new template (will be merged with parent)
     * @throws std::runtime_error If the parent template cannot be found
     */
    void create_inherited_template(const std::string& name, const std::string& parent_name, 
                                  const std::string& content);
    
    /**
     * @brief Load a template with all inherited content merged in
     *
     * Loads the template and all parent templates, then merges them according
     * to the inheritance rules.
     *
     * @param name The template name
     * @return The fully merged template content
     * @throws std::runtime_error If any template in the inheritance chain cannot be found
     */
    std::string load_template_with_inheritance(const std::string& name);
    
    /**
     * @brief Get the full inheritance chain for a template
     *
     * @param name The template name
     * @return Vector of template names in inheritance order (base to derived)
     * @throws std::runtime_error If the template cannot be found
     */
    std::vector<std::string> get_inheritance_chain(const std::string& name);

private:
    std::string m_templates_dir;  ///< Directory where templates are stored
    
    /**
     * @brief Get the full file path for a template
     *
     * @param name The template name
     * @return Full file path
     */
    std::string get_template_path(const std::string& name) const;
    
    /**
     * @brief Create templates directory if it doesn't exist and validate structure
     */
    void ensure_templates_directory();
    
    /**
     * @brief Validate the template directory structure and permissions
     *
     * @return true if directory structure is valid, false otherwise
     */
    bool validate_template_directory() const;
    
    /**
     * @brief Initialize standard directory structure for templates
     */
    void initialize_template_structure();
    
    /**
     * @brief Repair template directory structure if it's damaged
     *
     * @return true if repair was successful, false otherwise
     */
    bool repair_template_directory();
    
    /**
     * @brief Create README file with usage instructions
     */
    void create_readme_file();
    
    /**
     * @brief Ensure standard directories (common/ and user/) exist
     */
    void ensure_standard_directories();
    
    /**
     * @brief Format template metadata as markdown documentation
     *
     * @param metadata The template metadata
     * @param content The template content
     * @return Formatted markdown string
     */
    std::string format_template_markdown(const TemplateMetadata& metadata, const std::string& content);
    
    /**
     * @brief Extract variable names from template content
     *
     * @param content The template content
     * @return Vector of variable names
     */
    std::vector<std::string> extract_variables(const std::string& content);
    
    /**
     * @brief Extract description from template content
     *
     * Looks for @description directive in the template.
     *
     * @param content The template content
     * @return Description string
     */
    std::string extract_description(const std::string& content);
    
    /**
     * @brief Replace variable references with their values
     *
     * @param content The template content with variable references
     * @param variables Map of variable names to values
     * @return Content with variables replaced
     */
    std::string replace_variables(const std::string& content, 
                                  const std::map<std::string, std::string>& variables);
    
    /**
     * @brief Extract parent template name if this template inherits from another
     *
     * Looks for @inherits directive in the template.
     *
     * @param content The template content
     * @return Optional containing parent name if found, empty otherwise
     */
    std::optional<std::string> extract_parent_template(const std::string& content);
    
    /**
     * @brief Merge parent template content with child template content
     *
     * Implements the template inheritance algorithm:
     * - Child directives override parent directives
     * - Parent directives not in child are preserved
     * - Special directives like @inherits are handled specially
     *
     * @param parent_content The parent template content
     * @param child_content The child template content
     * @return Merged content
     */
    std::string merge_template_content(const std::string& parent_content, 
                                      const std::string& child_content);
                                      
    /**
     * @brief Extract example usage from template content
     *
     * @param content The template content
     * @return Example usage string
     */
    std::string extract_example(const std::string& content);

public: // Exposed for direct access from CLI
    /**
     * @brief Extract and combine all variables from template
     *
     * Collects all variables from the template content and declared variables.
     *
     * @param content The template content
     * @return Map of variable names to default values
     */
    std::map<std::string, std::string> collect_variables(const std::string& content);
};

} // namespace cql

#endif // cql_template_manager_hpp
