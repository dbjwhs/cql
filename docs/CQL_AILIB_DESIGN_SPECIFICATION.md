# AILib: Comprehensive Design Specification for C++ AI Provider Library

## Executive Summary

AILib is a proposed standalone C++ library providing unified interfaces to major AI providers (Anthropic, OpenAI, Google, etc.). This specification addresses the critical gap in the C++ ecosystem where no major AI provider offers official C++ SDK support, forcing developers to implement raw HTTP clients or use Python bindings.

## Market Analysis & Opportunity

### Current State of AI Provider SDKs

| Provider | Python | JS/TS | Java | Go | Ruby | **C++** | Rust | Market Gap |
|----------|--------|-------|------|----|----- |---------|------|------------|
| **Anthropic (Claude)** | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ **NONE** | ❌ | **High** |
| **OpenAI (GPT)** | ✅ | ✅ | ❌ | ✅ | ❌ | ❌ **NONE** | ❌ | **High** |
| **Google (Gemini)** | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ **NONE** | 🟡 Community | **High** |
| **Cohere** | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ **NONE** | ❌ | **Critical** |
| **Mistral** | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ **NONE** | ❌ | **Critical** |
| **Hugging Face** | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ **NONE** | ❌ | **Critical** |
| **AWS Bedrock** | ✅ | ✅ | 🟡 | 🟡 | ❌ | ❌ **NONE** | ❌ | **High** |

### Strategic Opportunity

- **Universal C++ Gap**: Zero official C++ support across 8+ major providers
- **Market Size**: Enterprise C++ applications forced to use suboptimal solutions
- **Technical Feasibility**: All providers use REST APIs with consistent patterns
- **Community Impact**: Could become the definitive standard (like nlohmann/json)

## Library Architecture

### Core Design Principles

1. **Modern C++20**: Concepts, ranges, coroutines, string_view
2. **Header-Only Option**: For easy integration like nlohmann/json
3. **PIMPL Pattern**: ABI stability for shared library versions
4. **Provider Abstraction**: Unified interface across all AI services
5. **Type Safety**: Strong typing with compile-time verification
6. **Performance**: Async-first design with minimal overhead
7. **Security**: Built-in API key protection and secure HTTP handling

### High-Level Architecture

```cpp
namespace ailib {
    // Core abstractions
    class provider_interface;
    class request;
    class response;
    class streaming_response;
    
    // Provider implementations
    namespace providers {
        class anthropic;
        class openai;
        class google;
        class cohere;
        // ... others
    }
    
    // Utilities
    class provider_factory;
    class http_client;
    class auth_manager;
}
```

## Directory Structure

### Repository Layout (Phase 1: Within CQL)

```
cql/
├── lib/
│   └── ailib/                           # Future standalone library
│       ├── CMakeLists.txt               # Library build configuration
│       ├── include/
│       │   └── ailib/
│       │       ├── ailib.hpp            # Single-header include (convenience)
│       │       ├── core/
│       │       │   ├── provider.hpp     # Core provider interface
│       │       │   ├── request.hpp      # Request types
│       │       │   ├── response.hpp     # Response types
│       │       │   ├── config.hpp       # Configuration management
│       │       │   └── errors.hpp       # Error handling
│       │       ├── providers/
│       │       │   ├── anthropic.hpp    # Claude implementation
│       │       │   ├── openai.hpp       # GPT implementation
│       │       │   ├── google.hpp       # Gemini implementation
│       │       │   └── factory.hpp      # Provider factory
│       │       ├── http/
│       │       │   ├── client.hpp       # HTTP client interface
│       │       │   └── curl_impl.hpp    # CURL implementation
│       │       ├── auth/
│       │       │   ├── manager.hpp      # Authentication handling
│       │       │   └── secure_store.hpp # Secure API key storage
│       │       └── detail/
│       │           ├── impl.hpp         # Private implementation
│       │           ├── platform.hpp     # Platform-specific code
│       │           └── json_utils.hpp   # JSON utilities
│       ├── src/                         # Implementation files (non-header-only)
│       │   ├── providers/
│       │   ├── http/
│       │   └── auth/
│       ├── tests/
│       │   ├── unit/
│       │   ├── integration/
│       │   └── mock_servers/
│       ├── examples/
│       │   ├── basic_usage.cpp
│       │   ├── streaming_example.cpp
│       │   └── multi_provider.cpp
│       └── docs/
│           ├── api_reference.md
│           ├── provider_guide.md
│           └── migration_guide.md
└── src/cql/                            # CQL-specific code (uses ailib)
```

### Standalone Repository Structure (Phase 2)

```
ailib/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── CHANGELOG.md
├── conanfile.py                         # Conan package support
├── vcpkg.json                          # vcpkg package support
├── cmake/
│   ├── Config.cmake.in                 # CMake package config
│   └── find-modules/
├── include/ailib/                      # Public API headers
├── src/                                # Implementation (if not header-only)
├── tests/
├── examples/
├── docs/
├── tools/                              # Development tools
└── third_party/                        # External dependencies
```

## Public API Design

### Core Interfaces

```cpp
namespace ailib {

// Version information
struct version {
    static constexpr int major = 1;
    static constexpr int minor = 0;
    static constexpr int patch = 0;
    static constexpr std::string_view string = "1.0.0";
};

// Forward declarations
class request;
class response;
class streaming_response;

// Main provider interface
class provider_interface {
public:
    virtual ~provider_interface() = default;
    
    // Synchronous generation
    [[nodiscard]] virtual response generate(const request& req) = 0;
    
    // Asynchronous generation  
    [[nodiscard]] virtual std::future<response> generate_async(const request& req) = 0;
    
    // Streaming generation
    virtual void generate_stream(const request& req, 
                               std::function<void(const streaming_response&)> callback) = 0;
    
    // Provider metadata
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual std::vector<std::string> supported_models() const = 0;
    [[nodiscard]] virtual provider_capabilities capabilities() const = 0;
};

// Request configuration
class request {
public:
    explicit request(std::string_view message);
    
    // Fluent API for configuration
    request& model(std::string_view model_name);
    request& max_tokens(int tokens);
    request& temperature(double temp);
    request& system_prompt(std::string_view prompt);
    request& add_message(role r, std::string_view content);
    
private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};

// Response handling
class response {
public:
    [[nodiscard]] bool success() const;
    [[nodiscard]] std::string content() const;
    [[nodiscard]] std::string model() const;
    [[nodiscard]] int tokens_used() const;
    [[nodiscard]] std::chrono::milliseconds latency() const;
    
    // Error handling
    [[nodiscard]] std::optional<error_info> error() const;
    
private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};

// Provider factory
class provider_factory {
public:
    [[nodiscard]] static std::unique_ptr<provider_interface> 
    create(std::string_view provider_name, const config& cfg);
    
    [[nodiscard]] static std::vector<std::string> available_providers();
    
    // Registration for custom providers
    static void register_provider(std::string_view name, 
                                 std::function<std::unique_ptr<provider_interface>(const config&)> factory);
};

} // namespace ailib
```

### Configuration Management

```cpp
namespace ailib {

class config {
public:
    // Load from various sources
    [[nodiscard]] static config from_environment();
    [[nodiscard]] static config from_file(const std::filesystem::path& path);
    [[nodiscard]] static config from_json(std::string_view json);
    
    // Provider-specific configuration
    config& api_key(std::string_view key);
    config& base_url(std::string_view url);
    config& timeout(std::chrono::seconds timeout);
    config& max_retries(int retries);
    config& proxy(std::string_view proxy_url);
    
    // Security options
    config& verify_ssl(bool verify = true);
    config& secure_storage(bool enable = true);
    
private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};

} // namespace ailib
```

### Error Handling

```cpp
namespace ailib {

enum class error_code {
    none = 0,
    network_error,
    authentication_error,
    rate_limit_error,
    invalid_request,
    server_error,
    timeout_error,
    parsing_error
};

struct error_info {
    error_code code;
    std::string message;
    std::optional<int> http_status;
    std::optional<std::string> provider_error_code;
};

class ailib_exception : public std::exception {
public:
    explicit ailib_exception(error_info info);
    [[nodiscard]] const char* what() const noexcept override;
    [[nodiscard]] const error_info& info() const noexcept;
    
private:
    error_info error_info_;
    std::string what_message_;
};

} // namespace ailib
```

## Private Implementation Structure

### PIMPL Pattern Implementation

```cpp
// In detail/impl.hpp (private)
namespace ailib::detail {

struct provider_impl_base {
    virtual ~provider_impl_base() = default;
    virtual response generate_impl(const request_impl& req) = 0;
    // ... other virtual methods
};

struct anthropic_impl : public provider_impl_base {
    explicit anthropic_impl(const config& cfg);
    response generate_impl(const request_impl& req) override;
    // ... implementation details
    
private:
    std::string api_key_;
    std::string base_url_;
    std::unique_ptr<http_client> client_;
};

} // namespace ailib::detail
```

### HTTP Client Abstraction

```cpp
// In http/client.hpp
namespace ailib::http {

class client_interface {
public:
    virtual ~client_interface() = default;
    
    struct request {
        std::string url;
        std::string method;
        std::map<std::string, std::string> headers;
        std::string body;
    };
    
    struct response {
        int status_code;
        std::map<std::string, std::string> headers;
        std::string body;
    };
    
    [[nodiscard]] virtual response send(const request& req) = 0;
    [[nodiscard]] virtual std::future<response> send_async(const request& req) = 0;
};

// Platform-specific implementations
class curl_client : public client_interface {
    // CURL-based implementation
};

#ifdef _WIN32
class winhttp_client : public client_interface {
    // WinHTTP-based implementation
};
#endif

} // namespace ailib::http
```

## Versioning Strategy

### Semantic Versioning (SemVer 2.0.0)

```cpp
// Version encoding
#define AILIB_VERSION_MAJOR 1
#define AILIB_VERSION_MINOR 0
#define AILIB_VERSION_PATCH 0
#define AILIB_VERSION_STRING "1.0.0"

// Runtime version checking
namespace ailib {
    struct version_info {
        int major, minor, patch;
        std::string_view string;
        
        [[nodiscard]] bool compatible_with(int req_major, int req_minor = 0) const {
            return major == req_major && (major > 0 ? minor >= req_minor : minor == req_minor);
        }
    };
    
    [[nodiscard]] constexpr version_info version() noexcept {
        return {AILIB_VERSION_MAJOR, AILIB_VERSION_MINOR, AILIB_VERSION_PATCH, AILIB_VERSION_STRING};
    }
}
```

### API Evolution Strategy

```cpp
namespace ailib {
    // Versioned APIs using inline namespaces
    inline namespace v1 {
        class provider_interface { /* current API */ };
    }
    
    namespace v0 {
        [[deprecated("Use v1::provider_interface")]]
        class provider_interface { /* legacy API */ };
    }
}
```

### ABI Compatibility

- **Major Version**: Breaking changes allowed
- **Minor Version**: New features, backward compatible
- **Patch Version**: Bug fixes, fully compatible

**ABI Stability Techniques:**
1. PIMPL pattern for all public classes
2. Virtual destructors for all base classes
3. Avoid inline functions in public API
4. Use opaque handles for complex types

## Build System Design

### CMake Configuration

```cmake
cmake_minimum_required(VERSION 3.20)
project(ailib VERSION 1.0.0 LANGUAGES CXX)

# Modern C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(AILIB_BUILD_SHARED "Build shared library" OFF)
option(AILIB_BUILD_STATIC "Build static library" ON)
option(AILIB_HEADER_ONLY "Header-only mode" OFF)
option(AILIB_BUILD_TESTS "Build tests" OFF)
option(AILIB_BUILD_EXAMPLES "Build examples" OFF)

# Dependencies
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.10.0 REQUIRED)
find_package(Threads REQUIRED)

# Library target
add_library(ailib)

# Configure build type
if(AILIB_HEADER_ONLY)
    target_compile_definitions(ailib INTERFACE AILIB_HEADER_ONLY)
    set_target_properties(ailib PROPERTIES LINKER_LANGUAGE CXX)
else()
    # Implementation-based library
    target_sources(ailib PRIVATE ${AILIB_SOURCES})
endif()

# Public interface
target_include_directories(ailib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(ailib 
    PUBLIC 
        nlohmann_json::nlohmann_json
    PRIVATE
        CURL::libcurl
        Threads::Threads
)

# Compiler-specific options
if(MSVC)
    target_compile_options(ailib PRIVATE /W4)
else()
    target_compile_options(ailib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ailib
    EXPORT ailib-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ailib
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Package configuration
configure_package_config_file(
    cmake/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ailib-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ailib
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ailib-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ailib-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ailib-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ailib
)

install(EXPORT ailib-targets
    FILE ailib-targets.cmake
    NAMESPACE ailib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ailib
)
```

### Package Manager Support

**vcpkg.json**:
```json
{
    "name": "ailib",
    "version": "1.0.0",
    "description": "Modern C++ library for AI provider integration",
    "license": "MIT",
    "dependencies": [
        {
            "name": "curl",
            "platform": "!uwp"
        },
        {
            "name": "nlohmann-json",
            "version>=": "3.10.0"
        }
    ]
}
```

**conanfile.py**:
```python
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps

class AilibConan(ConanFile):
    name = "ailib"
    version = "1.0.0"
    
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "header_only": [True, False],
    }
    default_options = {
        "shared": False,
        "header_only": False,
    }
    
    def requirements(self):
        self.requires("libcurl/8.0.1")
        self.requires("nlohmann_json/3.11.2")
```

## Static vs Shared Library Strategy

### Recommended Approach: Hybrid with Static Default

**Static Library Benefits** (Primary recommendation):
- No runtime dependencies to manage
- Better security (contained API keys, no external attacks)
- Easier deployment (single executable)
- Better compiler optimization opportunities
- Cross-platform compatibility

**Shared Library Benefits** (Optional for specific use cases):
- Smaller executable size when multiple applications use ailib
- Runtime updates for security patches
- Plugin architecture possibilities

**Implementation Strategy**:
```cmake
# Default to static, allow shared
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if(BUILD_SHARED_LIBS)
    # Shared library configuration
    target_compile_definitions(ailib PUBLIC AILIB_SHARED)
    if(WIN32)
        target_compile_definitions(ailib PRIVATE AILIB_EXPORTS)
    endif()
else()
    # Static library configuration (default)
    target_compile_definitions(ailib PUBLIC AILIB_STATIC)
endif()
```

## Repository Strategy

### Phase 1: Incubate within CQL Repository

**Directory Structure**:
```
cql/
├── lib/ailib/              # Self-contained library
├── src/cql/                # CQL uses ailib
└── docs/                   # Shared documentation
```

**Benefits**:
- Rapid development and iteration
- Real-world testing with CQL as primary consumer
- Single repository management
- Easy API validation and refinement

### Phase 2: Extract to Standalone Repository

**Git History Preservation**:
```bash
# Method 1: git subtree (preserves history)
git subtree push --prefix=lib/ailib origin ailib-standalone

# Method 2: git filter-branch (complete history extraction)
git filter-branch --subdirectory-filter lib/ailib -- --all
```

**Migration Timeline**:
1. **Month 1-2**: Initial implementation within CQL
2. **Month 3**: API stabilization and documentation
3. **Month 4**: Community feedback and testing
4. **Month 5**: Repository extraction and open source release

## Implementation Phases

### Phase 1: Core Foundation (4-6 weeks)
- [ ] Basic provider interface
- [ ] HTTP client abstraction (CURL-based)
- [ ] Anthropic provider implementation
- [ ] Configuration management
- [ ] Error handling system

### Phase 2: Multi-Provider Support (3-4 weeks)
- [ ] OpenAI provider implementation
- [ ] Google (Gemini) provider implementation
- [ ] Provider factory system
- [ ] Unified request/response types

### Phase 3: Advanced Features (4-6 weeks)
- [ ] Streaming response support
- [ ] Async/Future-based API
- [ ] Authentication management
- [ ] Retry logic with exponential backoff

### Phase 4: Production Readiness (2-3 weeks)
- [ ] Comprehensive testing suite
- [ ] Documentation and examples
- [ ] Performance optimization
- [ ] Security audit

### Phase 5: Community Release (2-3 weeks)
- [ ] Repository extraction
- [ ] Package manager integration (vcpkg, Conan)
- [ ] Open source release
- [ ] Community documentation

## Testing Strategy

### Unit Testing Framework
```cpp
// Using GoogleTest for consistency with CQL
#include <gtest/gtest.h>
#include <ailib/ailib.hpp>

class ProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = ailib::config::from_environment()
                    .timeout(std::chrono::seconds(30))
                    .max_retries(3);
    }
    
    ailib::config config_;
};

TEST_F(ProviderTest, AnthropicBasicGeneration) {
    auto provider = ailib::provider_factory::create("anthropic", config_);
    
    auto request = ailib::request("Hello, world!")
                     .model("claude-3-opus-20240229")
                     .max_tokens(100);
    
    auto response = provider->generate(request);
    
    EXPECT_TRUE(response.success());
    EXPECT_FALSE(response.content().empty());
    EXPECT_GT(response.tokens_used(), 0);
}
```

### Integration Testing
- Live API testing with sandbox endpoints
- Mock server testing for error scenarios
- Performance benchmarking
- Memory leak detection with Valgrind/AddressSanitizer

### Cross-Platform Testing
- GitHub Actions CI/CD for Linux, Windows, macOS
- Various compiler testing (GCC, Clang, MSVC)
- Multiple C++ standard versions (C++17, C++20, C++23)

## Documentation Strategy

### API Documentation (Doxygen)
```cpp
/**
 * @brief Creates a new AI provider instance
 * 
 * @param provider_name Name of the provider (e.g., "anthropic", "openai")
 * @param config Configuration for the provider including API keys
 * @return std::unique_ptr<provider_interface> Provider instance
 * 
 * @throws ailib_exception If provider_name is unknown or config is invalid
 * 
 * @example
 * @code
 * auto config = ailib::config::from_environment();
 * auto provider = ailib::provider_factory::create("anthropic", config);
 * @endcode
 */
[[nodiscard]] static std::unique_ptr<provider_interface> 
create(std::string_view provider_name, const config& cfg);
```

### Usage Guides
1. **Quick Start Guide**: Basic usage in 5 minutes
2. **Provider Guide**: Detailed configuration for each provider
3. **Advanced Features**: Streaming, async, custom providers
4. **Migration Guide**: From raw HTTP to ailib
5. **Troubleshooting**: Common issues and solutions

## Security Considerations

### API Key Protection
```cpp
namespace ailib::auth {

class secure_string {
public:
    explicit secure_string(std::string_view data);
    ~secure_string();
    
    // Non-copyable to prevent accidental exposure
    secure_string(const secure_string&) = delete;
    secure_string& operator=(const secure_string&) = delete;
    
    // Move-only
    secure_string(secure_string&&) noexcept;
    secure_string& operator=(secure_string&&) noexcept;
    
    [[nodiscard]] std::string_view view() const noexcept;
    
private:
    void* data_;
    size_t size_;
};

} // namespace ailib::auth
```

### HTTP Security
- Mandatory TLS/SSL for all connections
- Certificate verification enabled by default
- Secure random number generation for nonces
- Protection against timing attacks

### Memory Security
- Secure memory allocation for sensitive data
- Memory wiping for API keys and tokens
- Stack protection for credential handling

## Performance Considerations

### HTTP Client Optimization
- Connection pooling and reuse
- Compression support (gzip, deflate)
- Configurable timeout handling
- Efficient memory management for large responses

### Async Design
```cpp
// Coroutine-based API (C++20)
namespace ailib {

class async_provider {
public:
    [[nodiscard]] std::future<response> generate(const request& req);
    
    // C++20 coroutines support
    auto generate_coro(const request& req) -> std::future<response>;
    
    // Streaming with coroutines
    auto generate_stream_coro(const request& req) -> generator<streaming_response>;
};

} // namespace ailib
```

### Memory Management
- RAII patterns throughout
- Smart pointer usage for automatic cleanup
- Minimal copying with move semantics
- Optional arena allocators for high-performance scenarios

## Future Extensibility

### Plugin Architecture
```cpp
// Support for runtime-loaded providers
namespace ailib {

class dynamic_provider_loader {
public:
    void load_provider(const std::filesystem::path& library_path);
    void register_provider_factory(std::string_view name, provider_factory_func func);
};

} // namespace ailib
```

### Custom Provider Interface
```cpp
// Allow users to implement custom providers
class custom_provider : public ailib::provider_interface {
public:
    response generate(const request& req) override {
        // Custom implementation
    }
};

// Registration
ailib::provider_factory::register_provider("custom", 
    [](const config& cfg) -> std::unique_ptr<provider_interface> {
        return std::make_unique<custom_provider>(cfg);
    });
```

## Community and Open Source Strategy

### License Strategy
- **MIT License**: Maximum compatibility and adoption
- Clear contribution guidelines
- Contributor License Agreement (CLA) for larger contributions

### Community Building
1. **GitHub Discussions**: Technical support and feature requests
2. **Documentation Wiki**: Community-maintained guides
3. **Example Repository**: Real-world usage examples
4. **Blog Posts**: Technical deep-dives and tutorials

### Contribution Guidelines
- Modern C++ coding standards
- Comprehensive test coverage requirements
- Documentation for all public APIs
- Security review process for sensitive changes

## Success Metrics

### Technical Metrics
- **Compile Time**: < 5 seconds for basic usage
- **Runtime Performance**: < 10ms overhead per request
- **Memory Usage**: < 1MB base memory footprint
- **Coverage**: > 90% test coverage

### Community Metrics
- GitHub stars and forks
- Package manager downloads (vcpkg, Conan)
- Community contributions and issues
- Documentation page views

### Business Impact
- CQL adoption and usage
- Enterprise customer feedback
- Developer productivity improvements
- Reduced barrier to AI integration in C++

## Conclusion

AILib represents a strategic opportunity to fill a critical gap in the C++ ecosystem while providing CQL with a competitive advantage. The universal absence of official C++ SDKs from major AI providers creates a significant market opportunity for a well-designed, comprehensive library.

The proposed architecture balances modern C++ practices with practical deployment concerns, ensuring the library will be both powerful and accessible to the C++ community. By incubating the library within CQL and later extracting it as a standalone project, we can validate the design with real-world usage while contributing valuable infrastructure to the broader ecosystem.

This approach positions CQL as an innovator in the AI tooling space while building a valuable standalone asset that could become essential infrastructure for C++ AI development.

---

*Ready for that beer when this ships! 🍺 The C++ community will thank us for finally giving them proper AI integration tooling.*
