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
 * @namespace validation_errors
 * @brief Standard validation error codes
 *
 * Using the format VAL-XXX where XXX is a numeric code:
 * - 001-099: General validation errors
 * - 100-199: Required directive errors
 * - 200-299: Exclusive directive errors
 * - 300-399: Dependency rule errors
 * - 400-499: Incompatibility errors
 * - 500-599: Custom validation errors
 */
namespace validation_errors {
    /// General validation errors
    inline constexpr char GENERAL_ERROR[] = "VAL-001";
    inline constexpr char MULTIPLE_ERRORS[] = "VAL-002";
    
    /// Required directive errors
    inline constexpr char MISSING_LANGUAGE[] = "VAL-101";
    inline constexpr char MISSING_DESCRIPTION[] = "VAL-102";
    inline constexpr char MISSING_COPYRIGHT[] = "VAL-103";
    
    /// Exclusive directive errors
    inline constexpr char DUPLICATE_DIRECTIVE[] = "VAL-201";
    
    /// Dependency rule errors
    inline constexpr char MISSING_DEPENDENCY[] = "VAL-301";
    
    /// Incompatibility errors
    inline constexpr char INCOMPATIBLE_DIRECTIVES[] = "VAL-401";
}

/**
 * @enum ValidationSeverity
 * @brief Severity levels for validation issues
 *
 * Determines how validation issues should be handled:
 * - INFO: Informational messages that don't affect validity
 * - WARNING: Issues that should be addressed but don't invalidate the query
 * - ERROR: Fatal issues that make the query invalid
 */
enum class ValidationSeverity {
    INFO,       ///< Informational message
    WARNING,    ///< Non-fatal warning
    ERROR       ///< Fatal error
};

/**
 * @class ValidationException
 * @brief Exception thrown when validation fails
 *
 * This specialized exception type provides detailed information about validation failures,
 * including the specific validation rule that failed and the severity of the error.
 *
 * The error code format is "VAL-XXX" where XXX is a numeric code related to
 * the validation rule that triggered the exception.
 */
class ValidationException : public std::runtime_error {
public:
    /**
     * @brief Create a new validation exception
     *
     * @param message The error message describing the validation failure
     * @param error_code Error code in format "VAL-XXX" (defaults to GENERAL_ERROR)
     * @param severity The severity level of the validation error (defaults to ERROR)
     */
    explicit ValidationException(
        const std::string& message,
        const std::string& error_code = validation_errors::GENERAL_ERROR,
        ValidationSeverity severity = ValidationSeverity::ERROR) 
        : std::runtime_error(message),
          m_error_code(error_code),
          m_severity(severity) {}
    
    /**
     * @brief Get the error code for this validation exception
     * @return The error code string (e.g., "VAL-101")
     */
    [[nodiscard]] const std::string& error_code() const { return m_error_code; }
    
    /**
     * @brief Get the severity level of this validation exception
     * @return The validation severity enum value
     */
    [[nodiscard]] ValidationSeverity severity() const { return m_severity; }
    
    /**
     * @brief Create a formatted error message including the error code
     * @return Formatted string in the form "[VAL-XXX] Error message"
     */
    [[nodiscard]] std::string formatted_message() const {
        return "[" + m_error_code + "] " + what();
    }
    
private:
    std::string m_error_code;       ///< The validation error code
    ValidationSeverity m_severity;  ///< The severity level
};

/**
 * @struct ValidationIssue
 * @brief Structure to hold validation issues detected during validation
 *
 * ValidationIssue captures information about a specific validation problem,
 * including its severity and a descriptive message. Multiple issues can be
 * collected during validation to provide comprehensive feedback.
 */
struct ValidationIssue {
    ValidationSeverity severity;  ///< The severity level of the issue
    std::string message;          ///< Descriptive message about the issue
    
    /**
     * @brief Construct a validation issue
     *
     * @param sev The severity level (INFO, WARNING, ERROR)
     * @param msg The descriptive message
     */
    ValidationIssue(ValidationSeverity sev, std::string msg)
        : severity(sev), message(std::move(msg)) {}
        
    /**
     * @brief Get the validation issue as a formatted string
     * @return Formatted string with severity prefix, e.g., "[ERROR] Missing required directive"
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
 * @enum RuleType
 * @brief Types of validation rules supported by the validator
 */
enum class RuleType {
    REQUIRED,     ///< Element must be present (e.g., @language directive)
    EXCLUSIVE,    ///< Only one allowed (e.g., only one @model directive)
    DEPENDENCY,   ///< If A exists, B must exist (e.g., if @test exists, @language must exist)
    INCOMPATIBLE, ///< If A exists, B must not exist (e.g., conflicting patterns)
    FORMAT        ///< Content format validation (e.g., variable name format)
};

/**
 * @class QueryValidator
 * @brief Validator class to check query structure and content
 *
 * The QueryValidator ensures CQL queries meet requirements by applying validation
 * rules. It supports multiple types of validation:
 *
 * 1. Required directives: Directives that must be present in every query
 * 2. Exclusive directives: Directives that can appear at most once
 * 3. Dependency rules: If directive A exists, directive B must also exist
 * 4. Incompatibility rules: If directive A exists, directive B must not exist
 * 5. Custom validation: User-defined validation functions for complex rules
 *
 * The validator can be configured with different rule sets depending on the
 * use case, allowing for flexible validation requirements.
 */
class QueryValidator {
public:
    /**
     * @brief Construct a new QueryValidator with default rules
     *
     * The default validator is configured with standard CQL language rules:
     * - Required: @language, @description
     * - Exclusive: Most directives (@language, @model, etc.)
     * - Dependencies: Various relationships between directives
     */
    QueryValidator();
    
    /**
     * @brief Run validation on a set of parsed nodes
     *
     * Applies all configured validation rules to the AST nodes and collects
     * any validation issues found.
     *
     * @param nodes Vector of unique pointers to AST nodes
     * @return Vector of validation issues (empty if validation passes)
     */
    std::vector<ValidationIssue> validate(const std::vector<std::unique_ptr<QueryNode>>& nodes);
    
    /**
     * @brief Configure which directives are required
     *
     * @param required_directives Vector of TokenTypes that must be present
     */
    void configure_required(const std::vector<TokenType>& required_directives);
    
    /**
     * @brief Configure which directives can appear at most once
     *
     * @param exclusive_directives Vector of TokenTypes that can appear at most once
     */
    void configure_exclusive(const std::vector<TokenType>& exclusive_directives);
    
    /**
     * @brief Configure a dependency relationship between directives
     *
     * If the dependent directive exists, the dependency directive must also exist.
     *
     * @param dependent The directive that depends on another
     * @param dependency The directive that must exist if dependent exists
     */
    void configure_dependency(TokenType dependent, TokenType dependency);
    
    /**
     * @brief Configure an incompatibility relationship between directives
     *
     * If directive A exists, directive B must not exist and vice versa.
     *
     * @param a First directive in the incompatible pair
     * @param b Second directive in the incompatible pair
     */
    void configure_incompatible(TokenType a, TokenType b);
    
    /**
     * @brief Add a custom validation rule
     *
     * Custom rules allow for complex validation logic beyond the standard rule types.
     * The rule function should return a ValidationIssue if validation fails, or
     * std::nullopt if validation passes.
     *
     * @param rule Function that performs custom validation
     */
    void add_custom_rule(std::function<std::optional<ValidationIssue>(const std::vector<std::unique_ptr<QueryNode>>&)> rule);
    
private:
    // Configuration storage
    std::vector<TokenType> m_required_directives;  ///< Directives that must be present
    std::vector<TokenType> m_exclusive_directives; ///< Directives limited to one occurrence
    std::vector<std::pair<TokenType, TokenType>> m_dependency_rules;      ///< (dependent, dependency) pairs
    std::vector<std::pair<TokenType, TokenType>> m_incompatibility_rules; ///< (a, b) incompatible pairs
    std::vector<std::function<std::optional<ValidationIssue>(const std::vector<std::unique_ptr<QueryNode>>&)>> m_custom_rules; ///< Custom validation functions
    
    /**
     * @brief Count occurrences of each directive type in the nodes
     *
     * @param nodes Vector of unique pointers to AST nodes
     * @return Map from TokenType to count of occurrences
     */
    std::map<TokenType, int> count_directives(const std::vector<std::unique_ptr<QueryNode>>& nodes);
    
    /**
     * @brief Check that all required directives are present
     *
     * @param counts Map of directive counts
     * @return Vector of validation issues (empty if all required directives present)
     */
    std::vector<ValidationIssue> check_required(const std::map<TokenType, int>& counts);
    
    /**
     * @brief Check that exclusive directives appear at most once
     *
     * @param counts Map of directive counts
     * @return Vector of validation issues (empty if no exclusivity violations)
     */
    std::vector<ValidationIssue> check_exclusive(const std::map<TokenType, int>& counts);
    
    /**
     * @brief Check that dependencies between directives are satisfied
     *
     * @param counts Map of directive counts
     * @return Vector of validation issues (empty if all dependencies satisfied)
     */
    std::vector<ValidationIssue> check_dependencies(const std::map<TokenType, int>& counts);
    
    /**
     * @brief Check that no incompatible directives are present together
     *
     * @param counts Map of directive counts
     * @return Vector of validation issues (empty if no incompatibilities)
     */
    std::vector<ValidationIssue> check_incompatibilities(const std::map<TokenType, int>& counts);
    
    /**
     * @brief Run all custom validation rules
     *
     * @param nodes Vector of unique pointers to AST nodes
     * @return Vector of validation issues from custom rules
     */
    std::vector<ValidationIssue> run_custom_rules(const std::vector<std::unique_ptr<QueryNode>>& nodes);
};

} // namespace cql

#endif // cql_validator_hpp
