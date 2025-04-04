# CQL Template Examples

This directory contains example templates demonstrating CQL features, including the new template inheritance system and API integration capabilities.

## 📚 API Reference Documentation

**[Complete API Reference](https://dbjwhs.github.io/cql/)**: Visit our [Doxygen site](https://dbjwhs.github.io/cql/) for comprehensive documentation of all classes, methods, and interfaces.

## Template Inheritance Examples

The following examples demonstrate template inheritance:

### Base Template
- `base_data_structure.cql` - Contains common properties for data structures

### Derived Templates
- `vector_template.cql` - Inherits from `base_data_structure` and adds vector-specific properties
- `concurrent_vector.cql` - Inherits from `vector_template` and adds thread-safety properties

These templates showcase a three-level inheritance chain:
`base_data_structure` → `vector_template` → `concurrent_vector`

## Variable Templates

- `default_values.cql` - Demonstrates template with variable declarations and default values
- `template_example.cql` - Shows template with variable references but no default values

## API Integration Examples

These examples demonstrate using CQL with Claude API integration:

- `api_basic_example.cql` - Simple query for API submission
- `api_streaming_example.cql` - Query configured for streaming responses
- `api_advanced_example.cql` - Complex query with multiple file generation

To use these examples with the API:

```bash
# Basic API submission
cql --submit examples/api_basic_example.cql

# API submission with output directory
cql --submit examples/api_advanced_example.cql --output-dir ./generated_code

# API submission with specific model
cql --submit examples/api_streaming_example.cql --model claude-3-opus
```

## Other Examples

- `variables.cql` - Simple example of variable usage
- `json_output.cql` - Example template formatted for JSON output
- `distributed_task_scheduler.cql` - Comprehensive example with advanced features
- `advanced_features.md` - Documentation of advanced CQL features

## Using Template Inheritance

To create a new template that inherits from an existing one:

```
template inherit my_new_template parent_template
```

This will create a template that inherits all properties and variables from the parent template, which you can then override or extend.

To see the inheritance chain for a template:

```
template parents template_name
```

When a template with inheritance is instantiated, the content from the parent templates is merged with the child template, with child properties overriding parent properties when there are conflicts.
