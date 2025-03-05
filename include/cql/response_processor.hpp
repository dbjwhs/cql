// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_RESPONSE_PROCESSOR_HPP
#define CQL_RESPONSE_PROCESSOR_HPP

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <string_view>
#include "api_client.hpp"

namespace cql {

/**
 * @struct CodeBlock
 * @brief Structure to represent a code block extracted from an API response
 */
struct CodeBlock {
    std::string m_language_tag;    ///< Programming language tag (e.g., "cpp", "python")
    std::string m_content;         ///< Content of the code block
    std::string m_context_before;  ///< Text context before the code block
    std::string m_context_after;   ///< Text context after the code block
    bool m_is_test;                ///< Whether this is a test code block
    std::string m_filename_hint;   ///< Hint for the filename (if found in context)
};

/**
 * @class ResponseProcessor
 * @brief Processes API responses and extracts code blocks
 */
class ResponseProcessor {
public:
    /**
     * @brief Constructor
     * @param config Configuration for the response processor
     */
    explicit ResponseProcessor(const Config& config);
    
    /**
     * @brief Process an API response and extract code blocks
     * @param response_text The raw response text to process
     * @return Vector of generated files
     */
    [[nodiscard]] std::vector<GeneratedFile> process_response(const std::string& response_text);
    
    /**
     * @brief Set the output directory for generated files
     * @param directory The directory path
     */
    void set_output_directory(const std::string& directory);
    
    /**
     * @brief Set whether to overwrite existing files
     * @param overwrite Whether to overwrite existing files
     */
    void set_overwrite_existing(bool overwrite);
    
    /**
     * @brief Set whether to create missing directories
     * @param create Whether to create missing directories
     */
    void set_create_directories(bool create);
    
private:
    /**
     * @brief Extract code blocks from response text
     * @param response_text The response text to process
     * @return Vector of extracted code blocks
     */
    [[nodiscard]] static std::vector<CodeBlock> extract_code_blocks(const std::string& response_text);
    
    /**
     * @brief Organize code blocks into files
     * @param blocks The code blocks to organize
     * @return Vector of generated files
     */
    [[nodiscard]] std::vector<GeneratedFile> organize_code_blocks(const std::vector<CodeBlock>& blocks);
    
    /**
     * @brief Determine a filename for a code block
     * @param block The code block
     * @param context Additional context (e.g., key from content analysis)
     * @return Generated filename
     */
    [[nodiscard]] std::string determine_filename(const CodeBlock& block, const std::string& context);
    
    /**
     * @brief Extract a filename hint from text
     * @param text The text to search for filename hints
     * @return The extracted filename hint, or empty string if none found
     */
    [[nodiscard]] static std::string extract_filename_hint(const std::string& text);
    
    /**
     * @brief Sanitize a filename to be valid on the filesystem
     * @param filename The filename to sanitize
     * @return The sanitized filename
     */
    [[nodiscard]] std::string sanitize_filename(const std::string& filename);
    
    /**
     * @brief Determine the standardized language name from a language tag
     * @param language_tag The language tag (e.g., "cpp", "py")
     * @return The standardized language name (e.g., "C++", "Python")
     */
    [[nodiscard]] static std::string determine_language(const std::string& language_tag);
    
    /**
     * @brief Determine a default file extension for a language
     * @param language The standardized language name
     * @return The default file extension (including dot)
     */
    [[nodiscard]] std::string determine_default_extension(const std::string& language);
    
    /**
     * @brief Generate a filename from code content
     * @param block The code block
     * @param key The key from content analysis
     * @return The generated filename
     */
    [[nodiscard]] std::string generate_filename_from_content(const CodeBlock& block, const std::string& key);
    
    /**
     * @brief Generate a test filename from an implementation filename
     * @param impl_name The implementation filename
     * @return The test filename
     */
    [[nodiscard]] std::string generate_test_filename(const std::string& impl_name);
    
    /**
     * @brief Determine a key from code content for grouping related blocks
     * @param block The code block
     * @return A key for grouping
     */
    [[nodiscard]] static std::string determine_key_from_content(const CodeBlock& block);
    
    // Member variables
    Config m_config;                 ///< Configuration for the processor
    std::string m_output_directory;  ///< Directory to save generated files
    bool m_overwrite_existing;       ///< Whether to overwrite existing files
    bool m_create_directories;       ///< Whether to create missing directories
};

/**
 * @brief Save a generated file to disk
 * @param file The file to save
 * @param directory The directory to save to
 * @param config The configuration for saving
 * @return true if saved successfully, false otherwise
 */
bool save_generated_file(const GeneratedFile& file, const std::string& directory, const Config& config);

} // namespace cql

#endif // CQL_RESPONSE_PROCESSOR_HPP