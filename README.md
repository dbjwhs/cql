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
- **Template System**: Powerful code generation with variables and inheritance
- **Enterprise Security**: Input validation, secure memory management
- **Comprehensive Testing**: 85%+ test coverage with automated quality gates

### 🤖 AILib Integration
- **Multi-Provider AI Support**: Anthropic Claude, with extensible provider architecture
- **Modern C++ API**: Type-safe, async-first design with RAII principles
- **HTTP Client**: CURL-based implementation with retry logic and exponential backoff
- **Secure Configuration**: Protected API key storage and management

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

# API integration
./cql --submit input.llm --output-dir ./output

# Interactive mode
./cql --interactive
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

## 📖 Documentation

- **[Complete Documentation](docs/README.md)** - Comprehensive guides and references
- **[AILib Design Specification](docs/CQL_AILIB_DESIGN_SPECIFICATION.md)** - Technical architecture
- **[API Reference](https://dbjwhs.github.io/cql/)** - Doxygen-generated documentation
- **[Tutorial](docs/tutorial.md)** - Step-by-step getting started guide

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

- **Input Validation**: All inputs validated against defined limits
- **Secure Memory**: `SecureString` class for sensitive data (API keys)
- **Path Protection**: Secure path resolution preventing directory traversal
- **Audit Logging**: Comprehensive security event logging

## 🚦 Roadmap

### Current Status (Phase 1) ✅
- [x] Core CQL compiler with template system
- [x] AILib provider interface and Anthropic integration
- [x] HTTP client with retry logic
- [x] Secure configuration management
- [x] Comprehensive testing infrastructure

### Phase 2: Multi-Provider Support
- [ ] OpenAI GPT integration
- [ ] Google Gemini support
- [ ] Provider plugin architecture

### Phase 3: Advanced Features
- [ ] Streaming response support
- [ ] Function calling capabilities
- [ ] Advanced authentication methods

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
