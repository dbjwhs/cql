# CQL Interactive Mode Guide

## Introduction

The interactive mode in Claude Query Language (CQL) provides a powerful command-line interface for creating, editing, and managing queries and templates in real-time. This REPL (Read-Eval-Print Loop) environment allows you to develop queries incrementally, test templates with different variables, and perform various operations without having to create separate files for each step.

## Getting Started

### Launching Interactive Mode

To start the interactive mode, use the `--interactive` or `-i` flag:

```bash
cql --interactive
```

Upon launching, you'll see a welcome message and a prompt:

```
CQL Interactive Mode
Type 'exit' to quit, 'help' for command list
>
```

### Basic Interaction

In interactive mode, each line you enter is either:
1. A command (starting at the beginning of the line)
2. Part of the query content being built

Commands are processed immediately, while other input lines are added to the current query buffer.

### Exiting Interactive Mode

To exit the interactive session:

```
> exit
```

or

```
> quit
```

## Core Commands

### Help and Navigation

| Command | Description |
|---------|-------------|
| `help` | Display the list of available commands |
| `exit` or `quit` | Exit the interactive mode |

### Query Management

| Command | Description |
|---------|-------------|
| `clear` | Clear the current query buffer |
| `show` | Display the current query content |
| `compile` | Compile the current query |
| `load FILE` | Load query content from a file |
| `save FILE` | Save the compiled query to a file |

Example usage:

```
> load my_query.cql
Loaded query from my_query.cql
> show
@language "C++"
@description "implement a thread-safe queue"
> compile
=== Compiled Query ===

Please generate C++ code that:
implement a thread-safe queue

Quality Assurance Requirements:
- All code must be well-documented with comments
- Follow modern C++ best practices
- Ensure proper error handling
- Optimize for readability and maintainability
===================
> save output.txt
Saved compiled query to output.txt
```

## Template Management

### Listing Templates and Categories

| Command | Description |
|---------|-------------|
| `templates` | List all available templates |
| `categories` | List all template categories |
| `category create NAME` | Create a new template category |

### Template Directory Management

| Command | Description |
|---------|-------------|
| `template dir` | Show the current templates directory |
| `template dir PATH` | Set a new templates directory |

### Basic Template Operations

| Command | Description |
|---------|-------------|
| `template save NAME` | Save the current query as a template |
| `template load NAME` | Load a template into the current query buffer |
| `template info NAME` | Show metadata about a template |
| `template delete NAME` | Delete a template |

Example for saving and loading templates:

```
> @language "SQL"
> @description "retrieve active users"
> 
> SELECT * FROM users WHERE status = 'active';
> template save active_users
Query saved as template: active_users
> clear
Query cleared
> template load active_users
Template loaded: active_users
> show
@language "SQL"
@description "retrieve active users"

SELECT * FROM users WHERE status = 'active';
```

## Variable Management

| Command | Description |
|---------|-------------|
| `template setvar NAME=VALUE` | Set a template variable |
| `template vars` | Show all currently defined variables |
| `template clearvars` | Clear all current variables |
| `template vars NAME` | List variables in a specific template |
| `template setvars` | Enter multiple variables interactively |

Variables can be set and used within templates:

```
> @language "SQL"
> @description "retrieve users with specific status"
> @variable "status" "active"
> 
> SELECT * FROM users WHERE status = '${status}';
> template save user_query
Query saved as template: user_query
> template setvar status=inactive
Variable added: status=inactive
> compile
=== Compiled Query ===

SELECT * FROM users WHERE status = 'inactive';
===================
```

### Interactive Variable Setting

The `template setvars` command allows you to set multiple variables in sequence:

```
> template setvars
Enter variables in NAME=VALUE format (empty line to finish):
var> table=customers
Variable set: table=customers
var> status=pending
Variable set: status=pending
var> limit=50
Variable set: limit=50
var> 
Finished setting variables
```

## Template Instantiation

| Command | Description |
|---------|-------------|
| `template use NAME` | Instantiate a template with current variables |

This command loads a template and substitutes all variables with their current values:

```
> template vars
Current variables:
  table = "customers"
  status = "pending"
  limit = "50"
> template use user_query
Template instantiated: user_query
> show
@language "SQL"
@description "retrieve users with specific status"
@variable "status" "pending"

SELECT * FROM users WHERE status = 'pending';
```

## Template Inheritance

| Command | Description |
|---------|-------------|
| `template inherit CHILD PARENT` | Create a template inheriting from another |
| `template parents NAME` | Show inheritance chain for a template |

Inheritance allows templates to build on each other:

```
> template load user_query
Template loaded: user_query
> @variable "order_by" "name"
> 
> ORDER BY ${order_by};
> template inherit user_query_sorted user_query
Created template 'user_query_sorted' inheriting from 'user_query'
> template parents user_query_sorted
Inheritance chain for 'user_query_sorted':
  Base: user_query
  Current: user_query_sorted
```

## Template Validation

| Command | Description |
|---------|-------------|
| `template validate NAME` | Validate a specific template |
| `template validateall` | Validate all templates |

Validation checks templates for issues and errors:

```
> template validate user_query
Validation results for template 'user_query':
------------------------------------------
Found 0 errors, 0 warnings, 1 info messages.

Info:
  - Template uses variable 'status'
```

## Template Documentation

| Command | Description |
|---------|-------------|
| `template docs NAME` | Generate documentation for a template |
| `template docsall` | Generate documentation for all templates |
| `template export PATH [format]` | Export documentation to a file (formats: md, html, txt) |

These commands generate detailed documentation about templates:

```
> template docs user_query

===== Template Documentation =====

# User Query

## Description
Retrieve users with specific status

## Language
SQL

## Variables
- status: Filter users by their account status (default: "active")

## Query
```sql
SELECT * FROM users WHERE status = '${status}';
```

==================================
```

## API Integration in Interactive Mode

When used with the `--interactive-api` option or within normal interactive mode, you can interact with the Claude API:

```
> compile
=== Compiled Query ===
...compiled query content...
===================
> submit
Submitting to Claude API (model: claude-3-opus)...
API request successful
Generated 3 files:
  - thread_safe_queue.hpp
  - thread_safe_queue.cpp
  - thread_safe_queue_test.cpp
Files saved to current directory
```

## Command Design Patterns

Interactive mode implements a command pattern design with a registry of handlers for different command types:

1. Each command has a matcher function that identifies if an input line matches the command
2. Each command has a handler function that implements the command's behavior
3. Commands are processed in order until a matching handler is found

This architecture makes the interactive mode easily extensible with new commands.

## Best Practices for Interactive Mode

1. **Building Complex Queries**:
   - Use `clear` to start fresh
   - Add directives one by one
   - Use `show` to verify content
   - Use `compile` to check the final result

2. **Working with Templates**:
   - Create base templates for common structures
   - Use inheritance for specialization
   - Use variables for all customizable elements
   - Validate templates after creating them

3. **Variable Management**:
   - Set variables before using templates
   - Use `template setvars` for setting multiple variables
   - Check variables with `template vars`
   - Clear variables between different tasks

4. **Workflow Optimization**:
   - Save frequently used queries as templates
   - Create categories for organizing templates
   - Use template validation to ensure quality
   - Document templates using the built-in tools

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Command not recognized | Check spelling and use `help` to see available commands |
| Template not found | Verify the template name and directory with `templates` and `template dir` |
| Variable substitution not working | Ensure variables are defined with `template vars` and properly referenced with `${name}` |
| Validation errors | Use `template validate NAME` to get detailed error information |
| Compilation errors | Check for syntax errors in the query with `show` |

## Examples

### Creating a Parameterized SQL Query

```
> clear
Query cleared
> @language "SQL"
> @description "retrieve users by status with limit"
> @variable "status" "active"
> @variable "limit" "100"
> 
> SELECT * FROM users
> WHERE status = '${status}'
> LIMIT ${limit};
> compile
=== Compiled Query ===
SELECT * FROM users
WHERE status = 'active'
LIMIT 100;
===================
> template save user_query_with_limit
Query saved as template: user_query_with_limit
```

### Using Template Inheritance

```
> template load user_query_with_limit
Template loaded: user_query_with_limit
> @variable "order_field" "last_login"
> @variable "order_dir" "DESC"
> 
> ORDER BY ${order_field} ${order_dir};
> template inherit user_query_sorted user_query_with_limit
Created template 'user_query_sorted' inheriting from 'user_query_with_limit'
> template use user_query_sorted
Template instantiated: user_query_sorted
> compile
=== Compiled Query ===
SELECT * FROM users
WHERE status = 'active'
LIMIT 100;
ORDER BY last_login DESC;
===================
```

### Interactive Template Customization

```
> template load user_query_sorted
Template loaded: user_query_sorted
> template setvars
Enter variables in NAME=VALUE format (empty line to finish):
var> status=inactive
Variable set: status=inactive
var> limit=50
Variable set: limit=50
var> order_field=created_at
Variable set: order_field=created_at
var> order_dir=ASC
Variable set: order_dir=ASC
var> 
Finished setting variables
> compile
=== Compiled Query ===
SELECT * FROM users
WHERE status = 'inactive'
LIMIT 50;
ORDER BY created_at ASC;
===================
```

## Conclusion

The interactive mode in CQL provides a powerful and flexible environment for developing, testing, and managing queries and templates. By combining immediate feedback with comprehensive template management, it enables efficient workflow and encourages best practices in query development.

For more information about CQL and its features, refer to the other documentation files in the `docs` directory, particularly the tutorial and quick reference guide.