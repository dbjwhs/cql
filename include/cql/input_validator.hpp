// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_INPUT_VALIDATOR_HPP
#define CQL_INPUT_VALIDATOR_HPP

#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <regex>

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
    static constexpr size_t MAX_DIRECTIVE_LENGTH = 10000;
    static constexpr size_t MAX_FILENAME_LENGTH = 255;
    static constexpr size_t MAX_PATH_LENGTH = 4096;
    static constexpr size_t MAX_API_KEY_LENGTH = 200;
    
    /**
     * @brief Validate file path for security issues
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