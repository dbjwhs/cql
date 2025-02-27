# CQL Template Examples

This directory contains example templates demonstrating CQL features, including the new template inheritance system.

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

## Other Examples

- `variables.cql` - Simple example of variable usage
- `json_output.cql` - Example template formatted for JSON output
- `phase2_features.cql` - Demonstrates features from Phase 2 of CQL development

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