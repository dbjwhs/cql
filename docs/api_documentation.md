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

## CQL Directives

CQL supports a variety of directives that provide instructions to the compiler and AI model. Below is a reference of all supported directives:

### Model Control Directives

These directives control the AI model's behavior:

| Directive | Description | Example |
|-----------|-------------|---------|
| `@model` | Specifies which Claude model to use | `@model "claude-3-opus"` |
| `@max_tokens` | Sets the maximum number of tokens in the response | `@max_tokens 100000` |
| `@temperature` | Controls the randomness/creativity of the response (0.0-1.0) | `@temperature 0.7` |
| `@output_format` | Specifies how to format the output | `@output_format "multiple_files"` |

#### Temperature

The temperature parameter controls the randomness or creativity of the AI model's response:
- **Lower values (e.g., 0.1-0.3)**: More deterministic, focused, and conservative responses
- **Mid-range values (e.g., 0.4-0.7)**: Balanced between deterministic and creative
- **Higher values (e.g., 0.8-1.0)**: More creative, diverse, and potentially unexpected responses

For code generation, lower temperatures are often preferred for predictable, standard implementations, while higher temperatures might be useful for creative problem-solving or exploring alternative approaches.

#### Output Format

The output_format directive determines how the generated code is structured:
- **"multiple_files"**: Generates a proper directory structure with multiple files organized in a project layout
- **"single_file"**: Outputs all code in a single consolidated file

For complex projects, "multiple_files" is recommended as it produces a more maintainable code structure.

### Project Structure Directives

These directives define the structure and organization of the generated code:

| Directive | Description | Example |
|-----------|-------------|---------|
| `@pattern` | Specifies design patterns to use | `@pattern "Observer pattern for task status updates"` |
| `@structure` | Defines the file structure | `@structure "include/scheduler_core.hpp: Core scheduler interface"` |

### Content Definition Directives

Core directives that define the project's content:

| Directive | Description | Example |
|-----------|-------------|---------|
| `@language` | Specifies the programming language | `@language "C++"` |
| `@description` | Describes what to implement | `@description "implement a thread-safe queue"` |
| `@context` | Provides context for implementation | `@context "Modern C++20 implementation"` |
| `@dependency` | Specifies project dependencies | `@dependency "Networking library (e.g., Boost.Asio)"` |
| `@test` | Defines test cases | `@test "Test concurrent access with 100 threads"` |
| `@security` | Defines security requirements | `@security "Secure communication with JWT"` |
| `@performance` | Specifies performance requirements | `@performance "Handle 10,000 requests/second"` |
| `@copyright` | Specifies copyright information | `@copyright "MIT License" "2025 CQL Project"` |
| `@architecture` | Defines architectural guidelines | `@architecture "Microservices-based design"` |
| `@constraint` | Specifies implementation constraints | `@constraint "No dynamic memory allocation"` |
| `@complexity` | Defines complexity requirements | `@complexity "O(log n) lookup time"` |
| `@format` | Specifies formatting requirements | `@format "Google C++ style guide"` |
| `@variable` | Defines a template variable | `@variable "container_type=vector"` |
| `@example` | Provides an example of usage | `@example "MyClass obj; obj.process();"` |

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

## Compiler Components

CQL's compiler system consists of three main components that work together to transform CQL source code into structured queries for language models.

### Lexer (lexer.hpp)

The lexer is responsible for breaking down raw CQL source code into tokens that can be processed by the parser.

**File: `include/cql/lexer.hpp`**

#### TokenType Enumeration

The lexer recognizes various token types that represent CQL directives and constructs:

```cpp
enum class TokenType {
    // Directive tokens
    LANGUAGE,       // @language
    DESCRIPTION,    // @description
    CONTEXT,        // @context
    TEST,           // @test
    DEPENDENCY,     // @dependency
    PERFORMANCE,    // @performance
    COPYRIGHT,      // @copyright
    ARCHITECTURE,   // @architecture
    CONSTRAINT,     // @constraint
    EXAMPLE,        // @example
    SECURITY,       // @security
    COMPLEXITY,     // @complexity
    MODEL,          // @model
    FORMAT,         // @format
    VARIABLE,       // @variable
    OUTPUT_FORMAT,  // @output_format
    MAX_TOKENS,     // @max_tokens
    TEMPERATURE,    // @temperature
    PATTERN,        // @pattern
    STRUCTURE,      // @structure
    
    // Basic tokens
    IDENTIFIER,     // any text
    STRING,         // "quoted text"
    NEWLINE,        // \n
    END             // end of input
};
```

#### Token Structure

Each token carries type, value, and location information for error reporting:

```cpp
struct Token {
    TokenType m_type;      // The type of token
    std::string m_value;   // The token's text value
    size_t m_line;         // Line number in source
    size_t m_column;       // Column number in source
    
    Token(TokenType t, std::string v, size_t l, size_t c);
    [[nodiscard]] std::string to_string() const;
};
```

#### Lexer Class

The main lexer class processes input character by character:

```cpp
class Lexer {
public:
    explicit Lexer(std::string_view input);
    
    // Main tokenization method
    std::optional<Token> next_token();
    
    // Location accessors for error reporting
    [[nodiscard]] size_t current_line() const;
    [[nodiscard]] size_t current_column() const;

private:
    std::string_view m_input;  // Input text being processed
    size_t m_current;          // Current position in input
    size_t m_line;             // Current line number
    size_t m_column;           // Current column number
    
    // Internal tokenization methods
    void advance();                            // Move to next character
    void skip_whitespace();                    // Skip spaces and tabs
    void skip_trailing_whitespace();          // Skip trailing whitespace
    std::optional<Token> lex_keyword();       // Parse @directive tokens
    std::optional<Token> lex_string();        // Parse quoted strings
    std::optional<Token> lex_identifier();    // Parse unquoted text
};
```

#### Error Handling

The lexer provides detailed error information:

```cpp
class LexerError : public std::runtime_error {
public:
    LexerError(const std::string& message, size_t line, size_t column);
    
    [[nodiscard]] size_t line() const;
    [[nodiscard]] size_t column() const;
};
```

**Example Lexer Usage:**
```cpp
cql::Lexer lexer("@language \"C++\"\n@description \"thread-safe queue\"");
while (auto token = lexer.next_token()) {
    if (token->m_type == cql::TokenType::END) break;
    std::cout << token->to_string() << std::endl;
}
```

### Parser (parser.hpp)

The parser takes tokens from the lexer and builds an Abstract Syntax Tree (AST) of QueryNode objects.

**File: `include/cql/parser.hpp`**

#### Error Codes

The parser uses standardized error codes for consistent error reporting:

```cpp
namespace parser_errors {
    // General parsing errors
    inline constexpr char GENERAL_ERROR[] = "PAR-001";
    
    // Token-related errors (100-199)
    inline constexpr char UNEXPECTED_TOKEN[] = "PAR-101";
    inline constexpr char MISSING_TOKEN[] = "PAR-102";
    inline constexpr char UNEXPECTED_END[] = "PAR-103";
    
    // Syntax-related errors (200-299)
    inline constexpr char INVALID_DIRECTIVE[] = "PAR-201";
    inline constexpr char INVALID_STRING[] = "PAR-202";
    
    // Structure-related errors (300-399)
    inline constexpr char INVALID_STRUCTURE[] = "PAR-301";
}
```

#### Parser Class

The main parser class converts token streams into AST nodes:

```cpp
class Parser {
public:
    explicit Parser(std::string_view input);
    
    // Main parsing method - returns vector of QueryNode objects
    std::vector<std::unique_ptr<QueryNode>> parse();

private:
    Lexer m_lexer;                    // Lexer for tokenization
    std::optional<Token> m_current_token;  // Current token being processed
    
    // Token management
    void advance();                   // Move to next token
    std::string parse_string();       // Parse string token value
    
    // Node parsing methods - one for each directive type
    std::unique_ptr<QueryNode> parse_code_request();
    std::unique_ptr<QueryNode> parse_context();
    std::unique_ptr<QueryNode> parse_test();
    std::unique_ptr<QueryNode> parse_dependency();
    std::unique_ptr<QueryNode> parse_performance();
    std::unique_ptr<QueryNode> parse_copyright();
    std::unique_ptr<QueryNode> parse_architecture();
    std::unique_ptr<QueryNode> parse_constraint();
    std::unique_ptr<QueryNode> parse_example();
    std::unique_ptr<QueryNode> parse_security();
    std::unique_ptr<QueryNode> parse_complexity();
    std::unique_ptr<QueryNode> parse_model();
    std::unique_ptr<QueryNode> parse_format();
    std::unique_ptr<QueryNode> parse_variable();
    std::unique_ptr<QueryNode> parse_output_format();
    std::unique_ptr<QueryNode> parse_max_tokens();
    std::unique_ptr<QueryNode> parse_temperature();
    std::unique_ptr<QueryNode> parse_pattern();
    std::unique_ptr<QueryNode> parse_structure();
};
```

#### Parser Error Class

Enhanced error reporting with location and error codes:

```cpp
class ParserError : public std::runtime_error {
public:
    ParserError(const std::string& message, size_t line, size_t column,
                std::string error_code = parser_errors::GENERAL_ERROR);
    
    [[nodiscard]] size_t line() const;
    [[nodiscard]] size_t column() const;
    [[nodiscard]] const std::string& error_code() const;
    [[nodiscard]] std::string formatted_message() const;
};
```

**Example Parser Usage:**
```cpp
try {
    cql::Parser parser("@language \"C++\"\n@description \"thread-safe queue\"");
    auto nodes = parser.parse();
    
    // Process the AST nodes
    for (const auto& node : nodes) {
        // Use visitor pattern to process each node
        node->accept(visitor);
    }
} catch (const cql::ParserError& e) {
    std::cerr << "Parse error: " << e.formatted_message() << std::endl;
}
```

### Compiler (compiler.hpp)

The compiler implements the Visitor pattern to traverse the AST and generate formatted query text suitable for language models.

**File: `include/cql/compiler.hpp`**

#### QueryCompiler Class

The main compiler class that transforms AST nodes into structured queries:

```cpp
class QueryCompiler final : public QueryVisitor {
public:
    // Visitor interface implementation - one method per node type
    void visit(const CodeRequestNode& node) override;
    void visit(const ContextNode& node) override;
    void visit(const TestNode& node) override;
    void visit(const DependencyNode& node) override;
    void visit(const PerformanceNode& node) override;
    void visit(const CopyrightNode& node) override;
    void visit(const ArchitectureNode& node) override;
    void visit(const ConstraintNode& node) override;
    void visit(const ExampleNode& node) override;
    void visit(const SecurityNode& node) override;
    void visit(const ComplexityNode& node) override;
    void visit(const ModelNode& node) override;
    void visit(const FormatNode& node) override;
    void visit(const VariableNode& node) override;
    void visit(const OutputFormatNode& node) override;
    void visit(const MaxTokensNode& node) override;
    void visit(const TemperatureNode& node) override;
    void visit(const PatternNode& node) override;
    void visit(const StructureNode& node) override;
    
    // Output generation methods
    [[nodiscard]] std::string get_compiled_query() const;
    void print_compiled_query(std::ostream& out = std::cout) const;

private:
    // Organized output content
    std::map<std::string, std::string> m_result_sections;     // Main content sections
    std::vector<std::string> m_test_cases;                    // Test requirements
    std::vector<std::pair<std::string, std::string>> m_examples;  // Code examples
    
    // Configuration for output generation
    std::string m_target_model = "claude-3-opus";             // Target LLM model
    std::string m_output_format = "markdown";                 // Output format
    std::string m_api_output_format;                          // API output format
    std::string m_max_tokens;                                 // Token limit
    std::string m_temperature;                                // Temperature setting
    
    // Template variable support
    std::map<std::string, std::string> m_variables;           // Variable substitutions
    
    // Variable interpolation method
    [[nodiscard]] std::string interpolate_variables(const std::string& input) const;
};
```

**Compilation Process:**

1. **AST Traversal**: The compiler visits each node in the AST using the visitor pattern
2. **Content Organization**: Each node type contributes to specific sections of the output
3. **Variable Interpolation**: Template variables (${variable_name}) are replaced with their values
4. **Output Formatting**: The final query is formatted according to the specified output format (markdown, JSON, etc.)

**Example Compiler Usage:**
```cpp
// Parse CQL source into AST
cql::Parser parser(cql_source);
auto nodes = parser.parse();

// Create compiler and process AST
cql::QueryCompiler compiler;
for (const auto& node : nodes) {
    node->accept(compiler);
}

// Get the compiled query
std::string compiled_query = compiler.get_compiled_query();

// Submit to language model or save to file
std::cout << compiled_query << std::endl;
```

**Variable Interpolation Example:**
```cpp
// CQL source with variables
std::string cql_source = R"(
@variable "container_type=vector"
@variable "element_type=int"
@description "Implement a ${container_type} of ${element_type}"
)";

// After compilation, the description becomes:
// "Implement a vector of int"
```

The compiler system provides a complete pipeline from raw CQL source code to structured queries that can be submitted to language models for code generation. Each component is designed to be modular and extensible, supporting future enhancements to the CQL language.

## Utility Components

### Project Utilities (project_utils.hpp)

The project utilities provide essential infrastructure for logging, random number generation, threading, and platform-specific operations.

**File: `include/cql/project_utils.hpp`**

#### Version Information

```cpp
#define PROJECT_VERSION_MAJOR 1
#define PROJECT_VERSION_MINOR 0
```

#### Utility Macros

Non-copyable and non-moveable class declarations:

```cpp
#define DECLARE_NON_COPYABLE(ClassType) \
    ClassType(const ClassType&) = delete; \
    ClassType& operator=(const ClassType&) = delete

#define DECLARE_NON_MOVEABLE(ClassType) \
    ClassType(ClassType&) = delete; \
    ClassType& operator=(ClassType&) = delete
```

#### Thread Utilities

Thread ID to string conversion:

```cpp
template<typename ThreadType = std::thread::id>
inline std::string threadIdToString(ThreadType thread_id = std::this_thread::get_id()) {
    std::stringstream ss;
    ss << thread_id;
    return ss.str();
}
```

#### Random Number Generation

Thread-safe random number generator:

```cpp
class RandomGenerator {
public:
    RandomGenerator(const int min, const int max);
    int getNumber();
    
    // Non-copyable and non-moveable
    DECLARE_NON_COPYABLE(RandomGenerator);
    DECLARE_NON_MOVEABLE(RandomGenerator);

private:
    std::mt19937 m_gen;
    std::uniform_int_distribution<int> m_dist;
};
```

#### Logging System

Comprehensive logging with multiple levels and thread safety:

```cpp
enum class LogLevel {
    INFO,     // General information
    NORMAL,   // Normal operation messages
    DEBUG,    // Debug information
    ERROR,    // Error conditions
    CRITICAL  // Critical failures
};

class Logger {
public:
    // Singleton access
    static Logger& getInstance();
    static Logger& getInstance(const std::string& custom_path);
    static std::shared_ptr<Logger> getInstancePtr();
    static std::shared_ptr<Logger> getInstancePtr(const std::string& custom_path);
    
    // Logging methods with variadic templates
    template<typename... Args>
    void log(const LogLevel level, const Args&... args);
    
    template<typename... Args>
    void log_with_depth(const LogLevel level, const int depth, const Args&... args);
    
    // Level control
    void setLevelEnabled(LogLevel level, const bool enabled);
    void setToLevelEnabled(LogLevel debug_level);
    bool isLevelEnabled(const LogLevel level) const;
    
    // Output control
    void disableStderr();
    void enableStderr();
    bool isStderrEnabled() const;
    void setFileOutputEnabled(bool enabled);
    bool isFileOutputEnabled() const;
    
    // RAII stderr suppression
    class StderrSuppressionGuard {
    public:
        StderrSuppressionGuard();
        ~StderrSuppressionGuard();
    private:
        bool m_was_enabled;
    };

private:
    std::ofstream m_log_file;
    std::mutex m_mutex;
    std::atomic<bool> m_stderr_enabled{true};
    std::atomic<bool> m_file_output_enabled{true};
    std::atomic<bool> m_enabled_levels[5];  // One per LogLevel
    
    // Internal helper methods
    void write_log_message(const LogLevel level, const std::string& message);
    static std::stringstream create_log_prefix(LogLevel level);
    static std::string log_level_to_string(const LogLevel level);
    static std::string get_utc_timestamp();
    static std::string getIndentation(const int depth);
};
```

#### Clipboard Operations

Platform-specific clipboard functionality:

```cpp
namespace clipboard {
    /**
     * Copy text to the system clipboard
     * @param text Text to copy to clipboard
     * @return True if the operation was successful
     */
    bool copy_to_clipboard(const std::string& text);
    
    /**
     * Get text from the system clipboard
     * @return Clipboard content as string
     */
    std::string get_from_clipboard();
}
```

**Example Logger Usage:**
```cpp
#include "cql/project_utils.hpp"

int main() {
    // Get logger instance
    auto& logger = Logger::getInstance("./cql.log");
    
    // Configure logging levels
    logger.setToLevelEnabled(LogLevel::DEBUG);  // Show DEBUG and above
    
    // Log various levels
    logger.log(LogLevel::INFO, "Starting CQL compiler");
    logger.log(LogLevel::DEBUG, "Processing file: ", filename);
    logger.log(LogLevel::ERROR, "Failed to parse: ", error_message);
    
    // Use depth for hierarchical logging
    logger.log_with_depth(LogLevel::DEBUG, 1, "  Parsing directive: @language");
    logger.log_with_depth(LogLevel::DEBUG, 2, "    Found value: C++");
    
    // Temporarily suppress stderr
    {
        Logger::StderrSuppressionGuard guard;
        logger.log(LogLevel::ERROR, "This won't go to stderr");
    }
    
    return 0;
}
```

### Test Utilities (test_utils.hpp)

Provides testing infrastructure with Google Test integration and legacy macro support.

**File: `include/cql/test_utils.hpp`**

#### Google Test Integration

Conditional compilation for Google Test support:

```cpp
#ifdef CQL_TESTING
  #include <gtest/gtest.h>
  #define TEST_ASSERT_GTEST(condition, message) ASSERT_TRUE(condition) << message
  #define TEST_ASSERT_MESSAGE_GTEST(condition, message) ASSERT_TRUE(condition) << message
#else
  #define TEST_ASSERT_GTEST(condition, message) do { if(!(condition)) { std::cerr << message; } } while(false)
  #define TEST_ASSERT_MESSAGE_GTEST(condition, message) do { if(!(condition)) { std::cerr << message; } } while(false)
#endif
```

#### Legacy Test Macros

Backward compatibility macros that create proper TestResult returns:

```cpp
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            return cql::test::TestResult::fail(message, __FILE__, __LINE__); \
        } \
    } while (false)

#define TEST_ASSERT_MESSAGE(condition, message) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << message; \
            return cql::test::TestResult::fail(ss.str(), __FILE__, __LINE__); \
        } \
    } while (false)
```

#### Test Result Printing

Formatted test output compatible with Google Test style:

```cpp
namespace cql::test {
    inline void print_test_result(const std::string& test_name, const TestResult& result) {
        if (result.passed()) {
            std::cout << "[       OK ] " << test_name << std::endl;
        } else {
            std::cout << "[  FAILED  ] " << test_name << std::endl;
            std::cout << "  Error: " << result.get_error_message() << std::endl;
            if (!result.get_file_name().empty()) {
                std::cout << "  Location: " << result.get_file_name() 
                         << ":" << result.get_line_number() << std::endl;
            }
        }
    }
}
```

**Example Test Usage:**
```cpp
#include "cql/test_utils.hpp"
#include "cql/cql.hpp"

cql::test::TestResult test_lexer() {
    cql::Lexer lexer("@language \"C++\"");
    auto token = lexer.next_token();
    
    TEST_ASSERT(token.has_value(), "Expected token but got none");
    TEST_ASSERT_MESSAGE(token->m_type == cql::TokenType::LANGUAGE, 
                       "Expected LANGUAGE token, got " << static_cast<int>(token->m_type));
    
    return cql::test::TestResult::pass();
}

int main() {
    auto result = test_lexer();
    cql::test::print_test_result("test_lexer", result);
    return result.passed() ? 0 : 1;
}
```

## Network and Testing Components

### Mock Server (mock_server.hpp)

Provides a lightweight HTTP mock server for testing API client implementations without making actual network requests.

**File: `include/cql/mock_server.hpp`**

#### MockServer Class

A simple HTTP server for testing purposes:

```cpp
class MockServer {
public:
    /**
     * Constructor that starts the server on the specified port
     * @param port Port number to listen on (default: 8080)
     */
    explicit MockServer(int port = 8080);
    
    /**
     * Destructor that stops the server
     */
    ~MockServer();
    
    /**
     * Add a response handler for a specific endpoint
     * @param endpoint The endpoint path (e.g., "/v1/messages")
     * @param handler Function that takes a request body and returns a response
     */
    void add_handler(const std::string& endpoint, 
                    std::function<std::string(const std::string&)> handler);
    
    /**
     * Set a default response for all unhandled endpoints
     * @param response The default response to return
     */
    void set_default_response(const std::string& response);
    
    /**
     * Start the mock server
     */
    void start();
    
    /**
     * Stop the mock server
     */
    void stop();
    
    /**
     * Get the base URL of the mock server
     * @return The base URL as a string
     */
    std::string get_url() const;
    
    /**
     * Get a list of received requests
     * @return Vector of request bodies
     */
    std::vector<std::string> get_requests() const;
    
    /**
     * Check if the server is running
     * @return true if running, false otherwise
     */
    bool is_running() const;
    
private:
    int m_port;
    std::atomic<bool> m_running;
    std::thread m_server_thread;
    std::map<std::string, std::function<std::string(const std::string&)>> m_handlers;
    std::string m_default_response;
    std::vector<std::string> m_requests;
    mutable std::mutex m_mutex;
    
    void run_server() const;
};
```

#### Response Generation Utilities

Helper functions for creating mock API responses:

```cpp
/**
 * Create a mock response for the Claude API
 * @param content The content to include in the response
 * @return JSON response string mimicking the Claude API
 */
std::string create_mock_claude_response(const std::string& content);

/**
 * Create a mock error response
 * @param status_code HTTP status code
 * @param error_type Error type
 * @param error_message Error message
 * @return JSON error response string
 */
std::string create_mock_error_response(int status_code, 
                                      const std::string& error_type,
                                      const std::string& error_message);
```

**Example Mock Server Usage:**
```cpp
#include "cql/mock_server.hpp"
#include "cql/api_client.hpp"

int main() {
    // Create and configure mock server
    cql::test::MockServer server(8080);
    
    // Add handler for Claude API endpoint
    server.add_handler("/v1/messages", [](const std::string& request_body) {
        return cql::test::create_mock_claude_response(
            "```cpp\nclass ThreadSafeQueue {\n    // Implementation\n};\n```"
        );
    });
    
    // Add error handler
    server.add_handler("/v1/error_test", [](const std::string& request_body) {
        return cql::test::create_mock_error_response(
            429, "rate_limit_error", "Too many requests"
        );
    });
    
    // Start server
    server.start();
    
    // Test your API client against the mock server
    cql::Config config;
    config.set_api_url(server.get_url());  // Point to mock server
    cql::ApiClient client(config);
    
    auto response = client.submit_query("test query");
    
    // Verify the request was received
    auto requests = server.get_requests();
    assert(!requests.empty());
    
    server.stop();
    return 0;
}
```

### Response Processor (response_processor.hpp)

Processes API responses from language models, extracting code blocks and organizing them into files.

**File: `include/cql/response_processor.hpp`**

#### CodeBlock Structure

Represents a code block extracted from an API response:

```cpp
struct CodeBlock {
    std::string m_language_tag;    // Programming language (e.g., "cpp", "python")
    std::string m_content;         // Content of the code block
    std::string m_context_before;  // Text context before the code block
    std::string m_context_after;   // Text context after the code block
    bool m_is_test;                // Whether this is a test code block
    std::string m_filename_hint;   // Hint for the filename (if found in context)
};
```

#### ResponseProcessor Class

Main class for processing language model responses:

```cpp
class ResponseProcessor {
public:
    /**
     * Constructor
     * @param config Configuration for the response processor
     */
    explicit ResponseProcessor(const Config& config);
    
    /**
     * Process an API response and extract code blocks
     * @param response_text The raw response text to process
     * @return Vector of generated files
     */
    [[nodiscard]] std::vector<GeneratedFile> process_response(const std::string& response_text);
    
    /**
     * Set the output directory for generated files
     * @param directory The directory path
     */
    void set_output_directory(const std::string& directory);
    
    /**
     * Set whether to overwrite existing files
     * @param overwrite Whether to overwrite existing files
     */
    void set_overwrite_existing(bool overwrite);
    
    /**
     * Set whether to create missing directories
     * @param create Whether to create missing directories
     */
    void set_create_directories(bool create);

private:
    // Code block extraction and processing
    [[nodiscard]] static std::vector<CodeBlock> extract_code_blocks(const std::string& response_text);
    [[nodiscard]] static std::vector<GeneratedFile> organize_code_blocks(const std::vector<CodeBlock>& blocks);
    
    // Filename generation
    [[nodiscard]] static std::string determine_filename(const CodeBlock& block, const std::string& context);
    [[nodiscard]] static std::string extract_filename_hint(const std::string& text);
    [[nodiscard]] static std::string sanitize_filename(const std::string& filename);
    [[nodiscard]] static std::string generate_filename_from_content(const CodeBlock& block, const std::string& key);
    [[nodiscard]] static std::string generate_test_filename(const std::string& impl_name);
    
    // Language processing
    [[nodiscard]] static std::string determine_language(const std::string& language_tag);
    [[nodiscard]] static std::string determine_default_extension(const std::string& language);
    [[nodiscard]] static std::string determine_key_from_content(const CodeBlock& block);
    
    // Configuration and state
    Config m_config;
    std::string m_output_directory;
    bool m_overwrite_existing;
    bool m_create_directories;
};
```

#### File Saving Utility

```cpp
/**
 * Save a generated file to disk
 * @param file The file to save
 * @param directory The directory to save to
 * @param config The configuration for saving
 * @return true if saved successfully, false otherwise
 */
bool save_generated_file(const GeneratedFile& file, const std::string& directory, const Config& config);
```

**Example Response Processing:**
```cpp
#include "cql/response_processor.hpp"

int main() {
    // Create configuration
    cql::Config config;
    config.set_output_format("multiple_files");
    
    // Create response processor
    cql::ResponseProcessor processor(config);
    processor.set_output_directory("./generated");
    processor.set_create_directories(true);
    processor.set_overwrite_existing(false);
    
    // Example API response with code blocks
    std::string api_response = R"(
    Here's a thread-safe queue implementation:
    
    ```cpp
    // thread_safe_queue.hpp
    #pragma once
    #include <queue>
    #include <mutex>
    
    template<typename T>
    class ThreadSafeQueue {
    private:
        std::queue<T> m_queue;
        mutable std::mutex m_mutex;
    public:
        void push(const T& item) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(item);
        }
        
        bool try_pop(T& item) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) return false;
            item = m_queue.front();
            m_queue.pop();
            return true;
        }
    };
    ```
    
    And here are some tests:
    
    ```cpp
    // test_thread_safe_queue.cpp
    #include "thread_safe_queue.hpp"
    #include <gtest/gtest.h>
    #include <thread>
    #include <vector>
    
    TEST(ThreadSafeQueueTest, ConcurrentAccess) {
        ThreadSafeQueue<int> queue;
        std::vector<std::thread> threads;
        
        // Test concurrent push/pop operations
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&queue, i]() {
                queue.push(i);
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        // Verify queue has 10 elements
        int count = 0;
        int value;
        while (queue.try_pop(value)) {
            count++;
        }
        EXPECT_EQ(count, 10);
    }
    ```
    )";
    
    // Process the response
    auto generated_files = processor.process_response(api_response);
    
    // Save files to disk
    for (const auto& file : generated_files) {
        std::cout << "Generated file: " << file.filename << std::endl;
        std::cout << "Language: " << file.language << std::endl;
        std::cout << "Content size: " << file.content.size() << " bytes" << std::endl;
        
        if (cql::save_generated_file(file, "./generated", config)) {
            std::cout << "Saved successfully" << std::endl;
        }
    }
    
    return 0;
}
```

### Pattern Compatibility (pattern_compatibility.hpp)

Manages compatibility relationships between architectural design patterns to ensure coherent system architectures.

**File: `include/cql/pattern_compatibility.hpp`**

#### Pattern Class

Represents an architectural design pattern:

```cpp
class Pattern {
public:
    /**
     * Create a pattern from an architecture node
     * @param node The architecture node containing pattern information
     * @throws std::runtime_error If the node cannot be parsed
     */
    explicit Pattern(const ArchitectureNode& node);
    
    /**
     * Create a pattern manually with explicit parameters
     * @param layer The architectural layer (FOUNDATION, COMPONENT, INTERACTION)
     * @param name The pattern name (e.g., "factory_method", "observer")
     * @param parameters Optional configuration parameters
     */
    Pattern(PatternLayer layer, std::string name, std::string parameters = "");
    
    // Accessors
    [[nodiscard]] PatternLayer get_layer() const;
    [[nodiscard]] const std::string& get_name() const;
    [[nodiscard]] const std::string& get_parameters() const;
    [[nodiscard]] std::string to_string() const;

private:
    PatternLayer m_layer;
    std::string m_name;
    std::string m_parameters;
};
```

#### Compatibility Issue Structure

Represents an incompatibility between patterns:

```cpp
struct CompatibilityIssue {
    std::string message;   // Description of the compatibility issue
    std::string pattern1;  // String representation of the first pattern
    std::string pattern2;  // String representation of the second pattern
    
    CompatibilityIssue(std::string msg, const Pattern& p1, const Pattern& p2);
    [[nodiscard]] std::string to_string() const;
};
```

#### PatternCompatibilityManager Class

Manages compatibility rules for design patterns:

```cpp
class PatternCompatibilityManager {
public:
    /**
     * Create a new pattern compatibility manager with predefined rules
     * Initializes compatibility rules for all 23 standard design patterns
     */
    PatternCompatibilityManager();
    
    /**
     * Check if all patterns in a set are compatible with each other
     * @param patterns The set of patterns to check
     * @return List of compatibility issues (empty if all compatible)
     */
    [[nodiscard]] std::vector<CompatibilityIssue> check_compatibility(
        const std::vector<Pattern>& patterns) const;
    
    /**
     * Check if all architecture nodes represent compatible patterns
     * @param nodes Vector of architecture nodes to check
     * @return List of compatibility issues (empty if all compatible)
     */
    [[nodiscard]] std::vector<CompatibilityIssue> check_compatibility(
        const std::vector<const ArchitectureNode*>& nodes) const;
    
    /**
     * Check if two specific patterns are compatible
     * @param p1 First pattern to check
     * @param p2 Second pattern to check
     * @return true if patterns are compatible, false otherwise
     */
    [[nodiscard]] bool are_patterns_compatible(
        const Pattern& p1, const Pattern& p2) const;

private:
    struct CompatibilityRule {
        std::string pattern_name;
        std::set<std::string> compatible_patterns;
        std::set<std::string> incompatible_patterns;
    };
    
    std::map<std::string, CompatibilityRule> m_compatibility_rules;
    
    // Pattern initialization methods
    void initialize_creational_patterns();    // Factory, Builder, Singleton, etc.
    void initialize_structural_patterns();    // Adapter, Bridge, Composite, etc.
    void initialize_behavioral_patterns();    // Observer, Strategy, Command, etc.
};
```

**Example Pattern Compatibility Usage:**
```cpp
#include "cql/pattern_compatibility.hpp"

int main() {
    // Create pattern compatibility manager
    cql::PatternCompatibilityManager manager;
    
    // Create some patterns
    std::vector<cql::Pattern> patterns = {
        cql::Pattern(cql::PatternLayer::FOUNDATION, "microservices"),
        cql::Pattern(cql::PatternLayer::COMPONENT, "factory_method"),
        cql::Pattern(cql::PatternLayer::COMPONENT, "singleton"),
        cql::Pattern(cql::PatternLayer::INTERACTION, "observer")
    };
    
    // Check compatibility
    auto issues = manager.check_compatibility(patterns);
    
    if (issues.empty()) {
        std::cout << "All patterns are compatible!" << std::endl;
    } else {
        std::cout << "Found compatibility issues:" << std::endl;
        for (const auto& issue : issues) {
            std::cout << "  " << issue.to_string() << std::endl;
        }
    }
    
    // Check specific pattern pair
    cql::Pattern singleton(cql::PatternLayer::COMPONENT, "singleton");
    cql::Pattern factory(cql::PatternLayer::COMPONENT, "factory_method");
    
    if (manager.are_patterns_compatible(singleton, factory)) {
        std::cout << "Singleton and Factory Method are compatible" << std::endl;
    } else {
        std::cout << "Singleton and Factory Method are incompatible" << std::endl;
    }
    
    return 0;
}
```

The utility and testing components provide essential infrastructure for the CQL compiler system, including robust logging, testing frameworks, network mocking capabilities, response processing, and architectural pattern validation. These components ensure that the CQL system is reliable, testable, and maintainable.
