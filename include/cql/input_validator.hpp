// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_INPUT_VALIDATOR_HPP
#define CQL_INPUT_VALIDATOR_HPP

#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <regex>
#include <filesystem>

namespace cql {

/**
 * @class SecurityValidationError
 * @brief Exception thrown when security validation fails
 */
class SecurityValidationError : public std::runtime_error {
public:
    explicit SecurityValidationError(const std::string& message) 
        : std::runtime_error("Security validation failed: " + message) {}
};

/**
 * @class InputValidator
 * @brief Comprehensive input validation for security
 * 
 * This class provides various validation methods to prevent security
 * vulnerabilities such as injection attacks, path traversal, etc.
 */
class InputValidator {
public:
    /**
     * @brief Maximum allowed input length for various input types
     */
    // File and path limits
    static constexpr size_t MAX_DIRECTIVE_LENGTH = 10000;
    static constexpr size_t MAX_FILENAME_LENGTH = 255;
    static constexpr size_t MAX_PATH_LENGTH = 4096;
    static constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024;  // 10 MB
    
    // API and network limits
    static constexpr size_t MAX_API_KEY_LENGTH = 200;
    static constexpr size_t MAX_URL_LENGTH = 2048;
    static constexpr size_t MAX_RESPONSE_SIZE = 100 * 1024 * 1024;  // 100 MB
    
    // Template and variable limits
    static constexpr size_t MAX_TEMPLATE_NAME_LENGTH = 128;
    static constexpr size_t MAX_CATEGORY_NAME_LENGTH = 64;
    static constexpr size_t MAX_VARIABLE_NAME_LENGTH = 64;
    static constexpr size_t MAX_VARIABLE_VALUE_LENGTH = 1024;
    static constexpr size_t MAX_TEMPLATE_CONTENT_LENGTH = 100000;
    
    // Query and compilation limits
    static constexpr size_t MAX_QUERY_LENGTH = 50000;
    static constexpr size_t MAX_COMPILED_OUTPUT_LENGTH = 100000;
    static constexpr size_t MAX_EXAMPLE_LENGTH = 5000;
    static constexpr size_t MAX_TEST_CASE_LENGTH = 500;
    
    // Directive-specific limits
    static constexpr size_t MAX_COPYRIGHT_LENGTH = 500;
    static constexpr size_t MAX_DESCRIPTION_LENGTH = 1000;
    static constexpr size_t MAX_CONTEXT_LENGTH = 2000;
    static constexpr size_t MAX_CONSTRAINT_LENGTH = 500;
    static constexpr size_t MAX_ARCHITECTURE_LENGTH = 500;
    
    // Security limits
    static constexpr size_t MAX_LOG_MESSAGE_LENGTH = 1000;
    static constexpr size_t MAX_ERROR_MESSAGE_LENGTH = 500;
    static constexpr size_t MAX_IDENTIFIER_LENGTH = 128;
    
    /**
     * @brief Resolve symlinks and canonicalize path securely
     * @param path File path to resolve
     * @return Canonical absolute path with symlinks resolved
     * @throws SecurityValidationError if resolution fails or path is unsafe
     */
    static std::string resolve_path_securely(std::string_view path);
    
    /**
     * @brief Validate file path for security issues (with symlink resolution)
     * @param path File path to validate
     * @throws SecurityValidationError if path is unsafe
     */
    static void validate_file_path(std::string_view path);
    
    /**
     * @brief Sanitize file path to prevent directory traversal
     * @param path Input path
     * @return Sanitized path safe for use
     */
    static std::string sanitize_file_path(std::string_view path);
    
    /**
     * @brief Validate filename for security issues
     * @param filename Filename to validate
     * @throws SecurityValidationError if filename is unsafe
     */
    static void validate_filename(std::string_view filename);
    
    /**
     * @brief Validate directive content for injection attacks
     * @param directive_name Name of the directive
     * @param content Content to validate
     * @throws SecurityValidationError if content is unsafe
     */
    static void validate_directive_content(std::string_view directive_name, 
                                         std::string_view content);
    
    /**
     * @brief Validate API key format
     * @param api_key API key to validate
     * @throws SecurityValidationError if API key format is invalid
     */
    static void validate_api_key(std::string_view api_key);
    
    /**
     * @brief Check for potential shell injection patterns
     * @param input Input string to check
     * @return true if input appears safe, false if suspicious
     */
    static bool is_shell_safe(std::string_view input);
    
    /**
     * @brief Sanitize input for safe logging (remove sensitive patterns)
     * @param input Input to sanitize
     * @return Sanitized string safe for logging
     */
    static std::string sanitize_for_logging(std::string_view input);
    
    /**
     * @brief Check if string contains only safe characters
     * @param input String to check
     * @param allowed_chars Set of allowed characters (regex pattern)
     * @return true if safe, false otherwise
     */
    static bool contains_only_safe_chars(std::string_view input, 
                                       std::string_view allowed_chars);
    
    /**
     * @brief Validate URL for security (must be HTTPS, valid domain)
     * @param url URL to validate
     * @throws SecurityValidationError if URL is unsafe
     */
    static void validate_url(std::string_view url);
    
    /**
     * @brief Check for potential SQL injection patterns
     * @param input Input to check
     * @return true if input appears safe, false if suspicious
     */
    static bool is_sql_safe(std::string_view input);
    
    /**
     * @brief Sanitize template variables for security validation
     * @param input Input containing template variables
     * @return Input with template variables replaced by safe placeholders
     */
    static std::string sanitize_template_variables(std::string_view input);
    
    /**
     * @brief Validate template name length and format
     * @param name Template name to validate
     * @throws SecurityValidationError if name is invalid
     */
    static void validate_template_name(std::string_view name);
    
    /**
     * @brief Validate variable name and value
     * @param name Variable name
     * @param value Variable value
     * @throws SecurityValidationError if invalid
     */
    static void validate_variable(std::string_view name, std::string_view value);
    
    /**
     * @brief Validate query length
     * @param query Query text to validate
     * @throws SecurityValidationError if query is too long
     */
    static void validate_query_length(std::string_view query);
    
    /**
     * @brief Validate response size
     * @param response Response text to validate
     * @throws SecurityValidationError if response is too large
     */
    static void validate_response_size(std::string_view response);
    
    /**
     * @brief Validate category name
     * @param category Category name to validate
     * @throws SecurityValidationError if invalid
     */
    static void validate_category_name(std::string_view category);

private:
    // Common dangerous patterns
    static const std::vector<std::string> SHELL_INJECTION_PATTERNS;
    static const std::vector<std::string> SQL_INJECTION_PATTERNS;
    static const std::vector<std::string> PATH_TRAVERSAL_PATTERNS;
    
    /**
     * @brief Check input against dangerous patterns
     * @param input Input to check
     * @param patterns Vector of dangerous patterns to check against
     * @return true if input contains dangerous patterns
     */
    static bool contains_dangerous_patterns(std::string_view input,
                                          const std::vector<std::string>& patterns);
};

} // namespace cql

#endif // CQL_INPUT_VALIDATOR_HPP