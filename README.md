# Claude Query Language (CQL)

The Claude Query Language (CQL) is a domain-specific language and compiler designed to formalize and standardize the creation of technical queries for Large Language Models, specifically targeting Anthropic's Claude. CQL enables engineers to create structured, consistent prompts that consistently yield high-quality code generation from Claude.

## Overview

CQL addresses several critical challenges in prompt engineering for AI code generation:

1. **Standardization**: Eliminates inconsistency in query structure through a standardized format with required elements
2. **Efficiency**: Dramatically reduces the time engineers spend crafting detailed prompts
3. **Completeness**: Ensures all necessary context, requirements, and test specifications are included
4. **Reusability**: Enables template storage and modification through a comprehensive template system
5. **Collaboration**: Provides a common, version-controllable language for prompt engineering

## Features

### Core Language Features
- **Simple @directive syntax** for specifying different aspects of the query
- **Customizable output formatting** for different LLM preferences
- **Copyright and license specification** for generated code
- **Context and requirement clarification** capabilities
- **Test case specification** to ensure proper coverage
- **Performance requirement** directives
- **Dependency specification** for frameworks and libraries

### Template System
- **Template storage and management** for reusing effective query patterns
- **Variable interpolation** with `${var}` syntax
- **Template inheritance** for building on existing patterns
- **Template validation** to ensure quality and correctness
- **Documentation generation** for templates

### Claude API Integration
- **Direct query submission** to Claude API
- **Response processing** with multi-file code generation
- **Streaming responses** for long-running generations
- **Asynchronous API support** with C++20 features
- **Retries with exponential backoff** for transient errors
- **Comprehensive error handling** and categorization

## Installation

### Requirements

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 19.26+)
- CMake 3.30 or higher
- Git (for cloning the repository)
- CURL library for API integration

### Building from Source

```bash
# Clone the repository
git clone https://github.com/dbjwhs/cql.git
cd cql

# Create a build directory and enter it
mkdir build && cd build

# Configure and build the project
cmake ..
make

# Run the executable
./cql --help
```

## Command-Line Usage

### Basic Commands

```bash
# Show help information
./cql --help

# Run the test suite
./cql --test

# Show example queries
./cql --examples

# Enter interactive mode
./cql --interactive

# Process a query file
./cql input.cql output.txt
```

### API Integration Commands

```bash
# Submit a query to Claude API
./cql --submit input.cql

# Submit with specific model selection
./cql --submit input.cql --model claude-3-opus

# Submit and save generated files to a directory
./cql --submit input.cql --output-dir ./output
```

### Template Commands

```bash
# List all available templates
./cql --templates

# Use a template with variable substitutions
./cql --template thread_safe_queue collection_type=stack

# Validate a template
./cql --validate thread_safe_queue

# Validate all templates
./cql --validate-all

# Generate documentation for a template
./cql --docs thread_safe_queue

# Generate documentation for all templates
./cql --docs-all

# Export template documentation
./cql --export docs/templates.md
```

## Query Syntax Examples

### Basic Function Query

```
@language "C++"
@description "implement a string reverse function"
@context "Using string_view for efficiency"
@test "Empty string"
@test "Single character"
@test "Multiple characters"
```

**Compiled Output:**

```
Please generate C++ code that:
implement a string reverse function

Context:
- Using string_view for efficiency

Please include tests for the following cases:
- Empty string
- Single character
- Multiple characters

Quality Assurance Requirements:
- All code must be well-documented with comments
- Follow modern C++ best practices
- Ensure proper error handling
- Optimize for readability and maintainability
```

### Advanced Query with Requirements

```
@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "implement a thread-safe queue class with a maximum size"
@context "Using C++20 features and RAII principles"
@context "Must be exception-safe"
@dependency "std::mutex, std::condition_variable"
@performance "Support 100k operations per second"
@test "Test concurrent push operations"
@test "Test concurrent pop operations"
@test "Test boundary conditions (empty/full)"
@test "Test exception safety guarantees"
```

## Interactive Mode

CQL provides an interactive CLI mode for iterative query development:

```bash
./cql --interactive
```

Available commands in interactive mode:

```
help         - Show help
exit/quit    - Exit the program
clear        - Clear the current query
show         - Show the current query
compile      - Compile the current query
load FILE    - Load query from file
save FILE    - Save compiled query to file
submit       - Submit compiled query to Claude API
async_submit - Submit query asynchronously to Claude API
stream       - Submit query with streaming response
model NAME   - Set Claude model to use (e.g., claude-3-opus)
output_dir   - Set directory for generated files
templates    - List all templates
template     - Template management commands
```

## Best Practices

### DO:
- Be specific and detailed in your `@description`
- Provide context about the environment and constraints
- Specify test cases for all edge conditions
- Include performance requirements for critical code
- Add copyright information for production code

### DON'T:
- Write overly vague descriptions
- Skip test specifications
- Omit important dependencies
- Mix multiple unrelated requirements in a single query
- Include information irrelevant to the coding task

## Documentation

CQL offers comprehensive documentation:

- [API Documentation](docs/api_documentation.md): Integration with Claude API
- [Template Documentation](docs/template_documentation.md): Using the template system
- [Quick Reference](docs/quick_reference.md): Concise syntax guide
- [Tutorial](docs/tutorial.md): Step-by-step introduction to CQL
- [Troubleshooting](docs/troubleshooting.md): Common issues and solutions

For API reference, you can generate Doxygen documentation with:

```bash
# In the build directory
make docs
```

## License

This code is provided under the MIT License. Feel free to use, modify, and distribute as needed.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.