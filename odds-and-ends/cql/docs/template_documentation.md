# CQL Template Documentation System

This document describes how to generate and work with documentation for CQL templates.

## Overview

The CQL Template Documentation System allows you to:

1. Generate documentation for individual templates
2. Generate documentation for all templates in your library
3. Export documentation in various formats (Markdown, HTML, Text)
4. Include additional metadata about variables, examples, and inheritance

## CLI Commands

### Interactive Mode

When running CQL in interactive mode (`cql --interactive`), you can use these commands:

- `template docs NAME` - Generate documentation for a specific template
- `template docsall` - Generate documentation for all templates
- `template export PATH [format]` - Export documentation to a file (formats: md, html, txt)

### Command Line Options

You can also use these commands directly from the command line:

- `cql --docs NAME` - Generate documentation for a specific template
- `cql --docs-all` - Generate documentation for all templates
- `cql --export PATH [format]` - Export documentation to a file

## Template Documentation Features

### Variable Descriptions

You can add descriptions for your template variables using the `@variable_description` directive:

```sql
@variable "table_name" "users"
@variable_description "table_name" "The database table to query"
```

### Example Usage

You can provide example usage of your template with the `@example` directive:

```sql
@example "SELECT * FROM ${table_name} LIMIT 100;"
```

### Documentation Format

The generated documentation for each template includes:

- Template name and description
- Last modified date
- Parent template (if using inheritance)
- Variables and their descriptions
- Example usage
- Inheritance chain (if applicable)
- File location

## Export Formats

The documentation can be exported in three formats:

1. **Markdown** (default) - For rendering in GitHub or other Markdown viewers
2. **HTML** - For viewing in a web browser with formatting
3. **Text** - Plain text format for terminal viewing

Example export commands:

```bash
# Export as Markdown (default)
cql --export docs/templates.md

# Export as HTML
cql --export docs/templates.html html

# Export as plain text
cql --export docs/templates.txt text
```

## Best Practices

1. Add a good description to each template using `@description`
2. Document each variable with `@variable_description`
3. Include at least one usage example with `@example`
4. Update documentation when you modify templates
5. Export documentation regularly to maintain up-to-date reference material
