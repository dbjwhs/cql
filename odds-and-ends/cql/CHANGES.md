# Change Log

## Phase 3: Template Management System

### Template Inheritance Support (2025-02-26)
- Added template inheritance with `@inherit` directive
- Implemented inheritance chain management
- Added circular inheritance detection
- Added variable override support in inheritance hierarchy
- Added CLI commands for working with template inheritance
  - `template inherit CHILD PARENT` - Create a child template that inherits from a parent
  - `template parents NAME` - Show the inheritance chain for a template
- Added inheritance information to template metadata
- Updated `instantiate_template` to work with inherited templates
- Added comprehensive tests for inheritance functionality
- Created example templates demonstrating inheritance chains

### Template Variable System (2025-02-20)
- Standardized comment styling in codebase
- Added variable replacement system
- Implemented variable collection from templates
- Added CLI commands for working with template variables
  - `template vars` - Show current variables in memory
  - `template clearvars` - Clear all current variables
  - `template setvars` - Enter variables interactively
- Enhanced template use to combine in-memory and template variables

### Template Directory Structure (2025-02-15)
- Implemented standard directory structure (common/user)
- Added directory structure validation
- Added repair functionality for invalid structures
- Enhanced path handling to support category/template format
- Added README explaining directory structure

## Phase 2: Enhanced Query Directives (2025-02-10)

### New Directives
- Added `@architecture` directive
- Added `@constraint` directive
- Added `@security` directive
- Added `@complexity` directive
- Added `@example` directive with named examples
- Added `@variable` directive for template parameters

### Variable Interpolation
- Implemented `${variable}` syntax for variable references
- Added variable extraction and resolution
- Added variable validation

## Phase 1: Basic CQL Implementation (2025-02-01)

### Initial Features
- Implemented basic directive parsing
- Added `@copyright` directive
- Added `@language` directive
- Added `@description` directive
- Added `@context` directive
- Added `@dependency` directive
- Added `@test` directive

### Infrastructure
- Set up basic project structure
- Implemented lexer, parser, and compiler
- Added command-line interface
- Added interactive mode
- Implemented file processing

# CQL - Phase 1 Refactoring Summary

## Overview of Changes

This phase focused on restructuring the codebase by:
1. Separating declarations and implementations
2. Organizing code into modular components
3. Improving error handling
4. Updating the build system
5. Adding better documentation

## Modified Files

- `headers/project_utils.hpp` - Updated Logger implementation to use inline static members
- `CMakeLists.txt` - Completely revised for modular structure and better build process
- `README.md` - Updated installation instructions and requirements

## Added Files

### Header Files
- `include/cql/compiler.hpp` - Compiler class interface
- `include/cql/cql.hpp` - Main library interface
- `include/cql/lexer.hpp` - Lexical analyzer interface
- `include/cql/nodes.hpp` - AST node definitions
- `include/cql/parser.hpp` - Parser interface
- `include/cql/visitor.hpp` - Visitor pattern interface

### Implementation Files
- `src/cql/cli.cpp` - Interactive CLI implementation
- `src/cql/compiler.cpp` - Compiler implementation
- `src/cql/lexer.cpp` - Lexical analyzer implementation
- `src/cql/main.cpp` - Main program entry point
- `src/cql/nodes.cpp` - AST node implementations
- `src/cql/parser.cpp` - Parser implementation
- `src/cql/project_utils.cpp` - Project utilities implementation
- `src/cql/tests.cpp` - Test suite implementation
- `src/cql/util.cpp` - Utility functions implementation

### Project Infrastructure
- `LICENSE` - MIT license file