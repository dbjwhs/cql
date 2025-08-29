# CLAUDE.md - AI Assistant Context for CQL (Claude Query Language)

This document provides comprehensive context for AI assistants working on the CQL project. It contains all essential information needed to understand the project structure, standards, and current state.

## Project Overview

**CQL (Claude Query Language)** is a modern C++20 compiler and development platform focused on building robust, high-performance query language processing with enterprise-grade security and tooling. The project emphasizes:

- **Modern C++20**: Advanced language features, zero-cost abstractions, RAII patterns
- **Enterprise Security**: Comprehensive input validation, secure memory management, path traversal protection
- **Developer Experience**: Extensive tooling, comprehensive testing, automated quality gates
- **Performance Focus**: Optimized parsing, efficient compilation, memory-safe operations

## Build Commands

### Quick Setup Commands
```bash
# Clone and initial setup
git clone <repository-url>
cd cql

# Standard build
mkdir -p build && cd build
cmake .. && make

# Build without warnings (development)
cmake .. 2>/dev/null && make 2>/dev/null

# Debug build with sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Verify installation
./cql_test
```

### Application Usage
- Run all tests: `build/cql_test`
- Run specific test: `build/cql_test --gtest_filter="TestName*"`
- Run examples: `build/cql --examples`
- Interactive mode: `build/cql --interactive`
- Process a file: `build/cql input.llm output.txt`
- Process with headers: `build/cql input.llm output.txt --include-header`
- Copy output to clipboard: `build/cql input.llm --clipboard` or `build/cql --clipboard input.llm`
- API integration: `build/cql --submit input.llm --output-dir ./output`

## Coding Standards & Requirements

### Language & Style Requirements
- **C++20 minimum** - Use modern features (concepts, ranges, modules, std::string_view, constexpr improvements)
- **Template naming**: Modern concept-constrained descriptive naming preferred:
  ```cpp
  template<std::copyable ElementType>  // Preferred modern style
  template<typename T>                 // Acceptable for simple cases
  ```
- **Error Handling**: Structured error handling with comprehensive logging
- **RAII**: Strict resource management, prefer stack allocation
- **Zero-cost abstractions**: Performance-critical code with minimal overhead

### Code Quality Standards
- **Testing Required**: Every piece of code needs comprehensive tests (85%+ coverage target)
- **Documentation**: All public APIs require Doxygen-style documentation with examples
- **Performance**: Include benchmarks for performance-critical components
- **Memory Safety**: No memory leaks, secure memory management for sensitive data
- **Build Quality**: Zero warnings, clean static analysis, automated quality gates
- **Security First**: Input validation, path traversal protection, secure string handling

### CRITICAL: Code Cleanliness Standards
**THESE PRACTICES ARE ABSOLUTELY PROHIBITED:**

- **❌ NEVER commit commented-out code** - Use git history for deleted code
- **❌ NO dead code or disabled functionality** - Remove unused code completely  
- **❌ NO "temporary" commented blocks** - They become permanent technical debt
- **❌ NO debugging artifacts left in commits** - Clean up before committing

**Why this matters:** Commented code creates maintenance burden, confusion about intent, and violates professional engineering standards. Git already preserves deleted code history.

**Correct approaches instead:**
- **Use appropriate log levels**: `Logger::getInstance().log(LogLevel::DEBUG, "message")` for development diagnostics
- **Conditional compilation**: `#ifdef CQL_TESTING` for test-only code
- **Complete removal**: Delete unused code entirely - git preserves the history
- **Feature flags**: For incomplete features, use runtime configuration

**This standard is NON-NEGOTIABLE** - violations will require immediate fixes.

### File & Naming Conventions
- **Headers**: `.hpp` extension (not `.h`)
- **Classes**: `CamelCase` (e.g., `TemplateValidator`)
- **Functions/Methods**: `snake_case` (e.g., `process_template`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `MAX_TEMPLATE_NAME_LENGTH`)
- **Template Parameters**: `CamelCase` (modern descriptive names)
- **Namespaces**: `lower_case` (e.g., `cql::test`)
- **Private member variables**: `m_` prefix (e.g., `m_template_name`)

### Security Requirements
- **Input Validation**: All inputs must be validated against defined length limits
- **Path Security**: Use secure path resolution to prevent directory traversal attacks
- **Memory Security**: Use SecureString for sensitive data (API keys, secrets)
- **JSON Handling**: Use unified JsonUtils for all JSON operations
- **Logging Security**: Sanitize all logged data to prevent information leakage

## Code Style Guidelines
- Modern C++20 features preferred (concepts, ranges, string_view, modules)
- Use RAII principles for resource management
- Prefer smart pointers to raw pointers
- Classes use CamelCase, methods/variables use snake_case
- Private member variables prefixed with `m_`
- Include `[[nodiscard]]` for functions that return values
- Use `std::string_view` for string parameters not requiring ownership
- Always implement the Visitor pattern for new node types
- Document all classes and methods with Doxygen-style comments
- Use `const` for methods/parameters that don't modify state
- Use Logger::getInstance().log(LogLevel::INFO, "message", var) for logging
- Handle exceptions appropriately with try/catch blocks
- Implement thorough unit tests for new features

## Current Development Focus
- API Integration with Anthropic's Claude API
- Code generation from Claude responses
- Command-line interface enhancements
- Response parsing and file organization
- Enterprise-grade security hardening
- Performance optimization and memory safety

## Testing & Development Workflow

### Testing Requirements
- **Unit tests** for all public APIs (GoogleTest framework)
- **Integration tests** for component interactions  
- **Security tests** for input validation and path security
- **Performance tests** for critical paths
- **Memory safety** validation with comprehensive coverage

### Development Workflow
```bash
# CRITICAL: Always build first, then test
mkdir -p build && cd build && cmake .. && make

# Run all tests after building
./cql_test

# Run specific test suite
./cql_test --gtest_filter="CQLTest.*"

# Before submitting changes, run the full test suite
./cql_test && echo "All tests passed - ready to commit"
```

**CRITICAL BUILD RULE**: Always build the project after making code changes before running tests. Running old test binaries will give false results.

### Quality Assurance Workflow
- Test files are located in `src/cql/tests.cpp`
- Use `TEST_ASSERT` macro for assertions with meaningful messages
- All new features must have comprehensive tests
- Security features require dedicated test coverage
- Performance-critical code needs benchmarking

## Key Security Features

### SecureString Implementation
```cpp
// Use SecureString for sensitive data
SecureString api_key("your-api-key-here");
// Memory is automatically locked and zeroed on destruction
```

### Input Validation
```cpp
// All inputs validated against defined constants
if (!InputValidator::validate_template_name(name)) {
    return ValidationResult::error("Invalid template name");
}
```

### Path Security
```cpp
// Secure path resolution prevents directory traversal
auto secure_path = InputValidator::resolve_path_securely(user_path);
if (!secure_path) {
    return ValidationResult::error("Path traversal attempt detected");
}
```

### Unified JSON Handling
```cpp
// Use JsonUtils for all JSON operations
auto request = JsonUtils::create_api_request(model, query, max_tokens, temperature);
auto result = JsonUtils::safe_parse(json_string);
```

## AI Assistant Guidelines

When working on this project:

1. **Security First**: All inputs must be validated, sensitive data secured, paths protected
2. **Build Before Test**: Always build the project before running tests to avoid false results
3. **Quality First**: All code must pass comprehensive testing and security validation
4. **Modern C++20**: Use advanced language features appropriately (concepts, ranges, modules)
5. **Testing Mandatory**: Every component needs comprehensive test coverage
6. **Documentation Required**: All public APIs need Doxygen documentation with examples
7. **Follow Patterns**: Study existing code for established security and architectural patterns

### CRITICAL: Problem-Solving Standards

**❌ NEVER take shortcuts when fixing issues:**
- **DO NOT disable or comment out failing code** - This is absolutely unacceptable
- **DO NOT bypass security validations** - Fix the underlying issues properly
- **DO NOT skip building before testing** - This leads to false test results

**✅ ALWAYS fix root causes:**
- **Build first, test second** - Ensure you're testing current code, not old binaries
- **Fix compilation errors properly** - Add missing includes, implement missing methods
- **Implement security measures** - Don't bypass validation, strengthen it
- **Test comprehensively** - Security, functionality, and performance

**Why this matters:** Taking shortcuts creates security vulnerabilities and technical debt. This project maintains enterprise-grade standards where systematic fixes are the only acceptable approach.

### Context Notes
- **Developer Experience**: Advanced - assume knowledge of modern C++ and security practices
- **Build System**: CMake with comprehensive dependency management
- **Dependencies**: Minimal external dependencies, prefer security-hardened solutions
- **Performance**: Security-conscious optimization with comprehensive testing
- **Quality**: Enterprise-grade standards with security-first approach

This project represents a security-hardened approach to query language development with comprehensive tooling, testing, and enterprise-grade security measures.
