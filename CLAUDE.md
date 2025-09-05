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
- **AILib Integration**: Complete C++ AI provider library (Phase 1 COMPLETED)
- **HTTP Client CI Reliability**: COMPLETED - Comprehensive test architecture with environment controls
- **Multi-Provider Support**: Expanding beyond Anthropic to OpenAI, Google Gemini
- **Command-line interface enhancements** 
- **Response parsing and file organization**
- **Enterprise-grade security hardening**
- **Performance optimization and memory safety**

## AILib: C++ AI Provider Library

**AILib is now fully integrated** as a modern C++20 library providing unified interfaces to AI providers.

### Current Status (Phase 1: Within CQL) ✅ COMPLETED
```
lib/ailib/
├── include/ailib/
│   ├── core/           # Provider interfaces, configuration
│   ├── providers/      # Anthropic implementation, factory
│   ├── http/           # CURL-based HTTP client with retry logic
│   ├── auth/           # SecureString for API key protection
│   └── detail/         # JSON utilities, internal implementation
├── src/                # All implementation files
└── tests/              # Comprehensive test suite
```

### Key Features Implemented
- **Provider Interface**: Unified API across all AI providers
- **Anthropic Integration**: Complete Claude API support with streaming
- **HTTP Client**: CURL-based with exponential backoff retry logic
- **Secure Configuration**: `SecureString` class for API key protection
- **Factory Pattern**: Dynamic provider creation and management
- **Comprehensive Testing**: Unit and integration tests for all components

### Usage Example
```cpp
#include "ailib/providers/factory.hpp"
#include "ailib/core/config.hpp"

// Configure and create provider
cql::Config config;
config.set_api_key("anthropic", "your-key");
config.set_model("anthropic", "claude-3-sonnet-20240229");

auto& factory = cql::ProviderFactory::get_instance();
auto provider = factory.create_provider("anthropic", config);

// Make request with retry logic
cql::ProviderRequest request;
request.prompt = "Hello world";
request.max_tokens = 100;
request.retry_policy.max_retries = 3;
request.retry_policy.initial_delay = std::chrono::milliseconds(100);

auto response = provider->send_request(request);
```

### Build Integration
- **Object Library**: AILib compiles as part of CQL (build/CMakeFiles/cql_lib.dir/lib/ailib/)
- **Include Paths**: Fully integrated with `#include "ailib/..."` syntax
- **Testing**: AILib tests run as part of main test suite
- **Security**: All code follows enterprise security standards

### HTTP Client CI Reliability (Recently Completed)

**Problem Solved**: Jenkins CI tests were failing due to external service dependencies (httpbin.org returning 503 errors instead of expected status codes).

**Solution Implemented** (PR #39): Comprehensive test architecture improvements:

#### Test Architecture Changes
- **Split conditional tests**: Converted 3 monolithic conditional tests into 8+ focused methods:
  - `NoRetryOnClientError_Normal` / `NoRetryOnClientError_CIFallback`
  - `RetryOnRateLimitError_Normal` / `RetryOnServerError_Fallback`  
  - `ConfigWithCustomSettings_Normal` / `ConfigWithCustomSettings_Offline`

#### Environment Controls for CI
- **Environment variable support**: `CQL_SKIP_EXTERNAL_TESTS=1` 
- **Proper test skipping**: Uses `GTEST_SKIP()` for clean CI integration
- **Behavioral validation**: Focus on HTTP status codes and retry logic, not timing

#### Test Doubles and Mocks
- **MockHttpClientTest class**: 5 new offline tests using invalid domains
- **Predictable failures**: `https://invalid-domain.fake` for consistent test results
- **Network error simulation**: Tests retry behavior without external dependencies

#### Test Utilities
- **test_utils namespace**: Common utilities for retry testing
- **RetryTestResult struct**: Standardized result validation
- **simulate_retry_scenario()**: Helper for retry behavior testing

#### Key Files Modified
- **`/Users/dbjones/ng/dbjwhs/cql/lib/ailib/tests/test_http_client.cpp`**: Complete test restructuring
- **Test Results**: All 28 HTTP client tests now passing consistently
- **CI Integration**: Reliable execution with `CQL_SKIP_EXTERNAL_TESTS=1` in CI environments

## Testing & Development Workflow

### Testing Requirements
- **Unit tests** for all public APIs (GoogleTest framework)
- **Integration tests** for component interactions  
- **Security tests** for input validation and path security (`./cql_test --gtest_filter="SecurityTest.*"`)
- **HTTP Client tests** with CI reliability features (`./cql_test --gtest_filter="*HttpClient*"`)
- **Environment-controlled testing** via `CQL_SKIP_EXTERNAL_TESTS=1` for CI systems
- **Performance tests** for critical paths
- **Memory safety** validation with comprehensive coverage
- **Resource cleanup tests** ensure proper temporary file management

### Development Workflow
```bash
# CRITICAL: Always build first, then test
mkdir -p build && cd build && cmake .. && make

# Run all tests after building
./cql_test

# Run specific test suite
./cql_test --gtest_filter="CQLTest.*"

# Run HTTP client tests with external service checks
./cql_test --gtest_filter="*HttpClient*"

# Run HTTP client tests without external dependencies (CI mode)
CQL_SKIP_EXTERNAL_TESTS=1 ./cql_test --gtest_filter="*HttpClient*"

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

CQL implements comprehensive enterprise-grade security measures. For detailed security information, see [docs/SECURITY.md](docs/SECURITY.md).

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

### Resource Cleanup
```cpp
// Automatic resource management with RAII
#include "cql/resource_cleanup.hpp"
ResourceCleanup cleanup;
cleanup.register_temp_file(temp_path);
// Automatic cleanup on destruction
```

### Network Timeout Configuration
```bash
# User-configurable timeouts
cql --optimize query.llm --timeout 60  # 60 second timeout
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

## Recent Development Status

### Completed Work (September 2025)
- **✅ HTTP Client CI Reliability**: Comprehensive test architecture implemented and merged
  - **PR**: #39 - MERGED (commit `d423211`) - All review feedback addressed and implemented  
  - **Status**: COMPLETED - All tests passing, environment controls working
  - **Implementation**: Split conditional tests, environment variable controls, MockHttpClientTest class
  - **Last Commit**: `95fb241` - "fix: Remove unused variable in NoRetryOnClientError_Normal test"

### Current Project State  
- **Main Branch**: `main` - Stable with AILib Phase 1 complete and HTTP Client CI reliability merged
- **Build Status**: ✅ All tests passing (28+ HTTP client tests + full test suite)
- **CI Integration**: ✅ Environment variable controls implemented and tested (`CQL_SKIP_EXTERNAL_TESTS=1`)
- **Documentation**: ✅ Updated with latest changes and patterns

### Next Steps  
- Continue with multi-provider support expansion (OpenAI, Google Gemini integration)
- Performance optimization initiatives
- Enhanced command-line interface features

### Context Notes
- **Developer Experience**: Advanced - assume knowledge of modern C++ and security practices
- **Build System**: CMake with comprehensive dependency management
- **Dependencies**: Minimal external dependencies, prefer security-hardened solutions
- **Performance**: Security-conscious optimization with comprehensive testing
- **Quality**: Enterprise-grade standards with security-first approach

This project represents a security-hardened approach to query language development with comprehensive tooling, testing, and enterprise-grade security measures.

---
**Last Updated**: September 2025 - HTTP Client CI Reliability completion and documentation accuracy verification
