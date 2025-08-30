# CQL to Claude API Integration Roadmap

This document outlines the implementation plan for integrating the Claude Query Language (CQL) compiler with the Anthropic API to create a complete code generation pipeline.

## Overview

The integration will enable the following workflow:

```
CQL Query → Claude API → Generated Code → Local Files
```

This will transform CQL from a query generator into a full code generation solution, allowing developers to:
1. Write structured queries using CQL's directive-based syntax
2. Submit these queries directly to Claude via the Anthropic API
3. Receive generated code in response
4. Save the generated code to local files

## Implementation Plan

### Phase 1: API Client Implementation

#### 1.1. Anthropic API Client
- Create a new `ApiClient` class in `include/cql/api_client.hpp` and `src/cql/api_client.cpp`
- Implement HTTP client functionality using a lightweight library (e.g., `libcurl` or similar)
- Add support for API key configuration through environment variables and config files
- Implement proper error handling and response parsing

#### 1.2. Configuration Management
- Create a new `Config` class to manage API keys, model selection, and other settings
- Support loading configuration from:
  - Environment variables
  - Local config file (`~/.cql/config.json`)
  - Command-line arguments

#### 1.3. Basic API Integration
- Add a new command-line flag `--submit` to send compiled queries to the API
- Implement basic response handling to display the generated code
- Add timeout controls and retry logic

### Phase 2: Response Processing and File Generation

#### 2.1. Response Parser
- Implement a parser to extract code blocks from Claude's responses
- Support multiple file generation from a single response
- Handle different code block formats and languages

#### 2.2. File Output System
- Create a `CodeWriter` class to manage saving generated code to files
- Support configurable output directories
- Implement proper file naming conventions based on content

#### 2.3. Output Formatting
- Add syntax highlighting for console output
- Support generation of complete project structures
- Implement diff view when updating existing files

### Phase 3: Interactive Workflow Enhancement

#### 3.1. Interactive Mode Improvements
- Add API integration to interactive mode
- Implement real-time query submission and code preview
- Add ability to edit and resubmit queries based on results

#### 3.2. Template Integration
- Extend the template system to include API-specific settings
- Add template variables for API parameters
- Support storing and managing successful queries and their responses

#### 3.3. Session Management
- Add conversation history for multi-step interactions
- Implement context preservation between related queries
- Support chat-like interactions for refining generated code

### Phase 4: Advanced Features

#### 4.1. Batch Processing
- Add support for processing multiple CQL files in a batch
- Implement parallel request handling for efficiency
- Add result aggregation and reporting

#### 4.2. Project Integration
- Support integrating generated code into existing projects
- Add smart insertion of generated components
- Implement project structure recognition

#### 4.3. Version Control Integration
- Add automatic git commit generation for new files
- Support creating branches for generated code
- Implement PR generation for review

## Technical Requirements

### API Integration Requirements
- Secure API key management
- Proper rate limiting and error handling
- Support for different Claude models (claude-3-opus, claude-3-sonnet, etc.)
- Timeout handling and graceful failure modes

### Code Generation Requirements
- Multiple file generation and organization
- Support for generated tests alongside implementations
- Proper extraction of code blocks from responses
- Language-specific formatting and organization

### User Experience Requirements
- Clear progress indicators during API calls
- Helpful error messages for API failures
- Command-line options for all common operations
- Integration with existing CQL template system

## Command-Line Interface

The following new CLI options will be added:

```bash
# Submit a CQL query to the Claude API and display the result
cql --submit input.cql

# Submit and save the generated code to output files
cql --submit input.cql --output-dir ./output

# Submit with specific model selection
cql --submit input.cql --model claude-3-opus

# Use a template, submit to API, and save results
cql --template thread_safe_queue collection_type=stack --submit --output-dir ./output

# Interactive mode with API integration
cql --interactive --api-enabled
```

## Configuration File Structure

A new configuration file will be supported at `~/.cql/config.json`:

```json
{
  "api": {
    "key": "your_api_key_here",
    "model": "claude-3-opus",
    "timeout": 30,
    "max_retries": 3
  },
  "output": {
    "default_directory": "~/generated_code",
    "create_missing_dirs": true,
    "overwrite_existing": false
  },
  "templates": {
    "directories": ["~/.cql/templates", "~/project/templates"]
  }
}
```

## Implementation Timeline

1. **Phase 1:** Basic API client and configuration management (2-3 weeks)
2. **Phase 2:** Response processing and file generation (2-3 weeks)
3. **Phase 3:** Interactive workflow enhancements (3-4 weeks)
4. **Phase 4:** Advanced features (4-6 weeks)

Total estimated timeline: 3-4 months for complete implementation.

## Getting Started with Implementation

### First Steps
1. Review the Anthropic API documentation
2. Set up the basic API client structure
3. Implement configuration management
4. Create a simple end-to-end test with a basic query

### Sample Implementation Code

```cpp
// Example API client implementation with custom logger

// ApiClient.hpp
class ApiClient {
public:
    explicit ApiClient(const Config& config) {
        Logger::getInstance().log(LogLevel::INFO, "Initializing API client with model: ", config.get_model());
        // Initialize HTTP client, set API key, etc.
    }
    
    ApiResponse submit_query(const std::string& query) {
        Logger::getInstance().log(LogLevel::INFO, "Submitting query to Claude API");
        
        try {
            // Prepare request
            // Send to API
            // Process response
            
            Logger::getInstance().log(LogLevel::INFO, "Query processed successfully");
            return response;
        } catch (const std::exception& e) {
            Logger::getInstance().log(LogLevel::ERROR, "API request failed: ", e.what());
            ApiResponse error_response;
            error_response.success = false;
            error_response.error_message = e.what();
            return error_response;
        }
    }
};

// Example usage in CLI
void handle_submit_command(const std::string& input_file, const std::string& output_dir) {
    try {
        // Load configuration
        Config config = Config::load_from_default_locations();
        
        // Load and compile the CQL query
        std::string query_content = util::read_file(input_file);
        std::string compiled_query = QueryProcessor::compile(query_content);
        
        Logger::getInstance().log(LogLevel::INFO, "Compiled query from ", input_file);
        
        // Create API client and submit
        ApiClient api_client(config);
        ApiResponse response = api_client.submit_query(compiled_query);
        
        if (!response.success) {
            Logger::getInstance().log(LogLevel::ERROR, "API request failed: ", response.error_message);
            return;
        }
        
        // Process generated files
        for (const auto& file : response.generated_files) {
            std::string output_path = output_dir + "/" + file.filename;
            Logger::getInstance().log(LogLevel::INFO, "Writing generated file to ", output_path);
            util::write_file(output_path, file.content);
        }
        
        Logger::getInstance().log(LogLevel::INFO, "Generated ", response.generated_files.size(), " files");
        
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error processing submission: ", e.what());
    }
}
```

### Dependencies to Add
- HTTP client library (libcurl, cpp-httplib, or similar)
- JSON parsing library (nlohmann/json or similar)
- Optional: CLI argument parser enhancements

## Conclusion

This integration will transform CQL from a query generation tool into a complete code generation solution. By connecting directly to the Claude API, we enable a seamless workflow from structured query to implementation code, significantly improving the developer experience and standardizing interactions with AI code generation.
