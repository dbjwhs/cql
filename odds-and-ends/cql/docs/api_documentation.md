# CQL API Documentation

This document provides a comprehensive reference for developers looking to integrate with or extend the Claude Query Language (CQL) compiler.

## Core Classes

### QueryProcessor

The main entry point for compiling CQL queries.

```cpp
class QueryProcessor {
public:
    // Compile a CQL query string into output
    static std::string compile(const std::string& query);
    
    // Compile a CQL query from a file
    static std::string compile_file(const std::string& input_file);
    
    // Write compiled output to a file
    static void compile_to_file(const std::string& input_file, const std::string& output_file);
};
```

**Example:**
```cpp
#include "cql/cql.hpp"

int main() {
    std::string query = "@language \"C++\"\n@description \"implement a stack\"";
    std::string result = cql::QueryProcessor::compile(query);
    // Process the result...
    return 0;
}
```

### TemplateManager

Manages storing, retrieving, and instantiating templates.

```cpp
class TemplateManager {
public:
    // Constructor
    explicit TemplateManager(const std::string& templates_dir);
    
    // Save a template
    void save_template(const std::string& name, const std::string& content);
    
    // Load a template
    std::string load_template(const std::string& name);
    
    // Load a template with its entire inheritance chain
    std::string load_template_with_inheritance(const std::string& name);
    
    // Delete a template
    bool delete_template(const std::string& name);
    
    // List all available templates
    std::vector<std::string> list_templates();
    
    // List categories
    std::vector<std::string> list_categories();
    
    // Create a new category
    bool create_category(const std::string& name);
    
    // Get template metadata
    TemplateMetadata get_template_metadata(const std::string& name);
    
    // Get inheritance chain
    std::vector<std::string> get_inheritance_chain(const std::string& name);
    
    // Instantiate a template with variables
    std::string instantiate_template(const std::string& name, 
                                    const std::map<std::string, std::string>& variables);
};
```

**Example:**
```cpp
#include "cql/template_manager.hpp"

int main() {
    // Initialize the template manager
    cql::TemplateManager manager("./templates");
    
    // List all templates
    auto templates = manager.list_templates();
    for (const auto& tmpl : templates) {
        std::cout << tmpl << std::endl;
    }
    
    // Load a template with inheritance
    std::string template_content = manager.load_template_with_inheritance("my_template");
    
    // Instantiate with variables
    std::map<std::string, std::string> vars = {
        {"container", "vector"},
        {"type", "int"}
    };
    std::string instantiated = manager.instantiate_template("my_template", vars);
    
    return 0;
}
```

### TemplateValidator

Validates template content for errors and warnings.

```cpp
class TemplateValidator {
public:
    // Constructor
    explicit TemplateValidator(TemplateManager& manager);
    
    // Add a validation rule
    void add_validation_rule(ValidationRule rule);
    
    // Validate a template
    TemplateValidationResult validate_template(const std::string& name);
    
    // Validate template content directly
    TemplateValidationResult validate_content(const std::string& content);
};
```

**Example:**
```cpp
#include "cql/template_validator.hpp"

int main() {
    cql::TemplateManager manager("./templates");
    cql::TemplateValidator validator(manager);
    
    // Add a custom validation rule
    validator.add_validation_rule([](const std::string& content) {
        std::vector<cql::TemplateValidationIssue> issues;
        // Custom validation logic...
        return issues;
    });
    
    // Validate a template
    auto result = validator.validate_template("my_template");
    
    // Check for issues
    if (result.has_issues(cql::TemplateValidationLevel::ERROR)) {
        std::cout << "Template has errors!" << std::endl;
        for (const auto& issue : result.get_issues()) {
            if (issue.level == cql::TemplateValidationLevel::ERROR) {
                std::cout << issue.message << std::endl;
            }
        }
    }
    
    return 0;
}
```

## Node Classes

CQL uses a visitor-based design pattern for node traversal.

### Base Node

```cpp
class Node {
public:
    virtual ~Node() = default;
    virtual void accept(Visitor& visitor) = 0;
};
```

### Directive Node

```cpp
class DirectiveNode : public Node {
public:
    DirectiveNode(const std::string& name, const std::string& value);
    void accept(Visitor& visitor) override;
    
    std::string name;
    std::string value;
};
```

### Variable Node

```cpp
class VariableNode : public Node {
public:
    VariableNode(const std::string& name, const std::string& value);
    void accept(Visitor& visitor) override;
    
    std::string name;
    std::string value;
};
```

### Visitor Pattern

```cpp
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void visit(DirectiveNode& node) = 0;
    virtual void visit(VariableNode& node) = 0;
    // Other visit methods for other node types...
};
```

## Extending CQL

### Creating Custom Validators

```cpp
#include "cql/template_validator.hpp"

// Custom validation function type
using ValidationRule = std::function<std::vector<TemplateValidationIssue>(const std::string&)>;

// Create a custom validator for specific directives
ValidationRule create_directive_validator(const std::string& directive_name) {
    return [directive_name](const std::string& content) {
        std::vector<TemplateValidationIssue> issues;
        std::regex directive_regex("@" + directive_name + "\\s+\"([^\"]+)\"");
        
        std::smatch m;
        std::string::const_iterator search_start(content.cbegin());
        while (std::regex_search(search_start, content.cend(), m, directive_regex)) {
            std::string value = m[1].str();
            // Custom validation logic for the directive value
            if (value.length() < 3) {
                issues.emplace_back(
                    TemplateValidationLevel::WARNING,
                    directive_name + " value is too short: " + value
                );
            }
            search_start = m.suffix().first;
        }
        
        return issues;
    };
}

int main() {
    cql::TemplateManager manager("./templates");
    cql::TemplateValidator validator(manager);
    
    // Add custom validators
    validator.add_validation_rule(create_directive_validator("description"));
    validator.add_validation_rule(create_directive_validator("language"));
    
    // Validate a template
    auto result = validator.validate_template("my_template");
    
    return 0;
}
```

### Creating Custom Visitors

```cpp
#include "cql/visitor.hpp"

// Custom visitor that counts directive types
class DirectiveCounter : public cql::Visitor {
public:
    void visit(cql::DirectiveNode& node) override {
        directive_counts[node.name]++;
    }
    
    void visit(cql::VariableNode& node) override {
        // Not counted
    }
    
    // Other visit methods...
    
    std::map<std::string, int> directive_counts;
};

int main() {
    // Parse template into nodes...
    DirectiveCounter counter;
    
    // Accept the visitor
    root_node->accept(counter);
    
    // Print the counts
    for (const auto& [directive, count] : counter.directive_counts) {
        std::cout << directive << ": " << count << std::endl;
    }
    
    return 0;
}
```

## Error Handling

```cpp
try {
    cql::TemplateManager manager("./templates");
    std::string content = manager.load_template("non_existent");
} catch (const cql::TemplateNotFoundException& e) {
    std::cerr << "Template not found: " << e.what() << std::endl;
} catch (const cql::CircularInheritanceException& e) {
    std::cerr << "Circular inheritance: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Other error: " << e.what() << std::endl;
}
```

## Advanced Template Processing

```cpp
#include "cql/template_manager.hpp"
#include "cql/template_validator.hpp"

int main() {
    // Initialize managers
    cql::TemplateManager manager("./templates");
    cql::TemplateValidator validator(manager);
    
    // Get metadata for a template
    auto metadata = manager.get_template_metadata("my_template");
    
    // Check for parent
    if (metadata.parent.has_value()) {
        std::cout << "Parent template: " << metadata.parent.value() << std::endl;
    }
    
    // Get the inheritance chain
    auto chain = manager.get_inheritance_chain("my_template");
    std::cout << "Inheritance chain: ";
    for (const auto& ancestor : chain) {
        std::cout << ancestor << " -> ";
    }
    std::cout << "my_template" << std::endl;
    
    // Validate the template
    auto result = validator.validate_template("my_template");
    
    // Process only if no errors
    if (!result.has_issues(cql::TemplateValidationLevel::ERROR)) {
        // Get variables from metadata
        for (const auto& var : metadata.variables) {
            std::cout << "Variable: " << var << std::endl;
        }
        
        // Create variable map
        std::map<std::string, std::string> vars;
        for (const auto& var : metadata.variables) {
            // Prompt user for value or use default
            vars[var] = "default_value"; // Replace with actual logic
        }
        
        // Instantiate the template with variables
        std::string instantiated = manager.instantiate_template("my_template", vars);
        
        // Process the instantiated template
        std::cout << "Instantiated template: " << instantiated << std::endl;
    } else {
        // Handle validation errors
        for (const auto& issue : result.get_issues()) {
            if (issue.level == cql::TemplateValidationLevel::ERROR) {
                std::cerr << "Error: " << issue.message << std::endl;
            }
        }
    }
    
    return 0;
}
```