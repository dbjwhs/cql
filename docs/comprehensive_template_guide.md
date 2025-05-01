# CQL Comprehensive Template Guide

This guide provides a detailed overview of the template system in CQL (Claude Query Language), covering all aspects of creating, managing, and using templates for more powerful and customized interactions with Large Language Models.

## Table of Contents

1. [Introduction to Templates](#1-introduction-to-templates)
2. [Template Directory Structure](#2-template-directory-structure)
3. [Template Format and Syntax](#3-template-format-and-syntax)
   - [Core Directives](#31-core-directives)
   - [Advanced Directives](#32-advanced-directives)
   - [API-Specific Directives](#33-api-specific-directives)
4. [Template Variables](#4-template-variables)
   - [Declaring Variables](#41-declaring-variables)
   - [Using Variables](#42-using-variables)
   - [Variable Descriptions](#43-variable-descriptions)
5. [Template Inheritance](#5-template-inheritance)
   - [Inheriting from Parent Templates](#51-inheriting-from-parent-templates)
   - [Inheritance Chain](#52-inheritance-chain)
   - [Overriding Directives and Variables](#53-overriding-directives-and-variables)
6. [Template Management](#6-template-management)
   - [Command Line Interface](#61-command-line-interface)
   - [Interactive Mode Commands](#62-interactive-mode-commands)
   - [Saving and Loading Templates](#63-saving-and-loading-templates)
7. [Template Validation](#7-template-validation)
   - [Required Directives](#71-required-directives)
   - [Common Validation Errors](#72-common-validation-errors)
   - [Validation Commands](#73-validation-commands)
8. [Template Documentation](#8-template-documentation)
   - [Generating Documentation](#81-generating-documentation)
   - [Export Formats](#82-export-formats)
9. [Advanced Usage](#9-advanced-usage)
   - [Custom Categories](#91-custom-categories)
   - [Template Examples](#92-template-examples)
   - [Best Practices](#93-best-practices)
10. [Troubleshooting](#10-troubleshooting)

## 1. Introduction to Templates

Templates in CQL provide a way to create reusable query patterns for language models. They allow you to:

- Standardize query formats across your team or organization
- Create customizable patterns with variable substitution
- Build hierarchical query structures through inheritance
- Maintain a library of effective prompt strategies

Templates are stored as `.llm` files and use a directive-based syntax that specifies different aspects of your query, from basic descriptions to complex test requirements.

## 2. Template Directory Structure

Templates are organized in a hierarchical directory structure:

```
~/.llm/templates/
│
├── common/       # System templates that ship with CQL
│   ├── base.llm
│   └── ...
│
├── user/         # User-created templates
│   ├── my_template.llm
│   └── ...
│
└── my_category/  # Custom categories
    ├── example1.llm
    └── ...
```

The default template location is:
- `$HOME/.llm/templates` (Linux/macOS)
- `./llm_templates` (fallback if HOME is not set)

You can change the template directory using:
```bash
# In interactive mode
template dir /path/to/templates
```

## 3. Template Format and Syntax

Templates use a directive-based syntax, where each directive starts with `@` followed by the directive name and quoted parameters:

```
@directive_name "parameter1" "parameter2"
```

### 3.1 Core Directives

| Directive | Description | Format | Example |
|-----------|-------------|--------|---------|
| `@description` | Describes the query purpose (required) | `@description "text"` | `@description "implement a thread-safe queue"` |
| `@copyright` | License and copyright information | `@copyright "license" "owner"` | `@copyright "MIT License" "2025 dbjwhs"` |
| `@language` | Target programming language | `@language "lang"` | `@language "C++"` |
| `@context` | Additional implementation context | `@context "text"` | `@context "Using C++20 features"` |
| `@dependency` | Required libraries or dependencies | `@dependency "deps"` | `@dependency "std::mutex, std::condition_variable"` |
| `@test` | Test case specification | `@test "case"` | `@test "Test empty queue behavior"` |
| `@variable` | Define template variable | `@variable "name" "value"` | `@variable "max_size" "1000"` |
| `@inherit` | Inherit from another template | `@inherit "template"` | `@inherit "base_template"` |

### 3.2 Advanced Directives

| Directive | Description | Format | Example |
|-----------|-------------|--------|---------|
| `@architecture` | Architectural pattern to use | `@architecture "pattern"` | `@architecture "Observer pattern for notifications"` |
| `@constraint` | Implementation constraints | `@constraint "text"` | `@constraint "Must be thread-safe"` |
| `@security` | Security requirements | `@security "text"` | `@security "Prevent data races"` |
| `@complexity` | Performance complexity goals | `@complexity "text"` | `@complexity "O(1) for push/pop operations"` |
| `@example` | Example usage code | `@example "name" "code"` | `@example "Basic Usage" "queue.push(42);"` |
| `@performance` | Performance expectations | `@performance "text"` | `@performance "Support 100k ops/second"` |

### 3.3 API-Specific Directives

| Directive | Description | Format | Example |
|-----------|-------------|--------|---------|
| `@model` | Claude model to use | `@model "model_name"` | `@model "claude-3-opus"` |
| `@max_tokens` | Maximum response length | `@max_tokens value` | `@max_tokens 100000` |
| `@temperature` | Response randomness (0.0-1.0) | `@temperature value` | `@temperature 0.7` |
| `@output_format` | Output format specification | `@output_format "format"` | `@output_format "multiple_files"` |
| `@format` | Format type (markdown/json) | `@format "type"` | `@format "json"` |
| `@pattern` | Design pattern specification | `@pattern "text"` | `@pattern "Factory Method pattern"` |
| `@structure` | File structure definition | `@structure "text"` | `@structure "include/core.hpp: Core interface"` |

## 4. Template Variables

Variables make templates customizable and reusable by allowing different values to be substituted.

### 4.1 Declaring Variables

Variables are declared using the `@variable` directive:

```
@variable "variable_name" "default_value"
```

Example:
```
@variable "class_name" "ThreadSafeQueue"
@variable "element_type" "int"
@variable "max_size" "1000"
```

### 4.2 Using Variables

Variables are referenced in templates using `${variable_name}` syntax:

```
@description "Implement a ${class_name} with maximum size of ${max_size}"
@language "C++"
```

Variables can be used in any directive or in the template body.

### 4.3 Variable Descriptions

You can document variables using the `@variable_description` directive:

```
@variable "max_size" "1000"
@variable_description "max_size" "Maximum number of elements in the collection"
```

This information will be included in the generated documentation.

## 5. Template Inheritance

Template inheritance allows you to build complex templates based on simpler ones.

### 5.1 Inheriting from Parent Templates

To inherit from another template, use the `@inherit` directive:

```
@inherit "parent_template"
```

The child template will receive all directives and content from the parent template.

### 5.2 Inheritance Chain

Templates can form an inheritance chain:

```
base_template.llm <- derived_template.llm <- specialized_template.llm
```

The system will load and merge all templates in the chain.

### 5.3 Overriding Directives and Variables

Child templates can override directives and variables from parent templates:

```
# parent.llm
@description "Base data structure"
@variable "type" "int"

# child.llm
@inherit "parent"
@description "Specialized data structure" # Overrides parent description
@variable "type" "double" # Overrides parent variable
```

## 6. Template Management

### 6.1 Command Line Interface

```bash
# List all templates
./cql --templates

# Use a specific template
./cql --template template_name

# Use a template with variables
./cql --template template_name var1=value1 var2=value2

# Validate a template
./cql --validate template_name

# Validate all templates
./cql --validate-all [PATH]

# Generate documentation
./cql --docs template_name
./cql --docs-all
./cql --export docs/templates.md [format]
```

### 6.2 Interactive Mode Commands

```bash
# Enter interactive mode
./cql --interactive

# Template commands
list                           # List all templates
template save [name]           # Save current template
template load [name]           # Load a template
template delete [name]         # Delete a template
template create [category]     # Create a template category
template instantiate [name]    # Instantiate a template
template validate [name]       # Validate a template
template validateall           # Validate all templates
template dir [path]            # Get/set templates directory
template docs [name]           # Generate template documentation
template docsall               # Generate all documentation
template export [path] [format] # Export documentation
```

### 6.3 Saving and Loading Templates

```bash
# In interactive mode
template save my_template      # Save current content as a template
template load my_template      # Load a template into current buffer
```

Templates are saved to the templates directory with a `.llm` extension.

## 7. Template Validation

Templates are validated to ensure they follow the correct format and contain all required elements.

### 7.1 Required Directives

At minimum, every template must include:

- `@description` - Describes the purpose of the template

### 7.2 Common Validation Errors

| Error | Description | Solution |
|-------|-------------|----------|
| Missing description | No `@description` directive | Add a description directive |
| Undeclared variable | Variable used but not declared | Declare with `@variable` directive |
| Circular inheritance | Templates forming an inheritance loop | Remove circular references |
| Invalid directive format | Directive with incorrect syntax | Check directive format |
| Unknown directive | Unrecognized directive name | Check spelling or documentation |

### 7.3 Validation Commands

```bash
# Validate a specific template
./cql --validate template_name

# Validate all templates
./cql --validate-all [PATH]

# In interactive mode
template validate template_name
template validateall
```

## 8. Template Documentation

Templates can be automatically documented based on their directives and content.

### 8.1 Generating Documentation

```bash
# Generate documentation for a specific template
./cql --docs template_name

# Generate documentation for all templates
./cql --docs-all

# Export documentation to a file
./cql --export docs/templates.md [format]
```

### 8.2 Export Formats

Documentation can be exported in three formats:

1. **Markdown** (default) - For rendering in GitHub or other Markdown viewers
2. **HTML** - For viewing in a web browser with formatting
3. **Text** - Plain text format for terminal viewing

Example:
```bash
# Export as Markdown (default)
./cql --export docs/templates.md

# Export as HTML
./cql --export docs/templates.html html

# Export as plain text
./cql --export docs/templates.txt text
```

The generated documentation includes:
- Template name and description
- Last modified date
- Parent template (if using inheritance)
- Variables and their descriptions
- Example usage
- Inheritance chain
- File location

## 9. Advanced Usage

### 9.1 Custom Categories

You can organize templates into categories by:

1. Creating a category directory:
   ```bash
   # In interactive mode
   template create my_category
   ```

2. Using category/template syntax:
   ```bash
   template save my_category/my_template
   template load my_category/my_template
   ```

### 9.2 Template Examples

#### 9.2.1 Basic Template

```
@copyright "MIT License" "2025 Author"
@language "C++"
@description "implement a data structure"
```

#### 9.2.2 Parameterized Template

```
@variable "container_type" "vector"
@variable "element_type" "int"
@description "implement a ${container_type}<${element_type}> class"
@language "C++"
```

#### 9.2.3 Template with Inheritance

```
# Base template (parent.llm)
@description "Base data structure"
@variable "type" "int"

# Child template (child.llm)
@inherit "parent"
@description "Extended data structure"
@variable "container" "vector"
```

#### 9.2.4 Comprehensive Template

```
@copyright "MIT License" "2025 Author"
@language "C++"
@description "implement a thread-safe ${collection_type} with configurable capacity"
@context "Using C++20 features and RAII principles"
@dependency "std::mutex, std::condition_variable"

@variable "collection_type" "queue"
@variable "element_type" "int"
@variable "max_size" "1000"
@variable_description "max_size" "Maximum number of elements in the collection"

@test "Test concurrent push operations"
@test "Test concurrent pop operations"
@test "Test boundary conditions (empty/full)"

@security "Prevent data races through proper synchronization"
@complexity "O(1) for all operations"

@example "Basic Usage" "
  // Create a thread-safe ${collection_type}
  ThreadSafe${collection_type}<${element_type}> collection(${max_size});
  
  // Push an element
  collection.push(42);
  
  // Pop an element
  auto value = collection.try_pop();
"
```

### 9.3 Best Practices

1. **Naming**
   - Use descriptive template names
   - Consider using category prefixes for related templates

2. **Variables**
   - Provide meaningful default values
   - Document variables with `@variable_description`
   - Use consistent naming conventions

3. **Inheritance**
   - Create base templates for common patterns
   - Extend with specialized templates
   - Avoid deep inheritance chains (prefer shallow hierarchies)

4. **Documentation**
   - Include detailed descriptions
   - Provide usage examples
   - Document template dependencies

5. **Organization**
   - Use categories to group related templates
   - Maintain a consistent directory structure
   - Delete outdated templates

## 10. Troubleshooting

| Issue | Solution |
|-------|----------|
| Template not found | Check template name and directory structure |
| Template validation fails | Address the specific validation errors |
| Variable not substituted | Ensure the variable is properly referenced with `${var_name}` |
| Inheritance issues | Check for circular references or missing parent templates |
| Documentation not generated | Ensure required directives are present |

If you encounter issues with the template directory structure, you can repair it with:

```bash
# In interactive mode
template repair
```

This will recreate the standard directory structure and ensure all necessary directories exist.

---

## 11. Conclusion

Templates are a powerful way to standardize and reuse query patterns in CQL. By mastering the template system, you can create a library of effective patterns, share them with your team, and consistently generate high-quality results from Large Language Models.

For more information, see:
- [Template Documentation](docs/template_documentation.md) - Basic guide to templates
- [Quick Reference](docs/quick_reference.md) - Concise syntax reference
- [Tutorial](docs/tutorial.md) - Step-by-step introduction to CQL