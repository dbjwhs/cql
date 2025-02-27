// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_VALIDATOR_HPP
#define CQL_VALIDATOR_HPP

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <map>
#include "nodes.hpp"
#include "lexer.hpp"

namespace cql {

/**
 * Severity levels for validation issues
 */
enum class ValidationSeverity {
    INFO,       // Informational message
    WARNING,    // Non-fatal warning
    ERROR       // Fatal error
};

/**
 * Structure to hold validation issues
 */
struct ValidationIssue {
    ValidationSeverity severity;
    std::string message;
    
    ValidationIssue(ValidationSeverity sev, std::string msg)
        : severity(sev), message(std::move(msg)) {}
};

/**
 * Rule type for validation
 */
enum class RuleType {
    REQUIRED,       // Element must be present
    EXCLUSIVE,      // Only one allowed
    DEPENDENCY,     // If A exists, B must exist
    INCOMPATIBLE,   // If A exists, B must not exist
    FORMAT          // Content format validation
};

/**
 * Validator class to check query structure and content
 */
class QueryValidator {
public:
    QueryValidator();
    
    // Run validation on parsed nodes
    std::vector<ValidationIssue> validate(const std::vector<std::unique_ptr<QueryNode>>& nodes);
    
    // Configure validation rules
    void configure_required(const std::vector<TokenType>& required_directives);
    void configure_exclusive(const std::vector<TokenType>& exclusive_directives);
    void configure_dependency(TokenType dependent, TokenType dependency);
    void configure_incompatible(TokenType a, TokenType b);
    
    // Add custom validation rule
    void add_custom_rule(std::function<std::optional<ValidationIssue>(const std::vector<std::unique_ptr<QueryNode>>&)> rule);
    
private:
    // Configuration storage
    std::vector<TokenType> m_required_directives;
    std::vector<TokenType> m_exclusive_directives;
    std::vector<std::pair<TokenType, TokenType>> m_dependency_rules;
    std::vector<std::pair<TokenType, TokenType>> m_incompatibility_rules;
    std::vector<std::function<std::optional<ValidationIssue>(const std::vector<std::unique_ptr<QueryNode>>&)>> m_custom_rules;
    
    // Count occurrences of directive types
    std::map<TokenType, int> count_directives(const std::vector<std::unique_ptr<QueryNode>>& nodes);
    
    // Helper methods for validation
    std::vector<ValidationIssue> check_required(const std::map<TokenType, int>& counts);
    std::vector<ValidationIssue> check_exclusive(const std::map<TokenType, int>& counts);
    std::vector<ValidationIssue> check_dependencies(const std::map<TokenType, int>& counts);
    std::vector<ValidationIssue> check_incompatibilities(const std::map<TokenType, int>& counts);
    std::vector<ValidationIssue> run_custom_rules(const std::vector<std::unique_ptr<QueryNode>>& nodes);
};

} // namespace cql

#endif // CQL_VALIDATOR_HPP