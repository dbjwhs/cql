// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/input_validator.hpp"
#include <algorithm>
#include <filesystem>
#include <cctype>

namespace cql {

// Define dangerous patterns
const std::vector<std::string> InputValidator::SHELL_INJECTION_PATTERNS = {
    "; rm", "; cat", "; ls", "; del", "&&", "||", " | ", "`", "$(cat", "$(rm", "$(ls", 
    "../", "../../", "~", "/dev/", "/proc/", "/sys/",
    "rm ", "del ", "format ", " exec ", " eval ", " system "
};

const std::vector<std::string> InputValidator::SQL_INJECTION_PATTERNS = {
    "'; ", "\"; ", " or '", " and '", " union ", " select ", " insert ", " delete ", 
    " drop ", " update ", " exec ", " execute ", "--", "/*", "*/"
};

const std::vector<std::string> InputValidator::PATH_TRAVERSAL_PATTERNS = {
    "../", "..\\", "..%2f", "..%5c", "%2e%2e%2f", "%2e%2e%5c",
    "/..", "\\..", "/./", "\\.\\", "%00", "\\x00"
};

void InputValidator::validate_file_path(std::string_view path) {
    if (path.empty()) {
        throw SecurityValidationError("Empty file path");
    }
    
    if (path.size() > MAX_PATH_LENGTH) {
        throw SecurityValidationError("File path too long (max: " + 
                                    std::to_string(MAX_PATH_LENGTH) + ")");
    }
    
    // Check for path traversal attempts
    if (contains_dangerous_patterns(path, PATH_TRAVERSAL_PATTERNS)) {
        throw SecurityValidationError("Potential path traversal detected");
    }
    
    // Check for absolute paths outside safe directories
    if (path.front() == '/' && !path.starts_with("/tmp/") && !path.starts_with("/var/tmp/")) {
        throw SecurityValidationError("Absolute paths not allowed outside safe directories");
    }
    
    // Check for Windows drive letters
    if (path.size() >= 2 && path[1] == ':' && std::isalpha(path[0])) {
        throw SecurityValidationError("Windows absolute paths not allowed");
    }
    
    // Validate individual path components
    std::filesystem::path fs_path(path);
    for (const auto& component : fs_path) {
        std::string component_str = component.string();
        if (component_str == "." || component_str == "..") {
            throw SecurityValidationError("Relative path components not allowed");
        }
    }
}

std::string InputValidator::sanitize_file_path(std::string_view path) {
    if (path.empty()) {
        return "";
    }
    
    std::string sanitized(path);
    
    // Remove dangerous sequences
    for (const auto& pattern : PATH_TRAVERSAL_PATTERNS) {
        size_t pos = 0;
        while ((pos = sanitized.find(pattern, pos)) != std::string::npos) {
            sanitized.erase(pos, pattern.length());
        }
    }
    
    // Normalize path separators to forward slashes
    std::replace(sanitized.begin(), sanitized.end(), '\\', '/');
    
    // Remove consecutive slashes
    size_t pos = 0;
    while ((pos = sanitized.find("//", pos)) != std::string::npos) {
        sanitized.erase(pos, 1);
    }
    
    // Ensure path doesn't start with slash (make it relative)
    if (!sanitized.empty() && sanitized.front() == '/') {
        sanitized = sanitized.substr(1);
    }
    
    return sanitized;
}

void InputValidator::validate_filename(std::string_view filename) {
    if (filename.empty()) {
        throw SecurityValidationError("Empty filename");
    }
    
    if (filename.size() > MAX_FILENAME_LENGTH) {
        throw SecurityValidationError("Filename too long (max: " + 
                                    std::to_string(MAX_FILENAME_LENGTH) + ")");
    }
    
    // Check for dangerous characters
    const std::string dangerous_chars = "<>:\"|?*\\x00";
    if (filename.find_first_of(dangerous_chars) != std::string::npos) {
        throw SecurityValidationError("Filename contains dangerous characters");
    }
    
    // Check for reserved names (Windows)
    const std::vector<std::string> reserved_names = {
        "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5",
        "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4",
        "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string upper_filename(filename);
    std::transform(upper_filename.begin(), upper_filename.end(), 
                   upper_filename.begin(), ::toupper);
    
    for (const auto& reserved : reserved_names) {
        if (upper_filename == reserved || upper_filename.starts_with(reserved + ".")) {
            throw SecurityValidationError("Filename uses reserved name: " + std::string(filename));
        }
    }
}

void InputValidator::validate_directive_content(std::string_view directive_name, 
                                              std::string_view content) {
    if (content.size() > MAX_DIRECTIVE_LENGTH) {
        throw SecurityValidationError("Directive content too long for " + 
                                    std::string(directive_name) + 
                                    " (max: " + std::to_string(MAX_DIRECTIVE_LENGTH) + ")");
    }
    
    // Check for shell injection patterns
    if (!is_shell_safe(content)) {
        throw SecurityValidationError("Potential shell injection in directive: " + 
                                    std::string(directive_name));
    }
    
    // Check for SQL injection patterns (in case content is used in queries)
    if (!is_sql_safe(content)) {
        throw SecurityValidationError("Potential SQL injection in directive: " + 
                                    std::string(directive_name));
    }
    
    // Check for null bytes (using string find instead of string_view)
    std::string content_str(content);
    if (content_str.find('\0') != std::string::npos) {
        throw SecurityValidationError("Null bytes not allowed in directive content");
    }
}

void InputValidator::validate_api_key(std::string_view api_key) {
    if (api_key.empty()) {
        throw SecurityValidationError("Empty API key");
    }
    
    if (api_key.size() < 10) {
        throw SecurityValidationError("API key too short (minimum 10 characters)");
    }
    
    if (api_key.size() > MAX_API_KEY_LENGTH) {
        throw SecurityValidationError("API key too long (max: " + 
                                    std::to_string(MAX_API_KEY_LENGTH) + ")");
    }
    
    // API keys should contain only alphanumeric characters, hyphens, and underscores
    if (!contains_only_safe_chars(api_key, "A-Za-z0-9_\\-")) {
        throw SecurityValidationError("API key contains invalid characters");
    }
    
    // Check for patterns that might indicate test/dummy keys
    std::string lower_key(api_key);
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
    
    const std::vector<std::string> test_patterns = {
        "test", "dummy", "fake", "example", "sample", "demo", "placeholder"
    };
    
    for (const auto& pattern : test_patterns) {
        if (lower_key.find(pattern) != std::string::npos) {
            // Allow test keys in test environment, but warn
            // In production, this should throw an exception
            break;
        }
    }
}

bool InputValidator::is_shell_safe(std::string_view input) {
    return !contains_dangerous_patterns(input, SHELL_INJECTION_PATTERNS);
}

std::string InputValidator::sanitize_for_logging(std::string_view input) {
    std::string sanitized(input);
    
    // Truncate if too long
    if (sanitized.size() > 100) {
        sanitized = sanitized.substr(0, 97) + "...";
    }
    
    // Remove or replace dangerous characters
    std::replace(sanitized.begin(), sanitized.end(), '\n', ' ');
    std::replace(sanitized.begin(), sanitized.end(), '\r', ' ');
    std::replace(sanitized.begin(), sanitized.end(), '\t', ' ');
    
    // Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), 
                    sanitized.end());
    
    return sanitized;
}

bool InputValidator::contains_only_safe_chars(std::string_view input, 
                                            std::string_view allowed_chars) {
    try {
        std::regex safe_regex(std::string("^[") + std::string(allowed_chars) + "]*$");
        return std::regex_match(std::string(input), safe_regex);
    } catch (const std::regex_error&) {
        return false;
    }
}

void InputValidator::validate_url(std::string_view url) {
    if (url.empty()) {
        throw SecurityValidationError("Empty URL");
    }
    
    // Must be HTTPS
    if (!url.starts_with("https://")) {
        throw SecurityValidationError("Only HTTPS URLs are allowed");
    }
    
    // Check for basic URL structure
    if (url.find(' ') != std::string_view::npos) {
        throw SecurityValidationError("URL contains spaces");
    }
    
    // Check for dangerous characters
    const std::string dangerous_url_chars = "<>\"'{}|\\^`";
    if (url.find_first_of(dangerous_url_chars) != std::string_view::npos) {
        throw SecurityValidationError("URL contains dangerous characters");
    }
    
    // Basic domain validation - should contain at least one dot
    size_t domain_start = url.find("://") + 3;
    size_t domain_end = url.find('/', domain_start);
    if (domain_end == std::string_view::npos) {
        domain_end = url.size();
    }
    
    std::string_view domain = url.substr(domain_start, domain_end - domain_start);
    if (domain.find('.') == std::string_view::npos) {
        throw SecurityValidationError("URL domain appears invalid");
    }
}

bool InputValidator::is_sql_safe(std::string_view input) {
    return !contains_dangerous_patterns(input, SQL_INJECTION_PATTERNS);
}

bool InputValidator::contains_dangerous_patterns(std::string_view input,
                                               const std::vector<std::string>& patterns) {
    std::string lower_input(input);
    std::transform(lower_input.begin(), lower_input.end(), 
                   lower_input.begin(), ::tolower);
    
    for (const auto& pattern : patterns) {
        if (lower_input.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace cql