# Compiler Frontend Library

A modern C++20 compiler frontend implementation with lexical analysis, parsing, and AST generation capabilities.

## Features

- **Lexical Analysis**: Regex-based tokenization with error recovery
- **Parsing**: Recursive descent parser with operator precedence
- **AST Generation**: Type-safe Abstract Syntax Tree with visitor pattern support  
- **Symbol Table**: Scoped hash-based symbol storage
- **Error Handling**: Rich diagnostics with source location information
- **Unicode Support**: Proper handling of Unicode identifiers
- **Performance**: Optimized for parsing 100k+ lines per second

## Architecture

The compiler frontend follows a traditional pipeline architecture:

```
Source Code → Lexer → Parser → AST → Semantic Analysis
```

### Core Components

- **Lexer**: Converts source code into tokens using regex patterns
- **Parser**: Builds AST using recursive descent parsing with precedence climbing
- **AST**: Hierarchical representation with visitor pattern support
- **Symbol Table**: Manages scoped symbol declarations and lookups
- **Error System**: Provides detailed error messages with source locations

## Language Support

The frontend supports a simple programming language with:

- **Data Types**: `int`, `float`, `string`, `bool`
- **Functions**: First-class functions with typed parameters
- **Control Flow**: `if/else`, `while` loops
- **Expressions**: Binary/unary operators with proper precedence
- **Variables**: Typed variable declarations with optional initialization

### Example Language Syntax

```cpp
function factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

function main() -> int {
    var result: int = factorial(5);
    return result;
}
```

## Usage

### Basic Example

```cpp
#include "compiler_frontend.h"
using namespace compiler_frontend;

// Create compiler frontend
CompilerFrontend compiler;

// Tokenize source code
auto tokens = compiler.tokenize(source_code);

// Parse into AST
auto ast = compiler.parse(tokens);

// Check for errors
auto errors = compiler.getErrors();
if (errors.empty()) {
    // Process AST using visitor pattern
    PrettyPrintVisitor printer;
    ast->accept(printer);
} else {
    // Handle compilation errors
    for (const auto& error : errors) {
        std::cout << error.format() << std::endl;
    }
}
```

### Advanced Usage with Custom Visitors

```cpp
// Custom visitor for AST traversal
class MyVisitor : public ASTVisitor {
public:
    void visit_function_decl(FunctionDecl& node) override {
        std::cout << "Found function: " << node.name() << std::endl;
        // Process function...
    }
    
    void visit_variable_decl(VariableDecl& node) override {
        std::cout << "Found variable: " << node.name() << std::endl;
        // Process variable...
    }
    
    // Implement other visit methods...
};

// Use custom visitor
MyVisitor visitor;
ast->accept(visitor);
```

## Building

### Requirements

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- Standard library with `<regex>`, `<format>`, and `<ranges>` support

### Compilation

```bash
# Build everything
make all

# Run tests
make test

# Run examples
make example

# Clean build
make clean

# Debug build
make debug

# Release build
make release
```

### Manual Compilation

```bash
# Compile the library
g++ -std=c++20 -O2 -c compiler_frontend.cpp

# Build and run tests
g++ -std=c++20 -O2 test_compiler_frontend.cpp compiler_frontend.o -o test
./test

# Build and run examples
g++ -std=c++20 -O2 example_usage.cpp compiler_frontend.o -o example
./example
```

## Testing

The library includes comprehensive tests covering:

- **Tokenization**: All language constructs and edge cases
- **Parsing**: Operator precedence and complex expressions
- **Error Recovery**: Graceful handling of syntax errors
- **Visitor Pattern**: AST traversal and transformation
- **Symbol Table**: Scoped symbol management
- **Integration**: End-to-end compilation pipeline

Run tests with:
```bash
make test
```

## Performance

The frontend is optimized for high-performance compilation:

- **Lexing**: O(n) linear scanning with regex optimization
- **Parsing**: O(n log n) with operator precedence climbing
- **Memory**: RAII-based resource management with smart pointers
- **Throughput**: 100k+ lines per second on modern hardware

## Error Handling

The frontend provides rich error diagnostics:

```cpp
// Example error output
ERROR: Expected ';' after expression at input.lang:5:23
WARNING: Unused variable 'x' at input.lang:10:9
```

Error levels:
- `INFO`: Informational messages
- `WARNING`: Non-fatal issues
- `ERROR`: Compilation errors
- `FATAL`: Unrecoverable errors

## Extension Points

The design supports easy extension:

### Adding New AST Node Types

1. Create new node class inheriting from appropriate base
2. Add visitor method to `ASTVisitor` interface
3. Implement visitor methods in all concrete visitors
4. Update parser to generate the new node type

### Adding New Token Types

1. Add token type to `TokenType` enum
2. Add regex pattern to lexer initialization
3. Update parser to handle new token

### Custom Semantic Analysis

```cpp
class MySemanticAnalyzer : public ASTVisitor {
    // Implement semantic checks
    void visit_binary_expr(BinaryExpr& node) override {
        // Type checking, etc.
    }
};
```

## License

MIT License - Copyright (c) 2025 dbjwhs

## Contributing

This is a demonstration implementation. For production use, consider:

- More comprehensive error recovery
- Advanced optimization passes
- Code generation backends  
- IDE integration support
- Incremental compilation
- Parallel parsing for large files