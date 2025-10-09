# CLAUDE.md - AI Assistant Context for CQL (Claude Query Language)

This document provides comprehensive context for AI assistants working on the CQL project. It contains all essential information needed to understand the project structure, standards, and current state.

## Current Work Status

**Active Branch**: `main` (all phases complete and merged)
**Latest PR**: [#49 - Fix failing tests after Phase 3 merge](https://github.com/dbjwhs/cql/pull/49) - MERGED ✅

### All Core Logging Phases Complete! 🎉

**Recently Merged:**
- ✅ **PR #49**: Fixed all failing tests after Phase 3 merge
  - Fixed file rotation race conditions (flush before close, remove before rename)
  - Fixed test cleanup exceptions (error_code overload of filesystem operations)
  - Fixed HybridCompiler API key configuration (pass global config to PromptCompiler)
  - Enhanced HTTP client tests with intelligent httpbin.org failure detection
  - Replaced std::cout with Logger system for Phase 2 compliance
  - All 239 tests passing

**What's Next:**
- Phase 5: Multi-logger with independent level control
- Phase 6: Documentation and examples
- Multi-provider AI support expansion

### Development Roadmap

**Logging System Enhancement (Multi-Phase)**
- ✅ Phase 1: File-only logging by default with `--log-console` option (PR #44 - MERGED)
- ✅ Phase 2: Separate user output from debug logging (PR #45 - MERGED)
- ✅ Phase 3: Enhanced file logger configuration (rotation, timestamps) (PR #48 - MERGED)
- ✅ Phase 4: Clean up mixed output patterns (PR #46 - MERGED)
- 📋 Phase 5: Multi-logger with independent level control
- 📋 Phase 6: Documentation and examples

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

### API Key Setup
For live API integration and meta-prompt compilation:

1. **Create `.env` file in project root:**
   ```bash
   cp .env.example .env
   ```

2. **Add your Anthropic API key:**
   ```bash
   # .env file
   CQL_API_KEY=your-anthropic-api-key-here
   ANTHROPIC_API_KEY=your-anthropic-api-key-here
   ```

   **API Key Requirements:**
   - Must be at least 30 characters long (validated by `AnthropicProvider::is_configured()`)
   - Both `CQL_API_KEY` and `ANTHROPIC_API_KEY` can be used (for compatibility)
   - Keys starting with `sk-ant-api03-` are standard Anthropic API keys
   - The `.env` file is automatically loaded by the test suite and application

3. **Test API connectivity:**
   ```bash
   # Test API configuration and connectivity
   ./build/cql_test --gtest_filter="LiveAnthropicIntegrationTest.BasicConnectivityTest"

   # Test that LLM components are properly configured
   ./build/cql_test --gtest_filter="HybridCompilerTest.LLMAvailability"
   ```

**Important Notes:**
- The `.env` file is git-ignored for security - never commit API keys to version control
- Test API keys in unit tests must be at least 30 characters to pass validation
- The application loads `.env` from the project root directory automatically
- Without a valid API key, LLM-based features will be disabled (LOCAL_ONLY mode still works)

### Logging Configuration

CQL provides flexible logging configuration with secure file-based logging by default:

#### Default Behavior (File Logging Only)
- **Default**: Logs are written to `cql.log` in the current directory
- **No console clutter**: Console output remains clean for user-facing content
- **Security**: Log file paths are validated using `InputValidator::resolve_path_securely()` to prevent directory traversal attacks

```bash
# Default behavior - logs to cql.log only
./build/cql input.llm output.txt
```

#### Command-Line Options

**`--log-file PATH`** - Specify custom log file path
```bash
# Custom log file location
./build/cql input.llm --log-file /var/log/cql/app.log

# Relative path (validated for security)
./build/cql input.llm --log-file ./logs/debug.log
```

**`--log-console`** - Enable console logging in addition to file logging
```bash
# Log to both file and console
./build/cql input.llm --log-console

# Combine with custom log file
./build/cql input.llm --log-console --log-file debug.log
```

**`--debug-level LEVEL`** - Set minimum log level (INFO|NORMAL|DEBUG|ERROR|CRITICAL)
```bash
# Debug level logging to file
./build/cql input.llm --debug-level DEBUG

# Debug level logging to both file and console
./build/cql input.llm --log-console --debug-level DEBUG
```

**`--log-max-size BYTES`** - Enable log file rotation at specified size
```bash
# Rotate log file when it reaches 10MB (10 * 1024 * 1024 bytes)
./build/cql input.llm --log-max-size 10485760

# Rotate at 1MB
./build/cql input.llm --log-max-size 1048576
```

**`--log-max-files COUNT`** - Maximum number of rotated log files to keep (default: 5)
```bash
# Keep up to 10 rotated log files
./build/cql input.llm --log-max-size 10485760 --log-max-files 10

# Keep unlimited rotated files (use with caution)
./build/cql input.llm --log-max-size 10485760 --log-max-files 0
```

**`--log-timestamp FORMAT`** - Set timestamp format for log messages
```bash
# ISO 8601 UTC format: 2025-10-07T14:30:45.123Z
./build/cql input.llm --log-timestamp iso8601

# ISO 8601 local time with timezone: 2025-10-07T14:30:45.123-0700
./build/cql input.llm --log-timestamp iso8601-local

# Simple format: 2025-10-07 14:30:45.123 (default)
./build/cql input.llm --log-timestamp simple

# Epoch milliseconds: 1696695045123
./build/cql input.llm --log-timestamp epoch

# No timestamp
./build/cql input.llm --log-timestamp none
```

#### Logging Examples

```bash
# Production use - quiet console, detailed file logging
./build/cql production.llm --debug-level INFO

# Development - see logs in real-time
./build/cql development.llm --log-console --debug-level DEBUG

# Debugging specific issue - custom log location
./build/cql problem.llm --log-console --debug-level DEBUG --log-file /tmp/debug.log

# CI/CD pipeline - errors only to specific file
./build/cql pipeline.llm --debug-level ERROR --log-file build.log

# Production with log rotation - rotate at 50MB, keep 10 files
./build/cql production.llm --log-max-size 52428800 --log-max-files 10 --log-timestamp iso8601

# Long-running service - rotation with ISO timestamps
./build/cql service.llm --log-max-size 10485760 --log-max-files 20 --log-timestamp iso8601-local
```

#### Security Considerations

- **Path Validation**: All log file paths are validated to prevent directory traversal attacks
- **Secure Resolution**: Uses `InputValidator::resolve_path_securely()` for path sanitization
- **No Sensitive Data**: Avoid logging API keys, passwords, or other credentials
- **Log Rotation**: Consider implementing log rotation for long-running processes

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

### Completed Work (2025)
- **✅ Test Reliability Fixes (Post Phase 3)**: Comprehensive test suite hardening
  - **PR**: #49 - MERGED - All failing tests fixed after Phase 3 merge
  - **Fixes**: File rotation race conditions, test cleanup exceptions, API key configuration
  - **Enhancements**: Intelligent httpbin.org failure detection with visual warnings
  - **Compliance**: Replaced std::cout with Logger system for Phase 2 standards
  - **Testing**: All 239 tests passing consistently

- **✅ Enhanced File Logger (Phase 3)**: Advanced logging configuration features
  - **PR**: #48 - MERGED - Log rotation, timestamp formats, configurable retention
  - **Features**: `--log-max-size`, `--log-max-files`, `--log-timestamp` options
  - **Formats**: ISO 8601 (UTC/local), simple, epoch, none
  - **Rotation**: Automatic rotation with configurable file retention
  - **Testing**: Comprehensive rotation and timestamp format tests

- **✅ Mixed Output Cleanup (Phase 4)**: Unified output handling across codebase
  - **PR**: #46 - MERGED - Updated all CLI files to use UserOutputManager
  - **Coverage**: cli.cpp, meta_prompt_handler.cpp, template_operations.cpp, documentation_handler.cpp
  - **Compliance**: Documented intentional std::cout usage for interactive prompts
  - **Quality**: Removed unnecessary std::to_string() calls, verified newline consistency

- **✅ User Output Separation (Phase 2)**: Complete separation of user-facing output from debug logging
  - **PR**: #45 - MERGED (commit `33c717a`) - All review feedback addressed and implemented
  - **Features**: UserOutput interface with 5 implementations, colored console output, MessageType enum
  - **Testing**: 20 comprehensive unit tests, all passing
  - **Security**: Path validation for file output, thread-safe manager implementation
  - **Platform**: Windows compatibility with platform-specific headers

- **✅ File Logging by Default (Phase 1)**: Comprehensive logging configuration system
  - **PR**: #44 - MERGED (commit `4486e3f`) - All review feedback addressed and implemented
  - **Features**: File-only logging by default, `--log-console` flag, `--log-file PATH` custom paths
  - **Testing**: 22 unit tests + integration tests, all passing
  - **Security**: Path validation using `InputValidator::resolve_path_securely()`

- **✅ HTTP Client CI Reliability**: Comprehensive test architecture implemented and merged
  - **PR**: #39 - MERGED (commit `d423211`) - All review feedback addressed and implemented
  - **Status**: COMPLETED - All tests passing, environment controls working
  - **Implementation**: Split conditional tests, environment variable controls, MockHttpClientTest class

### Current Project State
- **Main Branch**: `main` - Stable with all 4 logging phases complete
- **Build Status**: ✅ All 239 tests passing consistently
- **CI Integration**: ✅ Environment variable controls implemented and tested (`CQL_SKIP_EXTERNAL_TESTS=1`)
- **Documentation**: ✅ Updated with latest changes and patterns
- **Logging System**: ✅ Complete - File logging, user output separation, rotation, timestamps all implemented

### Next Steps
- **Phase 5**: Multi-logger with independent level control
- **Phase 6**: Comprehensive logging documentation and examples
- **Multi-Provider Support**: Expanding beyond Anthropic to OpenAI, Google Gemini
- **Performance Optimization**: Profiling and optimization initiatives
- **Enhanced CLI**: Additional command-line interface features

### Context Notes
- **Developer Experience**: Advanced - assume knowledge of modern C++ and security practices
- **Build System**: CMake with comprehensive dependency management
- **Dependencies**: Minimal external dependencies, prefer security-hardened solutions
- **Performance**: Security-conscious optimization with comprehensive testing
- **Quality**: Enterprise-grade standards with security-first approach

This project represents a security-hardened approach to query language development with comprehensive tooling, testing, and enterprise-grade security measures.

---
**Last Updated**: 2025-10-08 - All 4 core logging phases complete (PRs #44, #45, #46, #48, #49 merged), ready for Phase 5
