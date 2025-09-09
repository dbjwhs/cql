// MIT License
// Copyright (c) 2025 dbjwhs

#include <fstream>
#include <stdexcept>
#include <regex>
#include <set>
#include <optional>
#include <cstdlib>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/project_utils.hpp"
#include "../../include/cql/input_validator.hpp"

namespace cql::util {

// file utility functions implementation
std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void write_file(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filepath);
    }
    file << content;
}

bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

// extract regex matches from a text using a pattern
std::vector<std::vector<std::string>> extract_regex_matches(
    const std::string& content, 
    const std::string& pattern,
    size_t expected_groups
) {
    std::vector<std::vector<std::string>> results;
    std::regex regex_pattern(pattern);
    
    auto begin = std::sregex_iterator(content.begin(), content.end(), regex_pattern);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator it = begin; it != end; ++it) {
        const std::smatch& match = *it;
        
        // skip if we don't have the expected number of groups
        if (expected_groups > 0 && match.size() <= expected_groups) {
            continue;
        }
        
        // each match has multiple groups
        std::vector<std::string> groups;
        for (const auto & i : match) {
            groups.push_back(i.str());
        }
        
        results.push_back(groups);
    }
    
    return results;
}

// extract string values that match a specific regex group
std::set<std::string> extract_regex_group_values(
    const std::string& content, 
    const std::string& pattern,
    size_t group_index
) {
    std::set<std::string> values;
    std::regex regex_pattern(pattern);
    
    auto begin = std::sregex_iterator(content.begin(), content.end(), regex_pattern);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator it = begin; it != end; ++it) {
        if (const std::smatch& match = *it; match.size() > group_index) {
            values.insert(match[group_index].str());
        }
    }
    
    return values;
}

bool load_env_file(const std::string& filepath) {
    try {
        // Security: Validate and resolve file path securely
        InputValidator::validate_file_path(filepath);
        std::string secure_path = InputValidator::resolve_path_securely(filepath);
        
        Logger::getInstance().log(LogLevel::DEBUG, "Loading .env file: ", InputValidator::sanitize_for_logging(secure_path));
        
        // RAII: Use ifstream for automatic resource cleanup
        std::ifstream env_file(secure_path);
        if (!env_file.is_open()) {
            Logger::getInstance().log(LogLevel::DEBUG, "Could not open .env file: ", InputValidator::sanitize_for_logging(secure_path));
            return false; // File doesn't exist or can't be opened
        }
        
        std::string line;
        int line_number = 0;
        
        while (std::getline(env_file, line)) {
            ++line_number;
            
            // Security: Validate line length
            if (line.length() > InputValidator::MAX_DIRECTIVE_LENGTH) {
                Logger::getInstance().log(LogLevel::ERROR, "Line too long in .env file at line ", line_number);
                throw SecurityValidationError("Line too long in .env file");
            }
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Find the '=' separator
            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) {
                Logger::getInstance().log(LogLevel::DEBUG, "Skipping malformed line ", line_number, " in .env file");
                continue; // Skip malformed lines
            }
            
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            
            // Trim whitespace from key
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            
            // Security: Validate environment variable key
            if (key.empty()) {
                Logger::getInstance().log(LogLevel::DEBUG, "Skipping empty key at line ", line_number);
                continue;
            }
            
            // Validate key format (alphanumeric and underscore only)
            if (!InputValidator::contains_only_safe_chars(key, "A-Za-z0-9_")) {
                Logger::getInstance().log(LogLevel::ERROR, "Invalid environment variable key format at line ", line_number);
                throw SecurityValidationError("Invalid environment variable key format: " + InputValidator::sanitize_for_logging(key));
            }
            
            // Security: Validate key and value lengths
            if (key.length() > InputValidator::MAX_IDENTIFIER_LENGTH) {
                throw SecurityValidationError("Environment variable key too long: " + InputValidator::sanitize_for_logging(key));
            }
            
            if (value.length() > InputValidator::MAX_API_KEY_LENGTH * 2) { // Allow some flexibility for values
                throw SecurityValidationError("Environment variable value too long for key: " + InputValidator::sanitize_for_logging(key));
            }
            
            // Remove surrounding quotes from value if present
            if (value.length() >= 2) {
                if ((value.front() == '"' && value.back() == '"') ||
                    (value.front() == '\'' && value.back() == '\'')) {
                    value = value.substr(1, value.length() - 2);
                }
            }
            
            // Security: Validate shell safety for both key and value
            if (!InputValidator::is_shell_safe(key) || !InputValidator::is_shell_safe(value)) {
                Logger::getInstance().log(LogLevel::ERROR, "Potentially unsafe environment variable detected at line ", line_number);
                throw SecurityValidationError("Potentially unsafe environment variable detected");
            }
            
            // Set the environment variable securely
            if (setenv(key.c_str(), value.c_str(), 1) != 0) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to set environment variable: ", InputValidator::sanitize_for_logging(key));
                throw std::runtime_error("Failed to set environment variable: " + InputValidator::sanitize_for_logging(key));
            }
            
            // Debug logging (sanitized)
            Logger::getInstance().log(LogLevel::DEBUG, "Set environment variable: ", InputValidator::sanitize_for_logging(key));
        }
        
        Logger::getInstance().log(LogLevel::DEBUG, "Successfully loaded .env file with ", line_number, " lines processed");
        return true;
        
    } catch (const SecurityValidationError&) {
        throw; // Re-throw security errors as-is
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error loading .env file: ", e.what());
        return false;
    }
}

} // namespace cql::util

namespace cql {

/**
 * Enhanced query processor implementation that separates parsing and validation phases.
 * 
 * This implementation follows these steps:
 * 1. Parse the query, capturing any parser errors but allowing partial parsing
 * 2. Validate the parsed nodes, even if parsing was incomplete
 * 3. Report validation issues first, as they're often more important than syntax errors
 * 4. Report parser errors if validation passed
 * 5. Compile the validated nodes into a query
 *
 * This separation allows us to provide more meaningful error messages to users,
 * focusing on content issues rather than just syntax problems.
 */
std::string QueryProcessor::compile(const std::string_view query_str) {
    // Validate query length first
    InputValidator::validate_query_length(query_str);
    
    // Store parser errors to report them after validation
    std::optional<std::string> parser_error;
    std::vector<std::unique_ptr<QueryNode>> nodes;
    
    // Try to parse the query string but catch parser errors
    try {
        Parser parser(query_str);
        nodes = parser.parse();
    } catch (const ParserError& e) {
        // Store the error message but continue with validation on any nodes we did parse
        parser_error = e.what();
    } catch (const std::exception& e) {
        // For other errors, just rethrow
        throw;
    }
    
    // If no nodes were parsed at all, we can't validate, so report the parser error
    if (nodes.empty() && parser_error) {
        throw std::runtime_error(*parser_error);
    }
    
    // Always attempt validation, even if parsing had errors
    std::vector<ValidationIssue> validation_issues;
    try {
        // validate the query structure with whatever nodes we have
        QueryValidator validator;
        validation_issues = validator.validate(nodes);
    } catch (const ValidationException& e) {
        // For validation errors, use the formatted message which includes the error code
        throw std::runtime_error(e.formatted_message());
    }
    
    // Report validation issues
    for (const auto& issue : validation_issues) {
        std::string level;
        switch (issue.severity) {
            case ValidationSeverity::INFO:
                level = "INFO";
                break;
            case ValidationSeverity::WARNING:
                level = "WARNING";
                break;
            case ValidationSeverity::ERROR:
                level = "ERROR";
                break;
        }
        
        Logger::getInstance().log(
            issue.severity == ValidationSeverity::ERROR ? LogLevel::ERROR : LogLevel::NORMAL,
            "Validation ", level, ": ", issue.message
        );
        
        // For errors, throw an exception - validation errors take precedence
        if (issue.severity == ValidationSeverity::ERROR) {
            // Create a ValidationException with a specific error code based on the message
            std::string error_code = validation_errors::GENERAL_ERROR;
            
            if (issue.message.find("Required directive @LANGUAGE") != std::string::npos) {
                error_code = validation_errors::MISSING_LANGUAGE;
            }
            else if (issue.message.find("Required directive @DESCRIPTION") != std::string::npos) {
                error_code = validation_errors::MISSING_DESCRIPTION;
            }
            else if (issue.message.find("Required directive @COPYRIGHT") != std::string::npos) {
                error_code = validation_errors::MISSING_COPYRIGHT;
            }
            
            ValidationException validation_error(issue.message, error_code, issue.severity);
            throw std::runtime_error(validation_error.formatted_message());
        }
    }
    
    // After reporting validation issues, report any parser errors
    if (parser_error) {
        throw std::runtime_error(*parser_error);
    }
    
    // If we got here, we have valid nodes, so compile the query
    QueryCompiler compiler;
    for (const auto& node : nodes) {
        node->accept(compiler);
    }
    
    return compiler.get_compiled_query();
}

std::string QueryProcessor::compile_file(const std::string& filepath) {
    std::string query = util::read_file(filepath);
    return compile(query);
}

void QueryProcessor::save_compiled(const std::string_view query_str, const std::string& filepath) {
    std::string compiled = compile(query_str);
    util::write_file(filepath, compiled);
}

std::string QueryProcessor::compile_template(
    const std::string& template_name, 
    const std::map<std::string, std::string>& variables
) {
    TemplateManager manager;
    std::string instantiated = manager.instantiate_template(template_name, variables);
    return compile(instantiated);
}

} // namespace cql
