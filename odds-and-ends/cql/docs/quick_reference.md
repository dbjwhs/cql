# CQL Quick Reference Guide

## Core Directives

| Directive | Description | Example |
|-----------|-------------|---------|
| `@copyright` | Specifies license and copyright holder | `@copyright "MIT License" "2025 Author"` |
| `@language` | Target programming language | `@language "C++"` |
| `@description` | Description of the code to generate | `@description "implement a thread-safe queue"` |
| `@context` | Additional context for generation | `@context "Using C++20 features"` |
| `@dependency` | Required libraries or dependencies | `@dependency "std::mutex, std::condition_variable"` |
| `@test` | Test cases to be supported | `@test "Test concurrent push operations"` |
| `@variable` | Define template variables | `@variable "max_size" "1000"` |
| `@inherit` | Inherit from another template | `@inherit "base_template"` |
| `@architecture` | Specifies architectural pattern | `@architecture component "factory_method" "options"` |
| `@constraint` | Defines implementation constraints | `@constraint "Thread-safe for concurrent access"` |
| `@security` | Security requirements | `@security "Prevent data races and deadlocks"` |
| `@complexity` | Performance complexity goals | `@complexity "O(1) for push and pop operations"` |
| `@example` | Usage examples with optional title | `@example "Basic Usage" "..."` |

## Variable Syntax

- Define variables: `@variable "name" "value"`
- Use variables: `${name}`
- Variables can be used in any directive value

## Template Inheritance

- Base template: `@description "base template"`
- Child template: `@inherit "base_template"`
- Variables from parent templates are inherited
- Child variables override parent variables with the same name

## Command Line Interface

```bash
# Compile a CQL file
cql input.cql output.txt

# Run in interactive mode
cql --interactive

# Run tests
cql --test

# Run examples
cql --examples

# Show help
cql --help
```

## Template Management

```bash
# In interactive mode:
save template_name                    # Save current template
load template_name                    # Load a template
list                                  # List all templates
delete template_name                  # Delete a template
create category_name                  # Create a category
instantiate template_name var=value   # Instantiate with variables
```

## Common Patterns

### Basic Template
```
@copyright "MIT License" "2025 Author"
@language "C++"
@description "implement a data structure"
```

### Parameterized Template
```
@variable "container_type" "vector"
@variable "element_type" "int"
@description "implement a ${container_type}<${element_type}> class"
@language "C++"
```

### Inheritance Example
```
# Base (parent.cql):
@description "Base data structure"
@variable "type" "int"

# Child (child.cql):
@inherit "parent"
@description "Extended data structure"
@variable "container" "vector"
```

## Best Practices

1. Use descriptive template names
2. Include detailed descriptions
3. Provide specific test cases
4. Use variables for customizable elements
5. Structure complex templates using inheritance
6. Include examples of expected usage
7. Define clear constraints and requirements
8. Use appropriate architectural patterns

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Template not found | Check template name and directory structure |
| Circular inheritance | Remove circular references between templates |
| Undeclared variable | Define variables before using with `@variable` |
| Validation warnings | Address unused variables or incomplete directives |
| Compilation failures | Verify syntax and required directives |
