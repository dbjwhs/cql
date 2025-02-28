// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_VALIDATOR_HPP
#define CQL_VALIDATOR_HPP

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <map>
#include <stdexcept>
#include "nodes.hpp"
#include "lexer.hpp"

namespace cql {

/**
 * Standard validation error codes
 * Using the format VAL-XXX where XXX is a numeric code
 * 100-199: Required directive errors
 * 200-299: Exclusive directive errors
 * 300-399: Dependency rule errors
 * 400-499: Incompatibility errors
 * 500-599: Custom validation errors
 */
namespace validation_errors {
    // General validation errors
    inline constexpr char GENERAL_ERROR[] = "VAL-001";
    inline constexpr char MULTIPLE_ERRORS[] = "VAL-002";
    
    // Required directive errors
    inline constexpr char MISSING_LANGUAGE[] = "VAL-101";
    inline constexpr char MISSING_DESCRIPTION[] = "VAL-102";
    inline constexpr char MISSING_COPYRIGHT[] = "VAL-103";
    
    // Exclusive directive errors
    inline constexpr char DUPLICATE_DIRECTIVE[] = "VAL-201";
    
    // Dependency rule errors
    inline constexpr char MISSING_DEPENDENCY[] = "VAL-301";
    
    // Incompatibility errors
    inline constexpr char INCOMPATIBLE_DIRECTIVES[] = "VAL-401";
}

/**
 * severity levels for validation issues
 */
enum class ValidationSeverity {
    INFO,       // informational message
    WARNING,    // non-fatal warning
    ERROR       // fatal error
};

/**
 * Exception thrown when validation fails
 * 
 * This specialized exception type provides information about validation failures,
 * including the specific validation rule that failed and the severity of the error.
 * 
 * The error code format is "VAL-XXX" where XXX is a numeric code related to 
 * the validation rule that triggered the exception.
 */
class ValidationException : public std::runtime_error {
public:
    /**
     * Create a new validation exception
     * @param message The error message
     * @param error_code Optional error code in format "VAL-XXX"
     * @param severity The severity level of the validation error
     */
    explicit ValidationException(
        const std::string& message,
        const std::string& error_code = validation_errors::GENERAL_ERROR,
        ValidationSeverity severity = ValidationSeverity::ERROR) 
        : std::runtime_error(message),
          m_error_code(error_code),
          m_severity(severity) {}
    
    /**
     * Get the error code for this validation exception
     * @return The error code string
     */
    [[nodiscard]] const std::string& error_code() const { return m_error_code; }
    
    /**
     * Get the severity level of this validation exception
     * @return The validation severity
     */
    [[nodiscard]] ValidationSeverity severity() const { return m_severity; }
    
    /**
     * Create a formatted error message including the error code
     * @return Formatted error message with code prefix
     */
    [[nodiscard]] std::string formatted_message() const {
        return "[" + m_error_code + "] " + what();
    }
    
private:
    std::string m_error_code;
    ValidationSeverity m_severity;
};

/**
 * structure to hold validation issues
 */
struct ValidationIssue {
    ValidationSeverity severity;
    std::string message;
    
    ValidationIssue(ValidationSeverity sev, std::string msg)
        : severity(sev), message(std::move(msg)) {}
        
    /**
     * Get the validation issue as a formatted string
     * @return Formatted string with severity and message
     */
    std::string to_string() const {
        std::string level;
        switch (severity) {
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
        
        return "[" + level + "] " + message;
    }
};

/**
 * rule type for validation
 */
enum class RuleType {
    REQUIRED,       // element must be present
    EXCLUSIVE,      // only one allowed
    DEPENDENCY,     // if a exists, b must exist
    INCOMPATIBLE,   // if a exists, b must not exist
    FORMAT          // content format validation
};

/**
 * validator class to check query structure and content
 */
class QueryValidator {
public:
    QueryValidator();
    
    // run validation on parsed nodes
    std::vector<ValidationIssue> validate(const std::vector<std::unique_ptr<QueryNode>>& nodes);
    
    // configure validation rules
    void configure_required(const std::vector<TokenType>& required_directives);
    void configure_exclusive(const std::vector<TokenType>& exclusive_directives);
    void configure_dependency(TokenType dependent, TokenType dependency);
    void configure_incompatible(TokenType a, TokenType b);
    
    // add custom validation rule
    void add_custom_rule(std::function<std::optional<ValidationIssue>(const std::vector<std::unique_ptr<QueryNode>>&)> rule);
    
private:
    // configuration storage
    std::vector<TokenType> m_required_directives;
    std::vector<TokenType> m_exclusive_directives;
    std::vector<std::pair<TokenType, TokenType>> m_dependency_rules;
    std::vector<std::pair<TokenType, TokenType>> m_incompatibility_rules;
    std::vector<std::function<std::optional<ValidationIssue>(const std::vector<std::unique_ptr<QueryNode>>&)>> m_custom_rules;
    
    // count occurrences of directive types
    std::map<TokenType, int> count_directives(const std::vector<std::unique_ptr<QueryNode>>& nodes);
    
    // helper methods for validation
    std::vector<ValidationIssue> check_required(const std::map<TokenType, int>& counts);
    std::vector<ValidationIssue> check_exclusive(const std::map<TokenType, int>& counts);
    std::vector<ValidationIssue> check_dependencies(const std::map<TokenType, int>& counts);
    std::vector<ValidationIssue> check_incompatibilities(const std::map<TokenType, int>& counts);
    std::vector<ValidationIssue> run_custom_rules(const std::vector<std::unique_ptr<QueryNode>>& nodes);
};

} // namespace cql

#endif // cql_validator_hpp
