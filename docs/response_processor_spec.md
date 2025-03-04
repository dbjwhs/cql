# CQL Response Processor Specification

This document specifies the design and implementation details for the Response Processor component, which extracts code blocks from Claude API responses and organizes them into usable files.

## Overview

The Response Processor is responsible for:

1. Parsing Claude's response text
2. Extracting code blocks and their metadata
3. Determining file names and organization
4. Handling multi-file responses
5. Managing tests and implementation files

## Class Structure

```cpp
namespace cql {

class ResponseProcessor {
public:
    explicit ResponseProcessor(const Config& config);
    
    // Primary processing methods
    std::vector<GeneratedFile> process_response(const std::string& response_text);
    
    // Configuration
    void set_output_directory(const std::string& directory);
    void set_overwrite_existing(bool overwrite);
    void set_create_directories(bool create);
    
private:
    // Helper methods
    std::vector<CodeBlock> extract_code_blocks(const std::string& response_text);
    std::vector<GeneratedFile> organize_code_blocks(const std::vector<CodeBlock>& blocks);
    std::string determine_filename(const CodeBlock& block, const std::string& context);
    std::string extract_filename_hint(const std::string& text);
    std::string sanitize_filename(const std::string& filename);
    std::string determine_language(const std::string& language_tag);
    
    // Member variables
    Config m_config;
    std::string m_output_directory;
    bool m_overwrite_existing;
    bool m_create_directories;
};

struct CodeBlock {
    std::string language_tag;
    std::string content;
    std::string context_before;
    std::string context_after;
    bool is_test;
    std::string filename_hint;
};

} // namespace cql
```

## Extracting Code Blocks

The response processor will extract code blocks from Claude's responses using the following approach:

1. Scan for markdown code blocks (```language ... ```)
2. Extract language tags to determine the file type
3. Capture context around the code block to help with filename determination
4. Identify test code vs. implementation code
5. Look for filename hints in comments or context

```cpp
std::vector<CodeBlock> ResponseProcessor::extract_code_blocks(const std::string& response_text) {
    std::vector<CodeBlock> blocks;
    
    Logger::getInstance().log(LogLevel::INFO, "Extracting code blocks from response");
    
    // Regex to match code blocks with language tags
    std::regex code_block_regex("```([a-zA-Z0-9+#]+)\\s*\\n([\\s\\S]*?)\\n```");
    
    auto begin = std::sregex_iterator(response_text.begin(), response_text.end(), code_block_regex);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator it = begin; it != end; ++it) {
        std::smatch match = *it;
        
        // Extract the language tag and code content
        std::string language_tag = match[1].str();
        std::string content = match[2].str();
        
        // Get context before and after the code block
        size_t match_pos = match.position();
        size_t context_length = 200; // Get 200 chars of context
        
        std::string context_before = match_pos > context_length ? 
            response_text.substr(match_pos - context_length, context_length) : 
            response_text.substr(0, match_pos);
            
        size_t match_end = match_pos + match.length();
        std::string context_after = match_end + context_length < response_text.length() ?
            response_text.substr(match_end, context_length) :
            response_text.substr(match_end);
        
        // Determine if this is test code
        bool is_test = language_tag.find("test") != std::string::npos || 
                      content.find("test") != std::string::npos ||
                      context_before.find("test") != std::string::npos;
                      
        // Look for filename hints
        std::string filename_hint = extract_filename_hint(context_before + content + context_after);
        
        Logger::getInstance().log(LogLevel::INFO, "Found code block with language: ", language_tag, 
                                 is_test ? " (test)" : "");
        
        // Create a new code block and add it to the list
        CodeBlock block;
        block.language_tag = language_tag;
        block.content = content;
        block.context_before = context_before;
        block.context_after = context_after;
        block.is_test = is_test;
        block.filename_hint = filename_hint;
        
        blocks.push_back(block);
    }
    
    Logger::getInstance().log(LogLevel::INFO, "Extracted ", blocks.size(), " code blocks");
    return blocks;
}
```

## Organizing Code into Files

After extracting code blocks, the processor organizes them into meaningful files:

1. Group related code blocks together (e.g., implementation and test)
2. Determine appropriate filenames based on content and context
3. Resolve conflicts for multiple blocks of the same type
4. Handle special cases (headers, implementations, tests)

```cpp
std::vector<GeneratedFile> ResponseProcessor::organize_code_blocks(const std::vector<CodeBlock>& blocks) {
    Logger::getInstance().log(LogLevel::INFO, "Organizing code blocks into files");
    
    std::vector<GeneratedFile> files;
    std::map<std::string, std::vector<CodeBlock>> grouped_blocks;
    
    // First, try to group blocks by filename hints if available
    for (const auto& block : blocks) {
        std::string key;
        
        if (!block.filename_hint.empty()) {
            key = block.filename_hint;
        } else {
            // Generate a key based on content analysis
            key = determine_key_from_content(block);
        }
        
        grouped_blocks[key].push_back(block);
    }
    
    // Then process each group into files
    for (const auto& [key, group_blocks] : grouped_blocks) {
        // Split into implementation and test blocks
        std::vector<CodeBlock> impl_blocks;
        std::vector<CodeBlock> test_blocks;
        
        for (const auto& block : group_blocks) {
            if (block.is_test) {
                test_blocks.push_back(block);
            } else {
                impl_blocks.push_back(block);
            }
        }
        
        // Process implementation blocks
        if (!impl_blocks.empty()) {
            GeneratedFile impl_file;
            impl_file.language = determine_language(impl_blocks[0].language_tag);
            impl_file.is_test = false;
            
            // Determine filename
            if (!impl_blocks[0].filename_hint.empty()) {
                impl_file.filename = sanitize_filename(impl_blocks[0].filename_hint);
            } else {
                impl_file.filename = generate_filename_from_content(impl_blocks[0], key);
            }
            
            // Combine content if multiple blocks
            std::stringstream content;
            for (const auto& block : impl_blocks) {
                content << block.content << "\n\n";
            }
            impl_file.content = content.str();
            
            files.push_back(impl_file);
            Logger::getInstance().log(LogLevel::INFO, "Created implementation file: ", impl_file.filename);
        }
        
        // Process test blocks
        if (!test_blocks.empty()) {
            GeneratedFile test_file;
            test_file.language = determine_language(test_blocks[0].language_tag);
            test_file.is_test = true;
            
            // Determine test filename
            std::string impl_name = !impl_blocks.empty() ? impl_file.filename : key;
            test_file.filename = generate_test_filename(impl_name);
            
            // Combine content if multiple test blocks
            std::stringstream content;
            for (const auto& block : test_blocks) {
                content << block.content << "\n\n";
            }
            test_file.content = content.str();
            
            files.push_back(test_file);
            Logger::getInstance().log(LogLevel::INFO, "Created test file: ", test_file.filename);
        }
    }
    
    Logger::getInstance().log(LogLevel::INFO, "Organized into ", files.size(), " files");
    return files;
}
```

## Filename Determination

The processor uses several strategies to determine appropriate filenames:

1. Look for explicit filename hints in code comments or context
2. Analyze class/function names in the code
3. Use language-specific conventions
4. Apply project-specific naming patterns
5. Generate fallback names if needed

```cpp
std::string ResponseProcessor::extract_filename_hint(const std::string& text) {
    Logger::getInstance().log(LogLevel::INFO, "Looking for filename hints in text");
    
    // Regular expressions to look for common filename hint patterns
    std::vector<std::regex> hint_patterns = {
        std::regex("filename:\\s*([\\w.-]+\\.[\\w]+)"),
        std::regex("file:\\s*([\\w.-]+\\.[\\w]+)"),
        std::regex("save\\s+(?:this|the|code)?\\s+(?:to|as|in)\\s+([\\w.-]+\\.[\\w]+)"),
        std::regex("create\\s+(?:a|the)?\\s+file\\s+(?:named|called)?\\s+([\\w.-]+\\.[\\w]+)"),
        std::regex("/\\*\\*\\s*\\n\\s*\\*\\s+([\\w.-]+\\.[\\w]+)"),
        std::regex("// ([\\w.-]+\\.[\\w]+)\\s*\\n")
    };
    
    for (const auto& pattern : hint_patterns) {
        std::smatch match;
        if (std::regex_search(text, match, pattern) && match.size() > 1) {
            Logger::getInstance().log(LogLevel::INFO, "Found filename hint: ", match[1].str());
            return match[1].str();
        }
    }
    
    // Look for class or struct definitions to derive filename
    std::regex class_def_regex("\\bclass\\s+(\\w+)\\b");
    std::smatch class_match;
    if (std::regex_search(text, class_match, class_def_regex) && class_match.size() > 1) {
        std::string class_name = class_match[1].str();
        std::string filename = snake_case_to_file(class_name);
        Logger::getInstance().log(LogLevel::INFO, "Derived filename from class name: ", filename);
        return filename;
    }
    
    Logger::getInstance().log(LogLevel::INFO, "No filename hint found");
    return "";
}

std::string ResponseProcessor::sanitize_filename(const std::string& filename) {
    Logger::getInstance().log(LogLevel::INFO, "Sanitizing filename: ", filename);
    
    // Remove any unsafe characters
    std::string sanitized = filename;
    std::regex unsafe_chars("[^\\w.-]");
    sanitized = std::regex_replace(sanitized, unsafe_chars, "_");
    
    // Ensure it has an extension
    if (sanitized.find('.') == std::string::npos) {
        sanitized += determine_default_extension(sanitized);
    }
    
    Logger::getInstance().log(LogLevel::INFO, "Sanitized filename: ", sanitized);
    return sanitized;
}
```

## File Output Management

The ResponseProcessor also handles the file output concerns:

1. Resolving output directories
2. Creating directories if needed
3. Determining whether to overwrite existing files
4. Setting appropriate file permissions

```cpp
void save_generated_file(const GeneratedFile& file, const std::string& directory) {
    Logger::getInstance().log(LogLevel::INFO, "Saving generated file: ", file.filename);
    
    // Resolve full path
    std::string full_path = directory.empty() ? file.filename : directory + "/" + file.filename;
    
    // Check if file exists
    bool file_exists = std::filesystem::exists(full_path);
    if (file_exists && !m_config.should_overwrite_existing_files()) {
        Logger::getInstance().log(LogLevel::NORMAL, "File already exists and overwrite is disabled: ", full_path);
        
        // Create a backup or versioned filename
        std::string backup_path = full_path + ".new";
        util::write_file(backup_path, file.content);
        Logger::getInstance().log(LogLevel::INFO, "Saved to alternative location: ", backup_path);
        return;
    }
    
    // Ensure directory exists
    std::string dir_path = std::filesystem::path(full_path).parent_path().string();
    if (!dir_path.empty() && !std::filesystem::exists(dir_path)) {
        if (m_config.should_create_missing_directories()) {
            std::filesystem::create_directories(dir_path);
            Logger::getInstance().log(LogLevel::INFO, "Created directory: ", dir_path);
        } else {
            Logger::getInstance().log(LogLevel::ERROR, "Directory does not exist and creation is disabled: ", dir_path);
            return;
        }
    }
    
    // Write the file
    util::write_file(full_path, file.content);
    Logger::getInstance().log(LogLevel::INFO, "File saved successfully: ", full_path);
}
```

## Language Specific Handling

The processor provides language-specific handling for various programming languages:

```cpp
std::string ResponseProcessor::determine_language(const std::string& language_tag) {
    // Normalize language tag
    std::string normalized = language_tag;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), 
                   [](unsigned char c){ return std::tolower(c); });
    
    // Map to standard language names
    std::map<std::string, std::string> language_map = {
        {"cpp", "C++"},
        {"c++", "C++"},
        {"python", "Python"},
        {"py", "Python"},
        {"javascript", "JavaScript"},
        {"js", "JavaScript"},
        {"typescript", "TypeScript"},
        {"ts", "TypeScript"},
        {"java", "Java"},
        {"rust", "Rust"},
        {"rs", "Rust"},
        {"go", "Go"},
        {"csharp", "C#"},
        {"cs", "C#"},
        {"ruby", "Ruby"},
        {"rb", "Ruby"},
        {"php", "PHP"},
        {"swift", "Swift"},
        {"kotlin", "Kotlin"},
        {"shell", "Shell"},
        {"bash", "Bash"},
        {"sh", "Shell"},
        {"html", "HTML"},
        {"css", "CSS"},
        {"json", "JSON"},
        {"xml", "XML"},
        {"yaml", "YAML"},
        {"yml", "YAML"},
        {"sql", "SQL"}
    };
    
    // Return the mapped language or the original if not found
    if (language_map.find(normalized) != language_map.end()) {
        return language_map[normalized];
    }
    
    // Default fallback
    return language_tag;
}

std::string ResponseProcessor::determine_default_extension(const std::string& language) {
    // Map languages to default extensions
    std::map<std::string, std::string> extension_map = {
        {"C++", ".cpp"},
        {"Python", ".py"},
        {"JavaScript", ".js"},
        {"TypeScript", ".ts"},
        {"Java", ".java"},
        {"Rust", ".rs"},
        {"Go", ".go"},
        {"C#", ".cs"},
        {"Ruby", ".rb"},
        {"PHP", ".php"},
        {"Swift", ".swift"},
        {"Kotlin", ".kt"},
        {"Shell", ".sh"},
        {"Bash", ".sh"},
        {"HTML", ".html"},
        {"CSS", ".css"},
        {"JSON", ".json"},
        {"XML", ".xml"},
        {"YAML", ".yml"},
        {"SQL", ".sql"}
    };
    
    // Return the mapped extension or a default
    if (extension_map.find(language) != extension_map.end()) {
        return extension_map[language];
    }
    
    // Default fallback
    return ".txt";
}
```

## Integration with Main Workflow

The Response Processor integrates with the CQL API client workflow:

```cpp
// In code that processes API responses
void process_api_response(const ApiResponse& response) {
    Logger::getInstance().log(LogLevel::INFO, "Processing API response");
    
    if (!response.success) {
        Logger::getInstance().log(LogLevel::ERROR, "API request failed: ", response.error_message);
        return;
    }
    
    // Create response processor with config
    ResponseProcessor processor(m_config);
    
    // Process the response text to extract code
    std::vector<GeneratedFile> files = processor.process_response(response.raw_response);
    
    Logger::getInstance().log(LogLevel::INFO, "Extracted ", files.size(), " files from response");
    
    // Save the generated files
    for (const auto& file : files) {
        save_generated_file(file, m_config.get_output_directory());
    }
    
    Logger::getInstance().log(LogLevel::INFO, "Code generation complete");
}
```

## Testing

The Response Processor should have comprehensive tests:

1. Unit tests for code block extraction
2. Unit tests for filename determination
3. Tests with sample Claude responses
4. Tests for multi-file generation
5. Edge case handling

## Error Handling

The processor implements robust error handling:

1. Graceful handling of malformed code blocks
2. Fallback strategies for filename determination
3. Clear error logging for issues
4. Recovery mechanisms for partial successes

## Extensibility

The design allows for future extensions:

1. Support for additional languages
2. Custom filename determination strategies
3. Project-specific naming conventions
4. Integration with IDEs and editors
5. Configuration options for different outputs