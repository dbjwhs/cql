# Claude Query Language (CQL) - Phase 3 Features

This document details the features added in Phase 3 of the CQL project, focusing on the implementation of a comprehensive template system for query reuse and management.

## Template Management System

The template management system allows users to save, load, and customize CQL queries as reusable templates.

### Core Features

1. **Template Storage**
   - Templates are stored in `~/.cql/templates` by default
   - Each template is a regular `.cql` file
   - Templates can be organized into categories (subdirectories)

2. **Variable Interpolation**
   - Templates can include `${variable}` syntax for dynamic content
   - Variables are defined using the `@variable` directive
   - Default values can be provided in templates

3. **Template Metadata**
   - Each template stores metadata including description, last modified date, and variables
   - Metadata is extracted from the template content

4. **Template Categories**
   - Templates can be organized into categories
   - Each category is a subdirectory in the templates directory

### API

```cpp
// Class for managing query templates
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
};
```

### CLI Commands

The interactive CLI has been extended with the following template-related commands:

```
templates               - List all available templates
template save NAME      - Save current query as a template
template load NAME      - Load a template
template info NAME      - Show info about a template
template delete NAME    - Delete a template
template setvar NAME=VAL - Set a template variable
template use NAME       - Use a template with current variables
template dir [PATH]     - Show or set templates directory
categories              - List template categories
category create NAME    - Create a new template category
```

### Command-Line Options

New command-line options have been added:

```
--templates, -l         List all available templates
--template NAME, -T     Use a specific template
```

### Implementation Details

1. **Variable Extraction and Interpolation**
   - Regular expressions are used to extract variables from templates
   - Variables are replaced with their values during template instantiation
   - Both declared variables (`@variable`) and referenced variables (`${var}`) are tracked

2. **Template Path Resolution**
   - Templates can be referenced by name, with the `.cql` extension automatically added
   - Templates can also be organized in categories using subdirectories

3. **Template Metadata Extraction**
   - Description is extracted from the `@description` directive
   - Last modified date is taken from the file's modification time
   - Variables are extracted from both declarations and references

4. **Error Handling**
   - Robust error handling for file operations
   - Clear error messages for missing templates or variables

## Example Templates

### Basic Template (examples/template_example.cql)

```
@copyright "${license_type}" "${license_owner}"
@language "${language}"
@description "implement a ${collection_type} class with thread-safety"
@context "Using ${language_version} features and RAII principles"
@context "Must be exception-safe"
@dependency "std::mutex, std::condition_variable"
@performance "Support ${ops_per_second} operations per second"
@security "Prevent data races through proper synchronization"
@architecture "Thread-safe implementation with ${synchronization_method}"
@test "Test concurrent push operations"
@test "Test concurrent pop operations"
@test "Test boundary conditions (empty/full)"
@test "Test exception safety guarantees"
```

### Template with Default Values (examples/default_values.cql)

```
@variable "license_type" "MIT License"
@variable "license_owner" "2025 dbjwhs"
@variable "language" "C++"
@variable "collection_type" "queue"
@variable "language_version" "C++20"
@variable "ops_per_second" "100k"
@variable "synchronization_method" "mutex and condition variables"

@copyright "${license_type}" "${license_owner}"
@language "${language}"
@description "implement a ${collection_type} class with thread-safety"
@context "Using ${language_version} features and RAII principles"
@context "Must be exception-safe"
@dependency "std::mutex, std::condition_variable"
@performance "Support ${ops_per_second} operations per second"
@security "Prevent data races through proper synchronization"
@architecture "Thread-safe implementation with ${synchronization_method}"
@test "Test concurrent push operations"
@test "Test concurrent pop operations"
@test "Test boundary conditions (empty/full)"
@test "Test exception safety guarantees"
```

## Future Enhancements

1. **Template Versioning**
   - Support for versioning templates to track changes over time
   - Ability to revert to previous versions

2. **Template Inheritance**
   - Allow templates to extend or inherit from other templates
   - Support for template composition and reuse

3. **Template Sharing**
   - Support for importing and exporting templates
   - Integration with version control systems

4. **Template Validation**
   - Validate templates against schema
   - Check for missing or unused variables

5. **Template Documentation**
   - Generate documentation for templates
   - Include usage examples and best practices