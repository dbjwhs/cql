# CQL: Claude Query Language - Tutorial

## 1. Introduction

### What is CQL?

Claude Query Language (CQL) is a specialized domain-specific language designed to streamline and standardize the query creation process. CQL allows users to define structured queries with metadata, variables, and templates that can be compiled into target languages. It's particularly useful for teams that need to maintain consistent query structures across projects.

CQL provides a layer of abstraction over raw queries, enabling better documentation, validation, and reuse. By separating the logical query structure from implementation details, CQL makes queries more maintainable and easier to understand.

### Key Features and Benefits

- **Structured Query Format**: Define queries with clear metadata and directives
- **Template System**: Create reusable query templates with variable substitution
- **Inheritance**: Build hierarchical template relationships
- **Validation**: Automatically validate templates for correctness
- **Documentation Generation**: Auto-generate documentation from templates
- **Multiple Output Formats**: Export to various formats including markdown and HTML
- **Interactive CLI**: Powerful command-line interface for working with templates

Key benefits include:
- Reduced redundancy in query development
- Improved query maintainability
- Better documentation of complex queries
- Standardized approach to query development
- Simplified onboarding for new team members

### Installation Requirements

To install and run CQL, you'll need:

- C++20 compatible compiler (GCC 10+, Clang 11+, or MSVC 19.28+)
- CMake 3.14 or higher
- Git (for version control and installation)

**Installation Steps:**

```bash
# clone the repository
git clone https://github.com/example/cql.git

# navigate to the project directory
cd cql

# create build directory
mkdir -p build && cd build

# configure and build
cmake ..
make

# verify installation
./cql --help
```

### Quick Start Guide

Let's create a simple query using CQL:

1. **Create a CQL file** (example.cql):

```
@copyright "MIT License" "2025 ExampleCorp"
@language "SQL"
@description "retrieve active users who recently logged in"

@variable "days_ago" "7"
@variable "status" "active"

SELECT 
  user_id, 
  username, 
  last_login
FROM users
WHERE status = '${status}'
AND last_login > DATE_SUB(NOW(), INTERVAL ${days_ago} DAY)
ORDER BY last_login DESC;
```

2. **Compile the query**:

```bash
./cql example.cql output.sql
```

3. **Use the interactive mode**:

```bash
./cql --interactive
```

Then at the prompt:
```
> load example.cql
> compile
> save output.sql
```

4. **Create a template**:

```
> template save user_query
> templates
```

You've now created and compiled your first CQL query and saved it as a template for future use!

## 2. Basic Concepts

### Query Structure

A CQL query consists of two main parts:

1. **Directive Section**: Contains metadata and directives that provide context and instructions for the compiler. Directives start with `@` and are typically at the beginning of the file.

2. **Query Body**: The actual query content that will be compiled according to the directives.

Basic structure:
```
@directive1 "parameter1" "parameter2"
@directive2 "parameter"

query content goes here...
```

### Directives Overview

Directives are special instructions that control how the query is processed, documented, and compiled. Each directive starts with `@` followed by its name and parameters in quotes.

Common directives include:
- `@copyright`: specifies license information
- `@language`: defines the target language
- `@description`: provides a description of the query
- `@variable`: defines a variable for substitution

Example:
```
@copyright "MIT License" "2025 ExampleCorp"
@language "SQL"
@description "retrieve active users"
@variable "status" "active"
```

### Comments and Formatting

CQL supports both directive-based metadata and regular comments:

- **CQL Directives**: start with `@` and are processed by the compiler
- **SQL Comments**: standard SQL comments (-- for single line, /* */ for multi-line)
- **Whitespace**: whitespace is preserved in the query body but normalized in directives

Example:
```
@description "User retrieval query"  -- this is a directive

-- this is a SQL comment
SELECT * FROM users
WHERE status = 'active'; /* this is also a comment */
```

### Query Compilation Process

The CQL compilation process follows these steps:

1. **Lexical Analysis**: the input is tokenized into directives, variables, and query content
2. **Parsing**: tokens are parsed into an Abstract Syntax Tree (AST)
3. **Variable Resolution**: variables are identified and prepared for substitution
4. **Directive Processing**: directives are processed to control the compilation
5. **Output Generation**: the final query is generated with variables substituted
6. **Validation**: the query is validated for correctness (if validation is enabled)

The result is a compiled query that can be executed in the target system.

## 3. Core Directives

### @copyright - Adding License Information

The `@copyright` directive specifies the license under which the query is distributed. It takes two parameters: the license type and the copyright holder.

Syntax:
```
@copyright "License Type" "Copyright Holder"
```

Example:
```
@copyright "MIT License" "2025 ExampleCorp"
```

This directive helps ensure proper attribution and legal compliance when sharing queries.

### @language - Specifying Target Language

The `@language` directive specifies the target language for the query. This helps CQL understand how to properly format and validate the query.

Syntax:
```
@language "Language Name"
```

Example:
```
@language "SQL"
@language "Python"
@language "C++"
```

The language directive affects how the query is processed and can influence syntax validation.

### @description - Describing Your Query

The `@description` directive provides a concise description of what the query does. This is essential for documentation and helps other users understand the query's purpose.

Syntax:
```
@description "Query description"
```

Example:
```
@description "retrieve active users who have completed their profile"
```

Good descriptions are specific, concise, and focused on the query's purpose.

### @context - Providing Context Information

The `@context` directive provides additional background information about when and how the query should be used.

Syntax:
```
@context "Context information"
```

Example:
```
@context "this query is designed for the reporting dashboard and should be run daily"
```

Context information helps users understand the broader application of the query.

### @dependency - Specifying Dependencies

The `@dependency` directive specifies other queries, functions, or resources that this query depends on.

Syntax:
```
@dependency "Dependency name or description"
```

Example:
```
@dependency "user_activity_log table must exist"
@dependency "get_active_users() function"
```

Listing dependencies helps ensure that all prerequisites are met before running the query.

### @test - Defining Test Cases

The `@test` directive defines test cases or testing scenarios for the query. This helps with validation and quality assurance.

Syntax:
```
@test "Test case description"
```

Example:
```
@test "should return empty result when no active users exist"
@test "should handle NULL values in last_login field"
```

Test directives document how the query should be tested and what scenarios to consider.

## 4. Advanced Directives (Phase 2)

### @architecture - Specifying Architecture Patterns

The `@architecture` directive specifies the architectural approach or pattern used in the query. This is particularly useful for complex queries that implement specific design patterns.

Syntax:
```
@architecture "Architecture pattern description"
```

Example:
```
@architecture "data pipeline with staging tables"
@architecture "materialized view refresh strategy"
```

This directive helps document the high-level design decisions in the query implementation.

### @constraint - Adding Constraints to Solutions

The `@constraint` directive defines limitations or constraints that the query must adhere to. These can be performance, resource, or business constraints.

Syntax:
```
@constraint "Constraint description"
```

Example:
```
@constraint "must execute in under 500ms"
@constraint "must not lock user table"
```

Documenting constraints helps ensure that the query meets operational requirements.

### @security - Defining Security Requirements

The `@security` directive specifies security considerations or requirements for the query.

Syntax:
```
@security "Security requirement"
```

Example:
```
@security "requires admin privileges"
@security "filters results based on user permissions"
```

Security directives help ensure that the query is used in accordance with security policies.

### @complexity - Specifying Performance Requirements

The `@complexity` directive documents the expected computational complexity or performance characteristics of the query.

Syntax:
```
@complexity "Complexity description"
```

Example:
```
@complexity "O(n) where n is the number of users"
@complexity "scales linearly with table size"
```

This helps users understand the performance implications of using the query.

### @example - Adding Usage Examples

The `@example` directive provides usage examples for the query. This is extremely helpful for documentation and onboarding.

Syntax:
```
@example "Example name" "Example content"
```

Example:
```
@example "Basic Usage" "
SELECT * FROM users
WHERE status = 'active';
"
```

Examples help users understand how to use and adapt the query for their needs.

## 5. Variables and Templates

### Variable Declaration with @variable

Variables allow you to create parameterized queries. The `@variable` directive defines a variable with a name and an optional default value.

Syntax:
```
@variable "variable_name" "default_value"
```

Example:
```
@variable "status" "active"
@variable "limit" "100"
```

Variables make your queries more flexible and reusable.

### Variable Reference Syntax ${var}

Once declared, variables can be referenced in the query using the `${variable_name}` syntax.

Example:
```
SELECT * 
FROM users
WHERE status = '${status}'
LIMIT ${limit};
```

When the query is compiled, these variable references are replaced with their values.

### Variable Description

You can provide descriptions for variables using the `@variable_description` directive:

Syntax:
```
@variable_description "variable_name" "description"
```

Example:
```
@variable "status" "active"
@variable_description "status" "filter users by their account status"
```

Variable descriptions are included in the generated documentation, making your templates more user-friendly.

### Default Values

The second parameter of the `@variable` directive sets the default value:

```
@variable "status" "active"  -- "active" is the default value
```

If a user doesn't provide a specific value, the default value is used.

You can override default values when using a template:

```
# command line
./cql --template user_query status=inactive limit=50

# interactive mode
> template use user_query
> template setvar status=inactive
```

### Variable Substitution Examples

Here's a complete example of variable usage:

```
@variable "table" "users"
@variable "status" "active"
@variable "limit" "100"

SELECT * 
FROM ${table}
WHERE status = '${status}'
LIMIT ${limit};
```

This can be compiled with different values:

```
# default values:
# table = "users", status = "active", limit = "100"
SELECT * 
FROM users
WHERE status = 'active'
LIMIT 100;

# custom values:
# table = "customers", status = "pending", limit = "50"
SELECT * 
FROM customers
WHERE status = 'pending'
LIMIT 50;
```

## 6. Template Management System

### Template Directory Structure

CQL organizes templates in a hierarchical directory structure:

```
~/.cql/templates/
  ├── common/     # standard templates that ship with CQL
  ├── user/       # user-created templates
  └── examples/   # example templates
```

You can also create custom categories as subdirectories.

### Creating Templates

To create a template, use the `template save` command in interactive mode:

```
> load my_query.cql
> template save my_template
```

Or save to a specific category:

```
> template save category/my_template
```

Templates are stored as .cql files in the templates directory.

### Loading Templates

To load a template:

```
> template load my_template
```

Or load from a specific category:

```
> template load category/my_template
```

The template content will be loaded into the current query buffer.

### Listing Available Templates

To list all available templates:

```
> templates
```

You can also list templates from the command line:

```bash
./cql --templates
```

### Template Metadata

To view metadata about a template:

```
> template info my_template
```

This shows:
- Template name
- Description
- Variables
- Last modified date
- Parent template (if inheriting)

### Template Categories

Templates can be organized into categories:

```
> category create reports
> template save reports/monthly_sales
```

To list all categories:

```
> categories
```

Categories help you organize templates by purpose, team, or application.

## 7. Template Inheritance

### Inheritance Basics with @inherit

Template inheritance allows one template to build upon another. Use the `@inherit` directive to specify a parent template:

```
@inherit "parent_template"
```

Child templates inherit all content and variables from their parent.

### Parent-Child Relationships

The child template can:
- Add new content
- Override variables
- Extend functionality

Example:
```
# parent: base_query.cql
@description "base query for user data"
@variable "table" "users"

SELECT * FROM ${table}

# child: active_users.cql
@inherit "base_query"
@description "query for active users only"
@variable "status" "active"

WHERE status = '${status}'
```

When compiled, the child template produces:
```
SELECT * FROM users
WHERE status = 'active'
```

### Variable Overriding

Child templates can override variables defined in the parent:

```
# parent
@variable "limit" "100"

# child
@inherit "parent"
@variable "limit" "50"  # overrides parent's value
```

The child template's variable values take precedence.

### Inheritance Chains

Templates can form inheritance chains:

```
base_template → intermediate_template → specialized_template
```

Each level can add or override functionality.

To view the inheritance chain for a template:

```
> template parents specialized_template
```

### Best Practices for Inheritance

- Create base templates with common structure
- Use intermediate templates for shared business logic
- Create specialized templates for specific use cases
- Keep inheritance chains reasonably short (3-4 levels max)
- Document the purpose of each inheritance level

## 8. Template Validation

### Validation Levels (ERROR, WARNING, INFO)

CQL supports three validation levels:
- **ERROR**: Critical issues that prevent proper functioning
- **WARNING**: Potential issues that should be addressed
- **INFO**: Informational messages about the template

### Common Validation Rules

CQL validates templates for:
- Syntax errors
- Missing variable declarations
- Undeclared variable references
- Circular inheritance
- Invalid directive usage
- Required directive checks

### Validating Templates from CLI

To validate a template from the command line:

```bash
./cql --validate my_template
```

To validate all templates:

```bash
./cql --validate-all
```

### Validating During Development

In interactive mode:

```
> template validate my_template
```

Or validate all templates:

```
> template validateall
```

Templates are also automatically validated when:
- Saving a template
- Creating an inherited template
- Using a template

### Handling Validation Issues

When validation issues are found, CQL provides detailed information:

```
Validation results for template 'my_template':
------------------------------------------
Found 1 errors, 2 warnings, 1 info messages.

Errors:
  - Variable 'status' is referenced but not declared

Warnings:
  - No description provided for variable 'limit'
  - Template missing @language directive

Info:
  - Template uses variable 'limit'
```

You can fix these issues or use the `--force` option to override ERROR-level validation issues.

## 9. Template Documentation

### Documentation Generation

CQL can automatically generate documentation for your templates based on directives and content:

```
> template docs my_template
```

This creates formatted documentation containing all relevant information about the template.

### Documentation Formats

Documentation can be generated in multiple formats:
- **Markdown**: Default format, great for GitHub and other markdown-compatible systems
- **HTML**: For web viewing with formatting and styling
- **Text**: Plain text for terminal viewing

### Variable Documentation

Variables are documented with their names, default values, and descriptions:

```
## Variables

| Name    | Description                          |
|---------|--------------------------------------|
| status  | Filter users by their account status |
| limit   | Maximum number of results to return  |
```

Use `@variable_description` to provide detailed descriptions of variables.

### Examples Documentation

Examples from the `@example` directive are included in the documentation:

```
## Example

```sql
SELECT * FROM users WHERE status = 'active' LIMIT 100;
```
```

Examples help users understand how to use the template.

### Exporting Documentation

To export documentation to a file:

```
> template export docs/my_template.md
```

Specify a format:

```
> template export docs/my_template.html html
```

From the command line:

```bash
./cql --export docs/all_templates.md
```

## 10. Command Line Interface

### Interactive Mode

Launch interactive mode:

```bash
./cql --interactive
```

Common interactive commands:
```
> help               # show help
> load file.cql      # load a file
> show               # show current query
> compile            # compile current query
> save file.sql      # save compiled query to file
> templates          # list available templates
> template load name # load a template
> exit               # exit interactive mode
```

### File Processing Mode

Process a file directly:

```bash
./cql input.cql output.sql
```

This compiles the input file and writes the result to the output file.

### Command Line Options

CQL supports numerous command line options:

```
--help, -h              Show help
--test, -t              Run tests
--examples, -e          Show examples
--interactive, -i       Interactive mode
--templates, -l         List templates
--template NAME, -T     Use a template
--validate NAME         Validate a template
--validate-all          Validate all templates
--docs NAME             Generate documentation
--docs-all              Generate all documentation
--export PATH [format]  Export documentation
```

### Template Operations via CLI

Work with templates from the command line:

```bash
# use a template with variables
./cql --template user_query status=active limit=50

# validate a template
./cql --validate user_query

# generate documentation
./cql --docs user_query
./cql --export documentation.md
```

### Batch Processing

For batch processing, you can:
1. Use shell scripts to run multiple CQL commands
2. Process multiple files with wildcards
3. Use the `--template` option with variables for parameterized queries

Example batch script:
```bash
#!/bin/bash
for template in template1 template2 template3; do
  ./cql --template $template status=active > ${template}_output.sql
done
```

## 11. Integration Examples

### Using CQL with Version Control

Store CQL templates in version control:

```bash
# add templates to git
git add ~/.cql/templates/user/
git commit -m "Add user query templates"

# or store templates directly in project repo
mkdir -p project/cql_templates
# configure CQL to use this directory
./cql --interactive
> template dir /path/to/project/cql_templates
```

### Integrating with Build Systems

Integrate CQL into your build process:

```makefile
# Makefile example
SQL_DIR = ./sql
CQL_DIR = ./cql
CQL_BIN = cql

sql: $(wildcard $(CQL_DIR)/*.cql)
	mkdir -p $(SQL_DIR)
	for file in $^; do \
		$(CQL_BIN) $$file $(SQL_DIR)/$$(basename $$file .cql).sql; \
	done
```

### Automated Documentation Workflows

Generate documentation automatically:

```bash
# script to generate documentation
mkdir -p docs/templates
./cql --export docs/templates/index.md
./cql --export docs/templates/reference.html html
```

### CI/CD Integration

Add CQL validation to CI/CD pipelines:

```yaml
# .github/workflows/validate-templates.yml
name: Validate CQL Templates

on:
  push:
    paths:
      - 'cql_templates/**'

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install CQL
        run: |
          # installation steps
      - name: Validate Templates
        run: |
          cql --validate-all
          if [ $? -ne 0 ]; then
            echo "Template validation failed"
            exit 1
          fi
```

## 12. Best Practices

### Organizing Templates

- Group related templates in categories
- Use consistent naming conventions
- Create a logical hierarchy of templates
- Document relationships between templates

Example organization:
```
templates/
  ├── common/
  │   ├── base_queries/
  │   └── util/
  ├── reports/
  │   ├── daily/
  │   └── monthly/
  └── admin/
```

### Naming Conventions

Consistent naming helps identify templates:

- Use lowercase with underscores for template names
- Use prefixes for related templates (e.g., `user_list`, `user_detail`)
- Include action verbs for clarity (e.g., `get_active_users`, `update_status`)
- Use suffixes to indicate variants (e.g., `report_daily`, `report_monthly`)

### Documentation Standards

Establish documentation standards:

- Always include a clear `@description`
- Document all variables with `@variable_description`
- Include at least one `@example`
- Add `@test` cases for important scenarios
- Use consistent formatting in documentation

### Modular Design

Design templates for modularity:

- Break complex queries into simpler components
- Use inheritance for shared structure
- Parameterize with variables rather than hardcoding values
- Create specialized templates that extend base templates

### Code Reuse with Templates

Maximize reuse:

- Create base templates with common structure
- Use variables for all customizable elements
- Leverage inheritance for common patterns
- Document how templates can be extended

## 13. Troubleshooting

### Common Errors and Solutions

**Compilation Errors**
- **Syntax errors**: Check for missing quotes or invalid directives
- **Variable reference errors**: Ensure all referenced variables are declared
- **Inheritance errors**: Verify parent templates exist and there are no circular references

**Validation Errors**
- **Missing directives**: Add required directives like `@description`
- **Undeclared variables**: Declare all variables before use
- **Incompatible inheritance**: Make sure child templates properly extend parents

**Runtime Errors**
- **File not found**: Check file paths and permissions
- **Template not found**: Verify template exists and name is correct
- **Invalid variable format**: Ensure variables use `${variable}` syntax

### Debugging Techniques

1. **Enable verbose logging**:
   ```bash
   export CQL_LOG_LEVEL=DEBUG
   ```

2. **Inspect intermediate results**:
   ```
   > load template
   > show
   ```

3. **Validate template structure**:
   ```
   > template validate my_template
   ```

4. **Check template metadata**:
   ```
   > template info my_template
   ```

5. **Temporary file inspection**:
   ```bash
   ./cql --debug input.cql output.sql
   ```

### Performance Optimization

For large template collections:
- Organize templates into categories
- Use specific template paths rather than wildcards
- Limit inheritance chain depth
- Consider template compilation caching

### Support Resources

- CQL GitHub repository: [github.com/example/cql](https://github.com/example/cql)
- Documentation website: [cql-docs.example.com](https://cql-docs.example.com)
- Issue tracker: [github.com/example/cql/issues](https://github.com/example/cql/issues)
- Community forum: [community.example.com/cql](https://community.example.com/cql)

## 14. Advanced Topics

### Creating Custom Validation Rules

You can extend CQL's validation system with custom rules:

```cpp
// Custom validation rule example
TemplateValidatorRule createCustomRule() {
    TemplateValidatorRule rule;
    rule.name = "company_copyright_check";
    rule.level = TemplateValidationLevel::WARNING;
    rule.description = "Checks if company copyright is present";
    
    rule.validate_function = [](const std::string& content) {
        if (content.find("@copyright") == std::string::npos ||
            content.find("ExampleCorp") == std::string::npos) {
            return TemplateValidationIssue(
                TemplateValidationLevel::WARNING,
                "Missing ExampleCorp copyright"
            );
        }
        return std::nullopt;
    };
    
    return rule;
}
```

### Extending CQL

CQL can be extended in several ways:

1. **New directives**: Add new directive types to the parser
2. **Custom validators**: Create validation rules for specific needs
3. **Output formats**: Add new compilation output formats
4. **Integration plugins**: Create plugins for CI/CD systems

### Plugin Development

Create plugins for CQL:

```cpp
// Plugin example (conceptual)
class CqlPlugin {
public:
    void initialize(CqlEngine& engine) {
        // Register custom directives
        engine.register_directive("custom_directive", 
                                 &CqlPlugin::handle_custom_directive);
        
        // Add validation rules
        engine.add_validation_rule(createCustomRule());
    }
    
private:
    static void handle_custom_directive(DirectiveContext& ctx) {
        // Implementation
    }
};
```

### API Usage

Use CQL programmatically in your applications:

```cpp
#include "cql/cql.hpp"

int main() {
    // Initialize CQL
    cql::CqlEngine engine;
    
    // Load a template
    auto template_content = engine.load_template("my_template");
    
    // Set variables
    std::map<std::string, std::string> variables;
    variables["status"] = "active";
    variables["limit"] = "50";
    
    // Compile the template
    std::string result = engine.compile(template_content, variables);
    
    // Use the result
    std::cout << result << std::endl;
    
    return 0;
}
```

## Appendix A: CQL Directive Reference

| Directive | Parameters | Description |
|-----------|------------|-------------|
| @copyright | "License" "Holder" | Specifies copyright information |
| @language | "Language" | Specifies the target language |
| @description | "Text" | Provides a description of the query |
| @context | "Text" | Provides context information |
| @dependency | "Text" | Specifies a dependency |
| @test | "Description" | Defines a test case |
| @architecture | "Description" | Specifies architecture pattern |
| @constraint | "Description" | Defines a constraint |
| @security | "Description" | Specifies security requirements |
| @complexity | "Description" | Specifies complexity characteristics |
| @example | "Name" "Content" | Provides an example |
| @variable | "Name" "Default" | Defines a variable |
| @variable_description | "Name" "Description" | Describes a variable |
| @inherit | "Template" | Specifies a parent template |

## Appendix B: Command Reference

| Command | Description |
|---------|-------------|
| `./cql --help` | Show help information |
| `./cql --test` | Run the test suite |
| `./cql --examples` | Show example queries |
| `./cql --interactive` | Run in interactive mode |
| `./cql --templates` | List available templates |
| `./cql --template NAME` | Use a specific template |
| `./cql --validate NAME` | Validate a template |
| `./cql --validate-all` | Validate all templates |
| `./cql --docs NAME` | Generate documentation |
| `./cql --docs-all` | Generate all documentation |
| `./cql --export PATH [format]` | Export documentation |
| `./cql input.cql [output.sql]` | Process a file |

## Appendix C: Configuration Options

CQL supports configuration through environment variables:

| Variable | Purpose | Default |
|----------|---------|---------|
| CQL_TEMPLATE_DIR | Templates directory | ~/.cql/templates |
| CQL_LOG_LEVEL | Logging verbosity | INFO |
| CQL_DEFAULT_FORMAT | Default export format | markdown |
| CQL_STRICT_MODE | Enforce strict validation | false |
| CQL_COLOR_OUTPUT | Enable colored output | true |

## Appendix D: Example Template Library

The CQL installation includes example templates:

| Template | Description |
|----------|-------------|
| `common/base_query.cql` | Basic query structure |
| `common/pagination.cql` | Query with pagination |
| `examples/user_query.cql` | User data query example |
| `examples/reporting.cql` | Reporting query example |
| `examples/variables.cql` | Variable usage example |
| `examples/inheritance.cql` | Inheritance example |

Access these examples with:
```bash
./cql --template common/base_query
```

Or in interactive mode:
```
> template load examples/variables
```

These examples serve as a starting point for your own templates.
