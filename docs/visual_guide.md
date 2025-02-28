# CQL Visual Guide

This document provides visual representations of key CQL concepts and workflows to help you better understand the system.

## CQL Compilation Pipeline

```
┌────────────┐     ┌────────────┐     ┌────────────┐     ┌────────────┐
│            │     │            │     │            │     │            │
│   Input    │────▶│   Lexer    │────▶│   Parser   │────▶│  Compiler  │
│            │     │            │     │            │     │            │
└────────────┘     └────────────┘     └────────────┘     └────────────┘
                                                               │
                                                               ▼
┌────────────┐     ┌────────────┐     ┌────────────┐     ┌────────────┐
│            │     │            │     │            │     │            │
│   Output   │◀────│ Formatter  │◀────│ Validator  │◀────│   Nodes    │
│            │     │            │     │            │     │            │
└────────────┘     └────────────┘     └────────────┘     └────────────┘
```

### Pipeline Stages

1. **Input**: CQL query file (`.cql`) or string
2. **Lexer**: Tokenizes the input into a stream of tokens
3. **Parser**: Converts tokens into an AST (Abstract Syntax Tree)
4. **Compiler**: Processes nodes and applies transformations
5. **Nodes**: The intermediate representation of the query
6. **Validator**: Checks for errors and warnings
7. **Formatter**: Formats the output according to rules
8. **Output**: Generated code or formatted text

## Template Inheritance

```
┌────────────────────────┐
│                        │
│    Base Template       │
│                        │
│  @variable "x" "1"     │
│  @variable "y" "2"     │
│                        │
└────────────┬───────────┘
             │
             │ inherits
             ▼
┌────────────────────────┐
│                        │
│    Child Template      │
│                        │
│  @inherit "base"       │
│  @variable "y" "3"     │ ──┐
│  @variable "z" "4"     │   │ overrides base.y
│                        │   │
└────────────┬───────────┘   │
             │               │
             │ inherits      │
             ▼               │
┌────────────────────────┐   │
│                        │   │
│  Grandchild Template   │   │
│                        │   │
│  @inherit "child"      │   │
│  @variable "w" "5"     │   │
│                        │   │
└────────────────────────┘   │
             │               │
             │ resolves to   │
             ▼               │
┌────────────────────────┐   │
│                        │   │
│  Effective Template    │   │
│                        │   │
│  @variable "x" "1"  ◀──────┘
│  @variable "y" "3"     │
│  @variable "z" "4"     │
│  @variable "w" "5"     │
│                        │
└────────────────────────┘
```

### Inheritance Rules

1. Child templates inherit all variables and directives from parent
2. Child definitions override parent definitions with the same name
3. Inheritance is transitive (grandchild inherits from base through child)
4. Circular inheritance is detected and prevented

## Template Variable Resolution

```
┌────────────────────────────────────────────────────┐
│                                                    │
│  Template Definition                               │
│                                                    │
│  @variable "container" "vector"                    │
│  @variable "type" "int"                            │
│  @description "implement a ${container}<${type}>"  │
│                                                    │
└───────────────────────┬────────────────────────────┘
                        │
                        │ instantiation
                        ▼
┌────────────────────────────────────────────────────┐
│                                                    │
│  Variable Map                                      │
│  ┌─────────────┬─────────────┐                     │
│  │    Name     │    Value    │                     │
│  ├─────────────┼─────────────┤                     │
│  │  container  │    list     │  ◀── override       │
│  │     type    │   string    │  ◀── override       │
│  └─────────────┴─────────────┘                     │
│                                                    │
└───────────────────────┬────────────────────────────┘
                        │
                        │ resolution
                        ▼
┌────────────────────────────────────────────────────┐
│                                                    │
│  Resolved Template                                 │
│                                                    │
│  @variable "container" "list"                      │
│  @variable "type" "string"                         │
│  @description "implement a list<string>"           │
│                                                    │
└────────────────────────────────────────────────────┘
```

### Variable Resolution Process

1. Collect all variables from the template and parent templates
2. Apply overrides from provided variable map
3. Scan template content for variable references (`${name}`)
4. Replace each reference with its resolved value
5. Report errors for undefined variables

## Template Validation

```
┌────────────┐
│            │
│  Template  │
│            │
└──────┬─────┘
       │
       │
       ▼
┌──────────────┐    ┌───────────────────┐
│              │    │                   │
│  Validator   │───▶│  Validation Rules │
│              │    │                   │
└──────┬───────┘    └───────────────────┘
       │
       │
       ▼
┌──────────────────────────────────────┐
│                                      │
│         Validation Result            │
│                                      │
│  ┌──────────┬────────────────────┐   │
│  │  Level   │      Message       │   │
│  ├──────────┼────────────────────┤   │
│  │  ERROR   │ Circular inherit.  │   │
│  │ WARNING  │ Unused variable    │   │
│  │  INFO    │ No test cases      │   │
│  └──────────┴────────────────────┘   │
│                                      │
└──────────────────────────────────────┘
```

### Validation Levels

1. **ERROR**: Critical issues that prevent compilation
2. **WARNING**: Potential problems that should be addressed
3. **INFO**: Suggestions for improving the template

## File Organization

```
cql/
│
├── templates/
│   ├── common/              # System templates
│   │   ├── base.cql
│   │   └── ...
│   │
│   └── user/                # User-defined templates
│       ├── my_template.cql
│       └── category/
│           └── template.cql
│
├── examples/                # Example CQL files
│   ├── basic.cql
│   └── ...
│
└── output/                  # Generated output (optional)
    └── ...
```

### Directory Structure Rules

1. Templates are organized in a hierarchical structure
2. `common` directory contains system templates
3. `user` directory contains user-defined templates
4. Categories create subdirectories in the appropriate location
5. Template paths use forward slashes for category separation

## Component Interaction

```
            ┌───────────────┐
            │               │
            │     User      │
            │               │
            └───────┬───────┘
                    │
                    │ input
                    ▼
┌──────────────────────────────────────┐
│                                      │
│              CQL CLI                 │
│                                      │
└──┬─────────────────┬─────────────┬───┘
   │                 │             │
   │                 │             │
   ▼                 ▼             ▼
┌─────────┐   ┌────────────┐  ┌──────────┐
│         │   │            │  │          │
│ Compiler│   │ Template   │  │Validator │
│         │   │ Manager    │  │          │
└────┬────┘   └─────┬──────┘  └────┬─────┘
     │              │              │
     │              │              │
     └──────────────┼──────────────┘
                    │
                    ▼
            ┌───────────────┐
            │               │
            │  File System  │
            │               │
            └───────────────┘
```

### Component Responsibilities

1. **CQL CLI**: Command-line interface for user interaction
2. **Compiler**: Processes CQL queries and generates output
3. **Template Manager**: Handles template storage and retrieval
4. **Validator**: Validates templates for errors and warnings
5. **File System**: Stores templates and output files

## Execution Flow

```
 ┌─────────────┐
 │  Start CQL  │
 └──────┬──────┘
        │
        ▼
┌────────────────┐
│  Parse Command │
│  Line Args     │
└────────┬───────┘
         │
         ▼
     ┌───────┐      ┌─────────────┐
     │ Mode? │──────▶ Interactive │
     └───┬───┘      └──────┬──────┘
         │                 │
         │                 ▼
         │          ┌────────────┐     ┌─────────────┐
         │          │ Read Input │◀────▶ Process CMD │
         │          └──────┬─────┘     └─────────────┘
         │                 │
         │                 │
         ▼                 │
┌─────────────────┐        │
│ Process File    │        │
└────────┬────────┘        │
         │                 │
         ▼                 │
┌────────────────┐         │
│ Generate Output│         │
└────────┬───────┘         │
         │                 │
         ▼                 │
  ┌────────────┐           │
  │ Write File │           │
  └──────┬─────┘           │
         │                 │
         └─────────────────┘
                │
                ▼
          ┌──────────┐
          │   End    │
          └──────────┘
```

### Operation Modes

1. **File Mode**: Process a CQL file and generate output
2. **Interactive Mode**: Provide a REPL for template management
3. **Test Mode**: Run the test suite
4. **Example Mode**: Run the examples

## Visitor Pattern

```
┌─────────────┐
│             │
│    Node     │◀───┐
│             │    │
└──────┬──────┘    │
       │           │
       │           │
       ▼           │
┌─────────────┐    │    ┌─────────────┐
│             │    │    │             │
│   Visitor   │    │    │ Directive   │
│             │    │    │   Node      │
└──────┬──────┘    │    │             │
       │           │    └──────┬──────┘
       │           │           │
       │           │           │
       │           │           │
       │           │    ┌──────┴──────┐
       │           │    │             │
       │           └────┤  Variable   │
       │                │    Node     │
       │                │             │
       │                └─────────────┘
       │
       │
       ▼
┌─────────────────────────────┐
│                             │
│       Concrete Visitor      │
│                             │
│  visit(DirectiveNode)       │
│  visit(VariableNode)        │
│                             │
└─────────────────────────────┘
```

### Visitor Pattern Implementation

1. Nodes declare an `accept(Visitor&)` method
2. Visitors declare `visit(Node&)` methods for each node type
3. Nodes call the appropriate visitor method from their accept method
4. This enables processing nodes without modifying their classes
