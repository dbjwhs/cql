# Claude Query Language (CQL)

🚧 **WORK IN PROGRESS** 🚧

**IMPORTANT NOTICE**: This project is currently a proof of concept and is still in early development. Many features may
be incomplete, untested, or subject to significant changes. The current implementation serves primarily as a
demonstration of the concept and requires substantial refinement before it can be considered production-ready. Use at
your own risk and expect breaking changes in future updates.

The Claude Query Language (CQL) is a domain-specific language and compiler designed to formalize and standardize the
creation of technical queries for Large Language Models, specifically targeting Anthropic's Claude. The implementation
leverages multiple software design patterns including the Visitor pattern for traversing Abstract Syntax Trees (AST),
the Builder pattern for constructing queries, and the Interpreter pattern for processing the DSL. CQL follows classic
compiler architecture with distinct lexical analysis, parsing, and code generation phases, utilizing modern C++20
features such as `std::optional`, `std::string_view`, designated initializers, and the ranges library. The core
architecture implements RAII principles and employs smart pointers for memory management. Developed in 2025, CQL arose
from the need to standardize the creation of complex, structured prompts that consistently yield high-quality code
generation from LLMs.

CQL addresses several critical challenges in prompt engineering for AI code generation. First, it eliminates
inconsistency in query structure that often leads to variable results from LLMs by enforcing a standardized format with
required elements. Second, it dramatically reduces the time engineers spend crafting detailed prompts manually through
its concise syntax. Third, it ensures all necessary context, requirements, and test specifications are included,
preventing the common problem of incomplete or ambiguous prompts. Fourth, it enables reusability through template
storage and modification, allowing teams to maintain libraries of effective queries. Finally, it solves the
collaboration problem by providing a common, version-controllable language for prompt engineering that can be shared
across teams and integrated into development workflows.

## Features

### Core Features (Phase 1)
- **Simple @directive syntax** for specifying different aspects of the query
- **Customizable output formatting** for different LLM preferences
- **Copyright and license specification** for generated code
- **Context and requirement clarification** capabilities
- **Test case specification** to ensure proper coverage
- **Performance requirement** directives
- **Dependency specification** for frameworks and libraries
- **Interactive CLI mode** for iterative query development
- **File import/export** for saving and loading queries
- **Robust error handling** with detailed diagnostics

### Advanced Features (Phase 2)
- **Advanced directives** for architecture, constraints, examples, and more
- **Template variables** with string interpolation
- **Query validation** to enforce structure and best practices
- **Multiple output formats** including markdown and JSON
- **Model targeting** to specify appropriate Claude model
- **Security requirements** for code generation
- **Algorithmic complexity** specifications

See [PHASE2_FEATURES.md](PHASE2_FEATURES.md) for detailed information on the new features.

## Template Storage and Reusability

CQL provides powerful reusability through its template storage and modification capabilities, enabling teams to maintain libraries
of effective queries. When developers find a query structure that consistently produces excellent results from Claude, they can
save it as a template file using CQL's file I/O capabilities:

```bash
# In interactive mode
> save successful_thread_safe_pattern.cql
```

These template files store the complete structured query with all its directives (`@language`, `@description`, `@context`, etc.)
in the original DSL format, not the compiled output.

Later, when facing a similar task, any team member can:

1. Load the template:
   ```bash
   > load successful_thread_safe_pattern.cql
   ```

2. Make targeted modifications while preserving the effective structure:
   ```bash
   # Change just what's needed, keeping the valuable context and test structure
   > show
   @language "C++"
   @description "implement a thread-safe queue"
   @context "Using C++20 features and RAII principles"
   @test "Test concurrent operations"
   ...
   
   # Modify just the description
   @description "implement a thread-safe stack"
   ```

3. Compile and use the adapted query:
   ```bash
   > compile
   ```

This process creates a "knowledge repository" of effective query patterns that:
- Preserves institutional knowledge about what works well
- Reduces repeated effort in crafting similar queries
- Enables standardization across teams
- Improves consistency of LLM outputs
- Allows iterative refinement of query templates over time

Teams can organize libraries of templates by domain, complexity, or programming language, similar to how they might maintain
code snippet libraries or documentation templates.

## Installation

### Requirements

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 19.26+)
- CMake 3.30 or higher
- Git (for cloning the repository)

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

## Basic Usage

### Command-line Arguments

```bash
# Run the default mode (tests and examples)
./cql

# Run just the test suite
./cql --test

# Show example queries
./cql --examples

# Enter interactive mode
./cql --interactive

# Show copyright example
./cql --copyright

# Process a query file
./cql input.cql output.txt
```

### Query Syntax Examples

#### Basic Function Query

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

#### Class Implementation with Requirements

```
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

**Compiled Output:**

```
Please generate C++ code that:
implement a thread-safe queue class with a maximum size

Context:
- Using C++20 features and RAII principles
- Must be exception-safe

Dependencies:
- std::mutex, std::condition_variable

Performance Requirements:
- Support 100k operations per second

Please include tests for the following cases:
- Test concurrent push operations
- Test concurrent pop operations
- Test boundary conditions (empty/full)
- Test exception safety guarantees

Quality Assurance Requirements:
- All code must be well-documented with comments
- Follow modern C++ best practices
- Ensure proper error handling
- Optimize for readability and maintainability
```

#### With Copyright Header

```
@copyright "MIT License" "2025 dbjwhs"
@language "C++"
@description "implement a binary search tree"
@context "Modern C++ implementation"
@test "Insert elements"
@test "Delete elements"
@test "Find elements"
```

**Compiled Output:**

```
Please include the following copyright header at the top of all generated files:
```
// MIT License
// Copyright (c) 2025 dbjwhs
```

Please generate C++ code that:
implement a binary search tree

Context:
- Modern C++ implementation

Please include tests for the following cases:
- Insert elements
- Delete elements
- Find elements

Quality Assurance Requirements:
- All code must be well-documented with comments
- Follow modern C++ best practices
- Ensure proper error handling
- Optimize for readability and maintainability
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

## Interactive Mode Commands

```
help       - Show help
exit/quit  - Exit the program
clear      - Clear the current query
show       - Show the current query
compile    - Compile the current query
load FILE  - Load query from file
save FILE  - Save compiled query to file
```

## Related Resources and Books

Domain-Specific Languages and query compilers like CQL draw from established patterns in these resources:

- "Compilers: Principles, Techniques, and Tools" (Dragon Book) by Aho, Lam, Sethi, and Ullman
- "Language Implementation Patterns" by Terence Parr
- "Domain-Specific Languages" by Martin Fowler
- "Design Patterns: Elements of Reusable Object-Oriented Software" by Gamma, Helm, Johnson, and Vlissides
- "Modern Compiler Design" by Grune, van Reeuwijk, Bal, Jacobs, and Langendoen
- "The Definitive ANTLR 4 Reference" by Terence Parr

Similar patterns are found in query languages like SQL, GraphQL, and SPARQL, as well as in build systems like CMake and
code generation tools like SWIG.

## License

This code is provided under the MIT License. Feel free to use, modify, and distribute as needed.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
