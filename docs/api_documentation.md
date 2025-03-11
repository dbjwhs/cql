# CQL API Documentation

This document provides a comprehensive reference for developers looking to integrate with or extend the Claude Query Language (CQL) compiler.

## Claude API Integration

CQL now provides direct integration with Anthropic's Claude API, enabling developers to submit queries and receive generated code in a single workflow.

### ApiClient Class

The `ApiClient` class provides communication with the Claude API, offering both synchronous and asynchronous methods as well as streaming capabilities.

```cpp
class ApiClient {
public:
    // Constructor with configuration
    explicit ApiClient(const Config& config);
    
    // Configuration setters
    void set_api_key(const std::string& api_key);
    void set_model(const std::string& model);
    void set_max_tokens(int max_tokens);
    void set_temperature(double temperature);
    void set_timeout(int timeout_seconds);
    void set_max_retries(int max_retries);
    
    // Synchronous query submission
    [[nodiscard]] ApiResponse submit_query(const std::string& query) const;
    
    // Asynchronous query submission
    [[nodiscard]] std::future<ApiResponse> submit_query_async(
        const std::string& query,
        const std::function<void(ApiResponse)>& callback = nullptr) const;
    
    // Streaming query submission
    void submit_query_streaming(
        const std::string& query,
        const std::function<void(std::string_view chunk)>& chunk_callback,
        const std::function<void(ApiResponse)>& completion_callback = nullptr) const;
    
    // Asynchronous streaming query submission
    [[nodiscard]] std::future<void> submit_query_streaming_async(
        const std::string& query,
        const std::function<void(std::string_view chunk)>& chunk_callback,
        const std::function<void(ApiResponse)>& completion_callback = nullptr) const;
};
```

### ApiResponse Structure

```cpp
struct ApiResponse {
    bool m_success = false;
    int m_status_code = 0;
    std::string m_raw_response;
    std::string m_error_message;
    ApiErrorCategory m_error_category = ApiErrorCategory::None;
    std::vector<GeneratedFile> m_generated_files;
    
    // Helper methods
    [[nodiscard]] bool is_retryable() const;
};
```

### Error Categories

```cpp
enum class ApiErrorCategory {
    None,
    Network,        // Network connectivity issues
    Authentication, // API key issues
    RateLimit,      // Rate limiting or quota
    Server,         // Server-side errors
    Timeout,        // Request timeout
    Client,         // Client-side errors
    Unknown         // Unknown errors
};
```

### Configuration

```cpp
class Config {
public:
    // Load configuration from default locations
    static Config load_from_default_locations();
    
    // Getters and setters
    std::string get_api_key() const;
    void set_api_key(const std::string& api_key);
    
    std::string get_model() const;
    void set_model(const std::string& model);
    
    double get_temperature() const;
    void set_temperature(double temperature);
    
    int get_max_tokens() const;
    void set_max_tokens(int max_tokens);
    
    int get_timeout() const;
    void set_timeout(int timeout_seconds);
    
    int get_max_retries() const;
    void set_max_retries(int max_retries);
    
    // Validate API key
    bool validate_api_key() const;
};
```

### Example: Synchronous API Call

```cpp
#include "cql/api_client.hpp"

int main() {
    // Create configuration
    cql::Config config = cql::Config::load_from_default_locations();
    config.set_model("claude-3-opus");
    config.set_temperature(0.7);
    
    // Create API client
    cql::ApiClient api_client(config);
    
    // Prepare and submit query
    std::string query = "@language \"C++\"\n@description \"implement a thread-safe queue\"";
    std::string compiled_query = cql::QueryProcessor::compile(query);
    
    // Submit synchronously
    cql::ApiResponse response = api_client.submit_query(compiled_query);
    
    if (response.m_success) {
        std::cout << "Generated " << response.m_generated_files.size() << " files" << std::endl;
        for (const auto& file : response.m_generated_files) {
            std::cout << "File: " << file.filename << std::endl;
            std::cout << file.content << std::endl;
        }
    } else {
        std::cerr << "Error: " << response.m_error_message << std::endl;
        std::cerr << "Category: " << static_cast<int>(response.m_error_category) << std::endl;
    }
    
    return 0;
}
```

### Example: Asynchronous API Call

```cpp
#include "cql/api_client.hpp"
#include <future>
#include <iostream>

int main() {
    // Create configuration and client
    cql::Config config = cql::Config::load_from_default_locations();
    cql::ApiClient api_client(config);
    
    // Prepare query
    std::string query = "@language \"C++\"\n@description \"implement a binary search tree\"";
    std::string compiled_query = cql::QueryProcessor::compile(query);
    
    // Submit asynchronously
    std::future<cql::ApiResponse> future = api_client.submit_query_async(
        compiled_query,
        [](const cql::ApiResponse& response) {
            // This callback runs when the request completes
            if (response.m_success) {
                std::cout << "Callback: Request succeeded" << std::endl;
            } else {
                std::cout << "Callback: Request failed: " << response.m_error_message << std::endl;
            }
        }
    );
    
    // Do other work while request is processing
    std::cout << "Request submitted, doing other work..." << std::endl;
    
    // Wait for and process the result
    cql::ApiResponse response = future.get();
    
    if (response.m_success) {
        for (const auto& file : response.m_generated_files) {
            std::cout << "Generated file: " << file.filename << std::endl;
        }
    }
    
    return 0;
}
```

### Example: Streaming API Call

```cpp
#include "cql/api_client.hpp"
#include <iostream>
#include <mutex>

int main() {
    // Create configuration and client
    cql::Config config = cql::Config::load_from_default_locations();
    cql::ApiClient api_client(config);
    
    // Prepare query (longer query for streaming demonstration)
    std::string query = "@language \"C++\"\n@description \"implement a full-featured HTTP server\"";
    std::string compiled_query = cql::QueryProcessor::compile(query);
    
    // Mutex for thread-safe console output
    std::mutex cout_mutex;
    
    // Submit with streaming
    api_client.submit_query_streaming(
        compiled_query,
        // Chunk callback - called as chunks arrive
        [&cout_mutex](std::string_view chunk) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Received chunk: " << chunk.size() << " bytes" << std::endl;
            // Process the chunk (e.g., display progress)
        },
        // Completion callback - called when streaming finishes
        [&cout_mutex](const cql::ApiResponse& response) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            if (response.m_success) {
                std::cout << "Streaming complete, generated " 
                          << response.m_generated_files.size() << " files" << std::endl;
            } else {
                std::cout << "Streaming failed: " << response.m_error_message << std::endl;
            }
        }
    );
    
    return 0;
}
```

### Error Handling with Retries

The API client includes built-in retry logic with exponential backoff for transient errors:

```cpp
// Configure retry behavior
config.set_max_retries(3);  // Maximum number of retry attempts
config.set_timeout(30);     // Timeout in seconds for each attempt

// The API client will automatically retry on retryable errors:
// - Network errors
// - Rate limit errors
// - Server errors

// You can check if an error is retryable
if (response.m_success == false && response.is_retryable()) {
    std::cout << "This error type would trigger automatic retry" << std::endl;
}
```

### Response Processing

The response processor extracts and organizes code from Claude's responses:

```cpp
// Example of processing a raw response into files
cql::ResponseProcessor processor;
std::vector<cql::GeneratedFile> files = processor.process_response(response.m_raw_response);

// Write files to disk
for (const auto& file : files) {
    std::string output_path = output_dir + "/" + file.filename;
    cql::util::write_file(output_path, file.content);
}
```

## Core Classes

### QueryProcessor

The main entry point for compiling CQL queries.

```cpp
class QueryProcessor {
public:
    // Compile a CQL query string into output
    static std::string compile(const std::string& query);
    
    // Compile a CQL query from a file
    static std::string compile_file(const std::string& input_file);
    
    // Write compiled output to a file
    static void compile_to_file(const std::string& input_file, const std::string& output_file);
};
```

**Example:**
```cpp
#include "cql/cql.hpp"

int main() {
    std::string query = "@language \"C++\"\n@description \"implement a stack\"";
    std::string result = cql::QueryProcessor::compile(query);
    // Process the result...
    return 0;
}
```

### TemplateManager

Manages storing, retrieving, and instantiating templates.

```cpp
class TemplateManager {
public:
    // Constructor
    explicit TemplateManager(const std::string& templates_dir);
    
    // Save a template
    void save_template(const std::string& name, const std::string& content);
    
    // Load a template
    std::string load_template(const std::string& name);
    
    // Load a template with its entire inheritance chain
    std::string load_template_with_inheritance(const std::string& name);
    
    // Delete a template
    bool delete_template(const std::string& name);
    
    // List all available templates
    std::vector<std::string> list_templates();
    
    // List categories
    std::vector<std::string> list_categories();
    
    // Create a new category
    bool create_category(const std::string& name);
    
    // Get template metadata
    TemplateMetadata get_template_metadata(const std::string& name);
    
    // Get inheritance chain
    std::vector<std::string> get_inheritance_chain(const std::string& name);
    
    // Instantiate a template with variables
    std::string instantiate_template(const std::string& name, 
                                    const std::map<std::string, std::string>& variables);
};
```

**Example:**
```cpp
#include "cql/template_manager.hpp"

int main() {
    // Initialize the template manager
    cql::TemplateManager manager("./templates");
    
    // List all templates
    auto templates = manager.list_templates();
    for (const auto& tmpl : templates) {
        std::cout << tmpl << std::endl;
    }
    
    // Load a template with inheritance
    std::string template_content = manager.load_template_with_inheritance("my_template");
    
    // Instantiate with variables
    std::map<std::string, std::string> vars = {
        {"container", "vector"},
        {"type", "int"}
    };
    std::string instantiated = manager.instantiate_template("my_template", vars);
    
    return 0;
}
```

### TemplateValidator

Validates template content for errors and warnings.

```cpp
class TemplateValidator {
public:
    // Constructor
    explicit TemplateValidator(TemplateManager& manager);
    
    // Add a validation rule
    void add_validation_rule(ValidationRule rule);
    
    // Validate a template
    TemplateValidationResult validate_template(const std::string& name);
    
    // Validate template content directly
    TemplateValidationResult validate_content(const std::string& content);
};
```

**Example:**
```cpp
#include "cql/template_validator.hpp"

int main() {
    cql::TemplateManager manager("./templates");
    cql::TemplateValidator validator(manager);
    
    // Add a custom validation rule
    validator.add_validation_rule([](const std::string& content) {
        std::vector<cql::TemplateValidationIssue> issues;
        // Custom validation logic...
        return issues;
    });
    
    // Validate a template
    auto result = validator.validate_template("my_template");
    
    // Check for issues
    if (result.has_issues(cql::TemplateValidationLevel::ERROR)) {
        std::cout << "Template has errors!" << std::endl;
        for (const auto& issue : result.get_issues()) {
            if (issue.level == cql::TemplateValidationLevel::ERROR) {
                std::cout << issue.message << std::endl;
            }
        }
    }
    
    return 0;
}
```

## Node Classes

CQL uses a visitor-based design pattern for node traversal.

### Base Node

```cpp
class Node {
public:
    virtual ~Node() = default;
    virtual void accept(Visitor& visitor) = 0;
};
```

### Directive Node

```cpp
class DirectiveNode : public Node {
public:
    DirectiveNode(const std::string& name, const std::string& value);
    void accept(Visitor& visitor) override;
    
    std::string name;
    std::string value;
};
```

### Variable Node

```cpp
class VariableNode : public Node {
public:
    VariableNode(const std::string& name, const std::string& value);
    void accept(Visitor& visitor) override;
    
    std::string name;
    std::string value;
};
```

### Visitor Pattern

```cpp
class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void visit(DirectiveNode& node) = 0;
    virtual void visit(VariableNode& node) = 0;
    // Other visit methods for other node types...
};
```

## Extending CQL

### Creating Custom Validators

```cpp
#include "cql/template_validator.hpp"

// Custom validation function type
using ValidationRule = std::function<std::vector<TemplateValidationIssue>(const std::string&)>;

// Create a custom validator for specific directives
ValidationRule create_directive_validator(const std::string& directive_name) {
    return [directive_name](const std::string& content) {
        std::vector<TemplateValidationIssue> issues;
        std::regex directive_regex("@" + directive_name + "\\s+\"([^\"]+)\"");
        
        std::smatch m;
        std::string::const_iterator search_start(content.cbegin());
        while (std::regex_search(search_start, content.cend(), m, directive_regex)) {
            std::string value = m[1].str();
            // Custom validation logic for the directive value
            if (value.length() < 3) {
                issues.emplace_back(
                    TemplateValidationLevel::WARNING,
                    directive_name + " value is too short: " + value
                );
            }
            search_start = m.suffix().first;
        }
        
        return issues;
    };
}

int main() {
    cql::TemplateManager manager("./templates");
    cql::TemplateValidator validator(manager);
    
    // Add custom validators
    validator.add_validation_rule(create_directive_validator("description"));
    validator.add_validation_rule(create_directive_validator("language"));
    
    // Validate a template
    auto result = validator.validate_template("my_template");
    
    return 0;
}
```

### Creating Custom Visitors

```cpp
#include "cql/visitor.hpp"

// Custom visitor that counts directive types
class DirectiveCounter : public cql::Visitor {
public:
    void visit(cql::DirectiveNode& node) override {
        directive_counts[node.name]++;
    }
    
    void visit(cql::VariableNode& node) override {
        // Not counted
    }
    
    // Other visit methods...
    
    std::map<std::string, int> directive_counts;
};

int main() {
    // Parse template into nodes...
    DirectiveCounter counter;
    
    // Accept the visitor
    root_node->accept(counter);
    
    // Print the counts
    for (const auto& [directive, count] : counter.directive_counts) {
        std::cout << directive << ": " << count << std::endl;
    }
    
    return 0;
}
```

## Error Handling

```cpp
try {
    cql::TemplateManager manager("./templates");
    std::string content = manager.load_template("non_existent");
} catch (const cql::TemplateNotFoundException& e) {
    std::cerr << "Template not found: " << e.what() << std::endl;
} catch (const cql::CircularInheritanceException& e) {
    std::cerr << "Circular inheritance: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Other error: " << e.what() << std::endl;
}
```

## Advanced Template Processing

```cpp
#include "cql/template_manager.hpp"
#include "cql/template_validator.hpp"

int main() {
    // Initialize managers
    cql::TemplateManager manager("./templates");
    cql::TemplateValidator validator(manager);
    
    // Get metadata for a template
    auto metadata = manager.get_template_metadata("my_template");
    
    // Check for parent
    if (metadata.parent.has_value()) {
        std::cout << "Parent template: " << metadata.parent.value() << std::endl;
    }
    
    // Get the inheritance chain
    auto chain = manager.get_inheritance_chain("my_template");
    std::cout << "Inheritance chain: ";
    for (const auto& ancestor : chain) {
        std::cout << ancestor << " -> ";
    }
    std::cout << "my_template" << std::endl;
    
    // Validate the template
    auto result = validator.validate_template("my_template");
    
    // Process only if no errors
    if (!result.has_issues(cql::TemplateValidationLevel::ERROR)) {
        // Get variables from metadata
        for (const auto& var : metadata.variables) {
            std::cout << "Variable: " << var << std::endl;
        }
        
        // Create variable map
        std::map<std::string, std::string> vars;
        for (const auto& var : metadata.variables) {
            // Prompt user for value or use default
            vars[var] = "default_value"; // Replace with actual logic
        }
        
        // Instantiate the template with variables
        std::string instantiated = manager.instantiate_template("my_template", vars);
        
        // Process the instantiated template
        std::cout << "Instantiated template: " << instantiated << std::endl;
    } else {
        // Handle validation errors
        for (const auto& issue : result.get_issues()) {
            if (issue.level == cql::TemplateValidationLevel::ERROR) {
                std::cerr << "Error: " << issue.message << std::endl;
            }
        }
    }
    
    return 0;
}
```
