# CQL (Claude Query Language) 

A modern C++20 compiler and development platform for AI-powered code generation with enterprise-grade security and tooling.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](#)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 🚀 Quick Start

```bash
# Clone and build
git clone <repository-url>
cd cql
mkdir -p build && cd build
cmake .. && make

# Run tests
./cql_test

# Try the compiler
./cql --version
```

## ✨ Features

### 🎯 Core CQL Compiler
- **Modern C++20**: Advanced language features, zero-cost abstractions
- **Template System**: Powerful code generation with variables, inheritance, and validation
- **Interactive Mode**: Real-time query development and testing
- **Documentation Generation**: Automatic template documentation in multiple formats
- **Enterprise Security**: Input validation, secure memory management, path protection
- **Advanced Logging**: Pluggable logging architecture with multiple adapters
- **Comprehensive Testing**: 85%+ test coverage with automated quality gates

### 🤖 AILib Integration
- **Multi-Provider AI Support**: Anthropic Claude, with extensible provider architecture
- **Modern C++ API**: Type-safe, async-first design with RAII principles
- **HTTP Client**: CURL-based implementation with retry logic and exponential backoff
- **Secure Configuration**: Protected API key storage with `SecureString` class
- **Response Processing**: Intelligent code generation and file organization
- **Live Testing**: Comprehensive integration tests with real API endpoints

## 📁 Project Structure

```
cql/
├── src/cql/                 # Core CQL compiler implementation
├── include/cql/             # Public CQL headers
├── lib/ailib/               # AILib: C++ AI Provider Library
│   ├── include/ailib/       # AILib public API
│   │   ├── core/           # Provider interfaces, config
│   │   ├── providers/      # Anthropic, factory
│   │   ├── http/           # HTTP client abstraction
│   │   ├── auth/           # Secure credential management
│   │   └── detail/         # Implementation details
│   ├── src/                # AILib implementation files
│   └── tests/              # AILib-specific tests
├── scripts/                # Utility scripts and tools
├── docs/                   # Comprehensive documentation
├── examples/               # Sample CQL files and demos
└── build/                  # Build artifacts
```

## 🛠️ Usage

### CQL Compiler Commands

```bash
# Basic compilation
./cql input.llm output.txt

# Include headers in output
./cql input.llm output.txt --include-header

# Copy to clipboard
./cql input.llm --clipboard

# Interactive mode
./cql --interactive

# API integration
./cql --submit input.llm --output-dir ./output
./cql --submit input.llm --model claude-3-sonnet --overwrite

# Template operations
./cql --templates                    # List all templates
./cql --template MyTemplate input.llm
./cql --validate MyTemplate          # Validate template
./cql --validate-all ./templates     # Validate all templates

# Documentation generation
./cql --docs MyTemplate              # Generate template docs
./cql --docs-all                     # Generate all docs
./cql --export ./docs md             # Export docs as markdown

# Debug and configuration  
./cql --debug-level DEBUG input.llm       # Enable debug logging
./cql --debug-level NORMAL input.llm      # Default level (clean output)
./cql --version
```

### AILib C++ API

```cpp
#include <ailib/ailib.hpp>

// Configure provider
cql::Config config;
config.set_api_key("anthropic", "your-api-key");
config.set_model("anthropic", "claude-3-sonnet-20240229");

// Create provider
auto factory = cql::ProviderFactory::get_instance();
auto provider = factory.create_provider("anthropic", config);

// Make request
cql::ProviderRequest request;
request.prompt = "Hello, world!";
request.max_tokens = 100;

auto response = provider->send_request(request);
if (response.is_success()) {
    std::cout << response.content << std::endl;
}
```

### Advanced Logging System

```cpp
#include <cql/logger_interface.hpp>
#include <cql/logger_adapters.hpp>

// Use built-in console logger
auto console_logger = std::make_unique<cql::DefaultConsoleLogger>();
console_logger->set_min_level(cql::LogLevel::INFO);

// File logging
auto file_logger = std::make_unique<cql::adapters::FileLogger>("app.log");

// Multi-output logging
auto multi_logger = std::make_unique<cql::adapters::MultiLogger>();
multi_logger->add_logger(std::move(console_logger));
multi_logger->add_logger(std::move(file_logger));

// Async logging for performance
auto async_logger = std::make_unique<cql::adapters::AsyncLogger>(std::move(multi_logger));

// Initialize logger system
cql::LoggerManager::initialize(std::move(async_logger));
```

## 📖 Documentation

- **[Complete Documentation](docs/README.md)** - Comprehensive guides and references
- **[AILib Design Specification](docs/CQL_AILIB_DESIGN_SPECIFICATION.md)** - Technical architecture
- **[Meta-Prompt Compiler Specification](docs/TECHNICAL_SPECIFICATION_META_PROMPT_COMPILER.md)** - Advanced compilation concepts
- **[Tutorial](docs/tutorial.md)** - Step-by-step getting started guide  
- **[API Documentation](docs/api_documentation.md)** - Complete API reference
- **[AILib Integration Guide](docs/AILIB_INTEGRATION_GUIDE.md)** - Integration documentation
- **[Interactive Mode Guide](docs/interactive_mode_guide.md)** - Interactive development workflow

## 🏗️ Development

### Prerequisites

- **C++20 compatible compiler** (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake 3.16+**
- **CURL development libraries**
- **Git** (with hooks support)

### Building from Source

```bash
# Debug build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run all tests
./cql_test

# Run specific tests
./cql_test --gtest_filter="CQLTest.*"
```

### Code Quality

- **Automated Testing**: Comprehensive unit and integration tests
- **Pre-commit Hooks**: Automated code formatting and validation
- **Static Analysis**: Built-in linting and security checks
- **Documentation**: All public APIs require Doxygen documentation

## 🔐 Security

CQL prioritizes enterprise-grade security:

- **Input Validation**: All inputs validated against defined limits and patterns
- **Secure Memory Management**: `SecureString` class with memory locking for API keys
- **Path Protection**: Secure path resolution preventing directory traversal attacks
- **Environment Variable Security**: Secure environment variable handling with precedence
- **Error Context Preservation**: Detailed error tracking without sensitive data leakage
- **Audit Logging**: Comprehensive security event logging with configurable levels
- **Thread-Safe Operations**: Lock-free data structures and secure concurrent access

## 🚦 Roadmap

### Current Status (Phase 1) ✅
- [x] Core CQL compiler with advanced template system
- [x] AILib provider interface and Anthropic Claude integration
- [x] HTTP client with retry logic and exponential backoff
- [x] Secure configuration management with environment variable support
- [x] Advanced logging system with pluggable adapters
- [x] Interactive development mode with real-time feedback
- [x] Template validation and documentation generation
- [x] Comprehensive testing infrastructure with live API integration
- [x] Response processing and intelligent code organization
- [x] CLI with extensive options and clipboard integration

### Phase 2: Multi-Provider Support
- [ ] OpenAI GPT integration
- [ ] Google Gemini support
- [ ] Provider plugin architecture

### Phase 3: Advanced Features
- [ ] Streaming response support
- [ ] Function calling capabilities
- [ ] Advanced authentication methods
- [ ] Meta-prompt compilation using LLM as compiler backend
- [ ] Multi-model orchestration and comparison
- [ ] Advanced caching and optimization strategies

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](docs/CONTRIBUTING.md) for details.

### Key Areas for Contribution

- **New AI Provider Support**: Implement additional provider integrations
- **Documentation**: Improve guides, examples, and API documentation
- **Testing**: Expand test coverage and add integration tests
- **Performance**: Optimize critical paths and memory usage

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Claude by Anthropic** - Primary AI provider integration
- **CURL** - HTTP client implementation
- **GoogleTest** - Testing framework
- **nlohmann/json** - JSON processing library

---

**Built with ❤️ using modern C++20 and enterprise-grade engineering practices.**
