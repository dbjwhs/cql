# CQL Troubleshooting Guide

This document provides solutions for common issues encountered when working with the Claude Query Language (CQL).

## Compilation Issues

### Error: Invalid directive

**Symptoms:**
```
Error: Invalid directive name: invalid-directive
```

**Causes:**
- Using a directive that doesn't exist
- Using hyphens instead of underscores in directive names
- Typos in directive names

**Solutions:**
1. Check the directive name against the [Quick Reference Guide](quick_reference.md)
2. Replace hyphens with underscores in directive names
3. Ensure all directives start with `@`

**Example Fix:**
```diff
- @invalid-directive "value"
+ @valid_directive "value"
```

### Error: Unterminated string

**Symptoms:**
```
Error: Unterminated string starting at line 5
```

**Causes:**
- Missing closing quotation mark in directive values
- Newlines inside string literals without proper escaping

**Solutions:**
1. Ensure all strings have matching opening and closing quotes
2. For multi-line strings, use proper escaping:
   ```
   @description "This is a multi-line
   description that needs proper
   quotation marks"
   ```

### Warning: Undeclared variable

**Symptoms:**
```
Warning: Undeclared variable: max_size
```

**Causes:**
- Using a variable that hasn't been declared with `@variable`
- Typo in variable name
- Variable was declared in a parent template that's not properly inherited

**Solutions:**
1. Declare the variable with `@variable "max_size" "default_value"`
2. Check for typos in variable names
3. Ensure parent templates are properly referenced with `@inherit`
4. Check case sensitivity in variable names

**Example Fix:**
```diff
+ @variable "max_size" "1000"
  @description "Implement a queue with ${max_size} elements"
```

### Error: Circular inheritance

**Symptoms:**
```
Error: Circular inheritance detected: Inheritance cycle: template_a -> template_b -> template_a
```

**Causes:**
- Templates that directly or indirectly inherit from themselves

**Solutions:**
1. Redesign the template hierarchy to remove circular references
2. Use a common parent template instead of circular inheritance
3. Combine the templates if possible

**Example Fix:**
```diff
// template_a.cql
- @inherit "template_b"
+ @inherit "common_parent"
  @description "Template A"

// template_b.cql
- @inherit "template_a"
+ @inherit "common_parent"
  @description "Template B"
```

## Template Management Issues

### Error: Template not found

**Symptoms:**
```
Error: Template not found: my_template
```

**Causes:**
- The template doesn't exist in the templates directory
- Template name mismatch (case sensitivity, typos)
- Template directory not properly configured

**Solutions:**
1. Check if the template exists using `list` command in interactive mode
2. Create the template if it doesn't exist
3. Check for typos or case sensitivity issues
4. Verify the templates directory structure (should have `common` and `user` subdirectories)

### Error: Failed to create category

**Symptoms:**
```
Error: Failed to create category: invalid/name
```

**Causes:**
- Invalid characters in category name
- Insufficient permissions to create directory
- Nested categories specified incorrectly

**Solutions:**
1. Use only alphanumeric characters and underscores in category names
2. Check filesystem permissions
3. Create parent categories first before creating nested categories

**Example Fix:**
```diff
- create invalid/name
+ create parent_category
+ create parent_category/child_category
```

## Variable Issues

### Warning: Unused variable

**Symptoms:**
```
Warning: Unused variable: unused_var
```

**Causes:**
- Variable declared but not used in the template

**Solutions:**
1. Remove the unused variable declaration
2. Use the variable in the template content with `${unused_var}`

**Example Fix:**
```diff
  @variable "unused_var" "value"
+ @description "Using ${unused_var} in the description"
```

### Error: Variable name contains invalid characters

**Symptoms:**
```
Error: Invalid variable name: invalid-name
```

**Causes:**
- Using invalid characters in variable names (hyphens, spaces, etc.)

**Solutions:**
1. Use only alphanumeric characters and underscores in variable names
2. Replace hyphens with underscores

**Example Fix:**
```diff
- @variable "invalid-name" "value"
+ @variable "valid_name" "value"
```

## Command Line Issues

### Error: Unable to open file

**Symptoms:**
```
Error: Unable to open file: /path/to/input.cql
```

**Causes:**
- File doesn't exist at the specified path
- Insufficient permissions to read the file
- Path contains spaces or special characters

**Solutions:**
1. Verify the file exists at the specified path
2. Check file permissions
3. If the path contains spaces, enclose it in quotes: `"path with spaces/input.cql"`

### Error: Failed to create template directory

**Symptoms:**
```
Error: Failed to create template directory: /path/to/templates
```

**Causes:**
- Insufficient permissions to create directory
- Path doesn't exist
- Invalid path format

**Solutions:**
1. Check filesystem permissions
2. Create parent directories first
3. Use an absolute path
4. Check for special characters in the path

## Validation Issues

### Error: Malformed template

**Symptoms:**
```
Error: Malformed template: Expected string after directive
```

**Causes:**
- Syntax errors in the template content
- Missing quotation marks around directive values
- Invalid directive format

**Solutions:**
1. Ensure all directives follow the format: `@directive "value"`
2. Check for missing or mismatched quotation marks
3. Validate the template using the validator

**Example Fix:**
```diff
- @description implement a stack
+ @description "implement a stack"
```

### Error: Invalid parent template

**Symptoms:**
```
Error: Invalid parent template: non_existent_parent
```

**Causes:**
- Referenced parent template doesn't exist
- Typo in parent template name

**Solutions:**
1. Create the parent template if it doesn't exist
2. Check for typos in the parent template name
3. Use `list` command to see available templates

**Example Fix:**
```diff
- @inherit "non_existent_parent"
+ @inherit "existing_parent"
```

## Directory Setup Issues

### Error: Template directory structure missing

**Symptoms:**
```
Error: Required template directory structure missing. Expected common and user subdirectories.
```

**Causes:**
- Template directory doesn't have the required subdirectories
- Permissions issues

**Solutions:**
1. Create the required directory structure:
   ```
   templates/
   ├── common/
   └── user/
   ```

2. Ensure proper permissions for the directories

### Error: Template path contains invalid characters

**Symptoms:**
```
Error: Template path contains invalid characters: template/with/invalid#chars
```

**Causes:**
- Using special characters not allowed in file paths

**Solutions:**
1. Use only alphanumeric characters, underscores, and forward slashes in template paths
2. Replace special characters with underscores

**Example Fix:**
```diff
- save template/with/invalid#chars
+ save template/with/invalid_chars
```

## General Troubleshooting Steps

1. **Enable verbose logging**:
   ```
   cql --verbose input.cql output.txt
   ```

2. **Validate a template**:
   ```
   cql --validate my_template
   ```

3. **Check template structure**:
   ```
   cql --info my_template
   ```

4. **Inspect template inheritance**:
   ```
   cql --inheritance my_template
   ```

5. **List all errors and warnings**:
   ```
   cql --strict input.cql output.txt
   ```

## Getting Help

If you're still experiencing issues after trying the solutions in this guide:

1. Check the [full CQL documentation](tutorial.md)
2. Look at the [example templates](../examples/) for reference
3. Search for similar issues in the project issues
4. Ask for help with a complete description of your issue, including:
   - The exact error message
   - The template content
   - The command you're running
   - Any relevant environment details
