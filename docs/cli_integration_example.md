# CQL API Integration Command-Line Interface Example

This document provides examples of how the CQL API integration would be used in the command-line interface.

## Command-Line Options

The following new command-line options will be added to the CQL CLI:

```
Usage: cql [options] [input_file] [output_file]

API Integration Options:
  --submit                 Submit the compiled query to the Claude API
  --model <model_name>     Specify the Claude model to use (default: claude-3-opus)
  --output-dir <directory> Directory to save generated code files
  --overwrite              Overwrite existing files without prompting
  --create-dirs            Create missing directories for output files
  --no-save                Display generated code but don't save to files
  --interactive-api        Enable API integration in interactive mode
  --timeout <seconds>      API request timeout in seconds (default: 60)
  --retry <count>          Number of retries for failed API requests (default: 3)
```

## Example Usage

### Basic API Submission

```bash
# Compile a query and submit it to the Claude API
$ cql --submit my_query.cql

Compiling query from my_query.cql...
Submitting to Claude API (model: claude-3-opus)...
API request successful
Generated 3 files:
  - thread_safe_queue.hpp
  - thread_safe_queue.cpp
  - thread_safe_queue_test.cpp

Files saved to current directory
```

### Customizing Output Directory

```bash
# Specify an output directory for generated files
$ cql --submit my_query.cql --output-dir ./generated

Compiling query from my_query.cql...
Submitting to Claude API (model: claude-3-opus)...
API request successful
Generated 3 files:
  - thread_safe_queue.hpp
  - thread_safe_queue.cpp
  - thread_safe_queue_test.cpp

Files saved to ./generated
```

### Using Templates with API

```bash
# Use a template, customize variables, and submit to API
$ cql --template thread_safe_queue collection_type=stack language=C++ --submit --output-dir ./output

Template 'thread_safe_queue' loaded and instantiated with variables:
  - collection_type = "stack"
  - language = "C++"

Compiling template...
Submitting to Claude API (model: claude-3-opus)...
API request successful
Generated 3 files:
  - thread_safe_stack.hpp
  - thread_safe_stack.cpp
  - thread_safe_stack_test.cpp

Files saved to ./output
```

### CLI Implementation Example

Here's an example of how the CLI processing code might look:

```cpp
bool cql::cli::process_submit_command(const std::string& input_file, 
                                     const std::string& output_dir,
                                     const std::string& model,
                                     bool overwrite,
                                     bool create_dirs) {
    try {
        Logger::getInstance().log(LogLevel::INFO, "Processing file: ", input_file);
        
        // Load the configuration
        Config config = Config::load_from_default_locations();
        
        // Override with command-line arguments
        if (!model.empty()) {
            config.set_model(model);
            Logger::getInstance().log(LogLevel::INFO, "Using model: ", model);
        }
        
        if (!output_dir.empty()) {
            config.set_output_directory(output_dir);
            Logger::getInstance().log(LogLevel::INFO, "Output directory: ", output_dir);
        }
        
        config.set_overwrite_existing_files(overwrite);
        config.set_create_missing_directories(create_dirs);
        
        // Compile the query
        std::string query_content = util::read_file(input_file);
        std::string compiled_query = QueryProcessor::compile(query_content);
        
        Logger::getInstance().log(LogLevel::INFO, "Query compiled successfully");
        
        // Create API client
        ApiClient api_client(config);
        
        // Submit the query
        Logger::getInstance().log(LogLevel::INFO, "Submitting to Claude API...");
        ApiResponse response = api_client.submit_query(compiled_query);
        
        if (!response.success) {
            Logger::getInstance().log(LogLevel::ERROR, "API request failed: ", response.error_message);
            return false;
        }
        
        Logger::getInstance().log(LogLevel::INFO, "API request successful");
        
        // Process the response
        ResponseProcessor processor(config);
        std::vector<GeneratedFile> files = processor.process_response(response.raw_response);
        
        Logger::getInstance().log(LogLevel::INFO, "Generated ", files.size(), " files:");
        
        // Save generated files
        for (const auto& file : files) {
            Logger::getInstance().log(LogLevel::INFO, "- ", file.filename);
            
            // Save file if not using --no-save option
            if (!config.no_save_mode()) {
                save_generated_file(file, config.get_output_directory());
            }
        }
        
        if (config.no_save_mode()) {
            Logger::getInstance().log(LogLevel::INFO, "Files not saved (--no-save option used)");
        } else if (!output_dir.empty()) {
            Logger::getInstance().log(LogLevel::INFO, "Files saved to ", output_dir);
        } else {
            Logger::getInstance().log(LogLevel::INFO, "Files saved to current directory");
        }
        
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error processing submit command: ", e.what());
        return false;
    }
}
```

## Interactive Mode with API Integration

The interactive mode will also be enhanced with API integration capabilities:

```
> help

Commands:
  help                    - Show this help
  exit/quit               - Exit the program
  clear                   - Clear the current query
  show                    - Show the current query
  compile                 - Compile the current query
  load FILE               - Load query from file
  save FILE               - Save compiled query to file
  submit                  - Submit query to Claude API
  submit --model MODEL    - Submit with specific model
  submit --output DIR     - Submit and save to directory
  
Template Commands:
  templates               - List all available templates
  template save NAME      - Save current query as a template
  template load NAME      - Load a template
  [... other template commands ...]
```

### Interactive Mode Example

```
> @language "C++"
> @description "implement a thread-safe queue with a maximum size"
> @context "Using C++20 features and RAII principles"
> @test "Test concurrent push operations"

> compile
=== Compiled Query ===

Please generate C++ code that:
implement a thread-safe queue with a maximum size

Context:
- Using C++20 features and RAII principles

Please include tests for the following cases:
- Test concurrent push operations

Quality Assurance Requirements:
- All code must be well-documented with comments
- Follow modern C++ best practices
- Ensure proper error handling
- Optimize for readability and maintainability
===================

> submit
Submitting to Claude API (model: claude-3-opus)...
API request successful

Generated 3 files:
- thread_safe_queue.hpp
- thread_safe_queue.cpp
- thread_safe_queue_test.cpp

Files saved to current directory

> submit --output ./generated --model claude-3-sonnet
Submitting to Claude API (model: claude-3-sonnet)...
API request successful

Generated 3 files:
- thread_safe_queue.hpp
- thread_safe_queue.cpp
- thread_safe_queue_test.cpp

Files saved to ./generated
```

## Implementation in main.cpp

Here's how these options could be integrated into the main entry point:

```cpp
int main(int argc, char* argv[]) {
    Logger::getInstance().log(LogLevel::INFO, "CQL starting");
    
    // Process command-line arguments
    std::string input_file;
    std::string output_file;
    std::string output_dir;
    std::string model;
    bool run_tests = false;
    bool show_examples = false;
    bool interactive_mode = false;
    bool submit_mode = false;
    bool overwrite_existing = false;
    bool create_dirs = false;
    bool no_save = false;
    bool interactive_api = false;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--test") {
            run_tests = true;
        } else if (arg == "--examples") {
            show_examples = true;
        } else if (arg == "--interactive") {
            interactive_mode = true;
        } else if (arg == "--interactive-api") {
            interactive_mode = true;
            interactive_api = true;
        } else if (arg == "--submit") {
            submit_mode = true;
        } else if (arg == "--model" && i + 1 < argc) {
            model = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (arg == "--overwrite") {
            overwrite_existing = true;
        } else if (arg == "--create-dirs") {
            create_dirs = true;
        } else if (arg == "--no-save") {
            no_save = true;
        } else if (arg.substr(0, 2) != "--") {
            if (input_file.empty()) {
                input_file = arg;
            } else if (output_file.empty()) {
                output_file = arg;
            }
        }
    }
    
    // Execute based on command-line arguments
    if (run_tests) {
        // Run tests
        Tests::run_all_tests();
    } else if (show_examples) {
        // Show examples
        Examples::show_all();
    } else if (interactive_mode) {
        // Run interactive mode with or without API integration
        cli::run_interactive(interactive_api);
    } else if (submit_mode) {
        // Submit mode - process input file and send to API
        if (input_file.empty()) {
            Logger::getInstance().log(LogLevel::ERROR, "No input file specified for --submit");
            return 1;
        }
        
        bool success = cli::process_submit_command(input_file, output_dir, model, 
                                                  overwrite_existing, create_dirs);
        return success ? 0 : 1;
    } else if (!input_file.empty()) {
        // Process input file normally (without API submission)
        bool success = cli::process_file(input_file, output_file);
        return success ? 0 : 1;
    } else {
        // Default: run tests and examples
        Tests::run_all_tests();
        Examples::show_all();
    }
    
    return 0;
}
```

## Configuration Management

The CLI also supports configuration management for API settings:

```bash
# Set API key in environment
$ export CQL_API_KEY="your-api-key-here"

# Or use a config file (~/.cql/config.json)
$ cat ~/.cql/config.json
{
  "api": {
    "key": "your-api-key-here",
    "model": "claude-3-opus",
    "timeout": 60,
    "max_retries": 3
  },
  "output": {
    "default_directory": "~/generated_code",
    "create_missing_dirs": true,
    "overwrite_existing": false
  }
}

# Command-line options override configuration
$ cql --submit my_query.cql --model claude-3-sonnet --overwrite
```
