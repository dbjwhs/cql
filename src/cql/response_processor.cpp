// MIT License
// Copyright (c) 2025 dbjwhs

#include <string>
#include <regex>
#include <map>
#include <vector>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/response_processor.hpp"
#include "../../include/cql/api_client.hpp"
#include "../../include/cql/project_utils.hpp"

namespace cql {

ResponseProcessor::ResponseProcessor(const Config& config)
    : m_config(config),
      m_output_directory(config.get_output_directory()),
      m_overwrite_existing(config.should_overwrite_existing_files()),
      m_create_directories(config.should_create_missing_directories()) {}

std::vector<GeneratedFile> ResponseProcessor::process_response(const std::string& response_text) {
    Logger::getInstance().log(LogLevel::INFO, "Processing API response");
    
    // Extract code blocks from the response
    const std::vector<CodeBlock> blocks = extract_code_blocks(response_text);
    
    if (blocks.empty()) {
        Logger::getInstance().log(LogLevel::NORMAL, "No code blocks found in response");
        return {};
    }
    
    // Organize code blocks into files
    std::vector<GeneratedFile> files = organize_code_blocks(blocks);
    
    Logger::getInstance().log(LogLevel::INFO, "Extracted ", files.size(), " files from response");
    return files;
}

void ResponseProcessor::set_output_directory(const std::string& directory) {
    m_output_directory = directory;
}

void ResponseProcessor::set_overwrite_existing(bool overwrite) {
    m_overwrite_existing = overwrite;
}

void ResponseProcessor::set_create_directories(bool create) {
    m_create_directories = create;
}

std::vector<CodeBlock> ResponseProcessor::extract_code_blocks(const std::string& response_text) {
    std::vector<CodeBlock> blocks;
    
    Logger::getInstance().log(LogLevel::INFO, "Extracting code blocks from response");
    
    // Regex to match code blocks with language tags
    std::regex code_block_regex(R"(```([a-zA-Z0-9+#]+)\s*\n([\s\S]*?)\n```)");
    
    auto begin = std::sregex_iterator(response_text.begin(), response_text.end(), code_block_regex);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator it = begin; it != end; ++it) {
        const std::smatch& match = *it;
        
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
        
        // Determine if this is tested code
        bool is_test = language_tag.find("test") != std::string::npos || 
                      content.find("test") != std::string::npos ||
                      context_before.find("test") != std::string::npos;
                      
        // Look for filename hints
        std::string filename_hint = extract_filename_hint(context_before + content + context_after);
        
        Logger::getInstance().log(LogLevel::INFO, "Found code block with language: ", language_tag, 
                                 is_test ? " (test)" : "");
        
        // Create a new code block and add it to the list
        CodeBlock block;
        block.m_language_tag = language_tag;
        block.m_content = content;
        block.m_context_before = context_before;
        block.m_context_after = context_after;
        block.m_is_test = is_test;
        block.m_filename_hint = filename_hint;
        
        blocks.push_back(block);
    }
    
    Logger::getInstance().log(LogLevel::INFO, "Extracted ", blocks.size(), " code blocks");
    return blocks;
}

std::string ResponseProcessor::extract_filename_hint(const std::string& text) {
    Logger::getInstance().log(LogLevel::INFO, "Looking for filename hints in text");
    
    // Regular expressions to look for common filename hint patterns
    std::vector<std::regex> hint_patterns = {
        std::regex(R"(filename:\s*([\w.-]+\.[\w]+))"),
        std::regex(R"(file:\s*([\w.-]+\.[\w]+))"),
        std::regex(R"(save\s+(?:this|the|code)?\s+(?:to|as|in)\s+([\w.-]+\.[\w]+))"),
        std::regex(R"(create\s+(?:a|the)?\s+file\s+(?:named|called)?\s+([\w.-]+\.[\w]+))"),
        std::regex(R"(/\*\*\s*\n\s*\*\s+([\w.-]+\.[\w]+))"),
        std::regex(R"(// ([\w.-]+\.[\w]+)\s*\n)")
    };
    
    for (const auto& pattern : hint_patterns) {
        if (std::smatch match; std::regex_search(text, match, pattern) && match.size() > 1) {
            Logger::getInstance().log(LogLevel::INFO, "Found filename hint: ", match[1].str());
            return match[1].str();
        }
    }
    
    // Look for class or struct definitions to derive filename
    std::regex class_def_regex(R"(\bclass\s+(\w+)\b)");
    if (std::smatch class_match; std::regex_search(text, class_match, class_def_regex) && class_match.size() > 1) {
        std::string class_name = class_match[1].str();
        
        // Convert CamelCase to snake_case for filename
        std::string filename;
        for (char c : class_name) {
            if (std::isupper(c) && !filename.empty()) {
                filename += '_';
            }
            filename += std::tolower(c);
        }
        
        // Add appropriate extension based on content
        if (text.find("#include") != std::string::npos) {
            // C++ file
            if (text.find("class") != std::string::npos && 
                text.find("public:") != std::string::npos && 
                text.find("private:") != std::string::npos) {
                filename += ".hpp";
            } else {
                filename += ".cpp";
            }
        } else if (text.find("def ") != std::string::npos || text.find("import ") != std::string::npos) {
            // Python file
            filename += ".py";
        } else {
            // Default to .hpp for C++ class definitions
            filename += ".hpp";
        }
        
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
    const std::regex unsafe_chars("[^\\w.-]");
    sanitized = std::regex_replace(sanitized, unsafe_chars, "_");
    
    // Ensure it has an extension
    if (sanitized.find('.') == std::string::npos) {
        sanitized += determine_default_extension(determine_language(sanitized));
    }
    
    Logger::getInstance().log(LogLevel::INFO, "Sanitized filename: ", sanitized);
    return sanitized;
}

std::string ResponseProcessor::determine_language(const std::string& language_tag) {
    // Normalize language tag
    std::string normalized = language_tag;
    std::ranges::transform(normalized, normalized.begin(), [](const unsigned char c) {
        return std::tolower(c);
    });
    
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
    if (language_map.contains(normalized)) {
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
    if (extension_map.contains(language)) {
        return extension_map[language];
    }
    
    // Default fallback
    return ".txt";
}

std::string ResponseProcessor::determine_key_from_content(const CodeBlock& block) {
    // Try to extract meaningful identifier from the code
    
    // For C++, look for class or function names
    if (block.m_language_tag == "cpp" || block.m_language_tag == "c++") {
        // Try to find a class name
        std::regex class_regex("class\\s+(\\w+)");
        if (std::smatch class_match; std::regex_search(block.m_content, class_match, class_regex) && class_match.size() > 1) {
            return class_match[1].str();
        }
        
        // Try to find the struct name
        std::regex struct_regex("struct\\s+(\\w+)");
        if (std::smatch struct_match; std::regex_search(block.m_content, struct_match, struct_regex) && struct_match.size() > 1) {
            return struct_match[1].str();
        }
        
        // Try to find the main function name
        std::regex func_regex(R"((\w+)\s*\([^)]*\)\s*\{)");
        if (std::smatch func_match; std::regex_search(block.m_content, func_match, func_regex) && func_match.size() > 1) {
            if (std::string func_name = func_match[1].str(); func_name != "if" && func_name != "for" && func_name != "while") {
                return func_name;
            }
        }
    }
    
    // For Python, look for class or function names
    if (block.m_language_tag == "python" || block.m_language_tag == "py") {
        // Try to find a class name
        std::regex class_regex("class\\s+(\\w+)");
        if (std::smatch class_match; std::regex_search(block.m_content, class_match, class_regex) && class_match.size() > 1) {
            return class_match[1].str();
        }
        
        // Try to find a function name
        std::regex func_regex("def\\s+(\\w+)");
        if (std::smatch func_match; std::regex_search(block.m_content, func_match, func_regex) && func_match.size() > 1) {
            return func_match[1].str();
        }
    }
    
    // Generate a default key based on the language
    return "code_" + block.m_language_tag;
}

std::string ResponseProcessor::generate_filename_from_content(const CodeBlock& block, const std::string& key) {
    // Convert key to snake_case for filename
    std::string filename;
    for (char c : key) {
        if (std::isupper(c) && !filename.empty()) {
            filename += '_';
        }
        filename += std::tolower(c);
    }
    
    // Add appropriate extension
    filename += determine_default_extension(determine_language(block.m_language_tag));
    
    return filename;
}

std::string ResponseProcessor::generate_test_filename(const std::string& impl_name) {
    // Extract base name without extension
    const size_t dot_pos = impl_name.find_last_of('.');
    const std::string base_name = dot_pos != std::string::npos ? impl_name.substr(0, dot_pos) : impl_name;
    
    // Extract extension
    const std::string extension = dot_pos != std::string::npos ? impl_name.substr(dot_pos) : determine_default_extension("C++");
    
    // Generate test filename
    return base_name + "_test" + extension;
}

std::string ResponseProcessor::determine_filename(const CodeBlock& block, const std::string& context) {
    // First, check if there's a filename hint
    if (!block.m_filename_hint.empty()) {
        return sanitize_filename(block.m_filename_hint);
    }
    
    // Generate a filename from the content
    return generate_filename_from_content(block, context);
}

std::vector<GeneratedFile> ResponseProcessor::organize_code_blocks(const std::vector<CodeBlock>& blocks) {
    Logger::getInstance().log(LogLevel::INFO, "Organizing code blocks into files");
    
    std::vector<GeneratedFile> files;
    std::map<std::string, std::vector<CodeBlock>> grouped_blocks;
    
    // First, try to group blocks by filename hints if available
    for (const auto& block : blocks) {
        std::string key;
        
        if (!block.m_filename_hint.empty()) {
            key = block.m_filename_hint;
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
            if (block.m_is_test) {
                test_blocks.push_back(block);
            } else {
                impl_blocks.push_back(block);
            }
        }
        
        // Process implementation blocks
        if (!impl_blocks.empty()) {
            GeneratedFile impl_file;
            impl_file.m_language = determine_language(impl_blocks[0].m_language_tag);
            impl_file.m_is_test = false;
            
            // Determine filename
            if (!impl_blocks[0].m_filename_hint.empty()) {
                impl_file.m_filename = sanitize_filename(impl_blocks[0].m_filename_hint);
            } else {
                impl_file.m_filename = generate_filename_from_content(impl_blocks[0], key);
            }
            
            // Combine content if multiple blocks
            std::stringstream content;
            for (const auto& block : impl_blocks) {
                content << block.m_content << "\n\n";
            }
            impl_file.m_content = content.str();
            
            files.push_back(impl_file);
            Logger::getInstance().log(LogLevel::INFO, "Created implementation file: ", impl_file.m_filename);
        }
        
        // Process test blocks
        if (!test_blocks.empty()) {
            GeneratedFile test_file;
            test_file.m_language = determine_language(test_blocks[0].m_language_tag);
            test_file.m_is_test = true;
            
            // Determine test filename
            std::string impl_name = !impl_blocks.empty() ? 
                                  files.back().m_filename : 
                                  key;
            test_file.m_filename = generate_test_filename(impl_name);
            
            // Combine content if multiple test blocks
            std::stringstream content;
            for (const auto& block : test_blocks) {
                content << block.m_content << "\n\n";
            }
            test_file.m_content = content.str();
            
            files.push_back(test_file);
            Logger::getInstance().log(LogLevel::INFO, "Created test file: ", test_file.m_filename);
        }
    }
    
    Logger::getInstance().log(LogLevel::INFO, "Organized into ", files.size(), " files");
    return files;
}

bool save_generated_file(const GeneratedFile& file, const std::string& directory, const Config& config) {
    Logger::getInstance().log(LogLevel::INFO, "Saving generated file: ", file.m_filename);
    
    // Resolve a full path
    const std::string full_path = directory.empty() ? file.m_filename : directory + "/" + file.m_filename;
    
    // Check if the file exists
    if (const bool file_exists = std::filesystem::exists(full_path); file_exists && !config.should_overwrite_existing_files()) {
        Logger::getInstance().log(LogLevel::NORMAL, "File already exists and overwrite is disabled: ", full_path);
        
        // Create a backup or versioned filename
        std::string backup_path = full_path + ".new";
        cql::util::write_file(backup_path, file.m_content);
        Logger::getInstance().log(LogLevel::INFO, "Saved to alternative location: ", backup_path);
        return true;
    }
    
    // Ensure the directory exists
    if (const std::string dir_path = std::filesystem::path(full_path).parent_path().string();
        !dir_path.empty() && !std::filesystem::exists(dir_path)) {
        if (config.should_create_missing_directories()) {
            try {
                std::filesystem::create_directories(dir_path);
                Logger::getInstance().log(LogLevel::INFO, "Created directory: ", dir_path);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to create directory: ", e.what());
                return false;
            }
        } else {
            Logger::getInstance().log(LogLevel::ERROR, "Directory does not exist and creation is disabled: ", dir_path);
            return false;
        }
    }
    
    // Write the file
    try {
        cql::util::write_file(full_path, file.m_content);
        Logger::getInstance().log(LogLevel::INFO, "File saved successfully: ", full_path);
        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Failed to write file: ", e.what());
        return false;
    }
}

} // namespace cql