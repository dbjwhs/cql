// MIT License
// Copyright (c) 2025 dbjwhs

#include <map>
#include <algorithm>
#include "../../include/cql/validator.hpp"
#include "../../include/cql/lexer.hpp"
#include "../../include/cql/pattern_compatibility.hpp"

namespace cql {

// helper function to get token type for a node
TokenType get_node_type(const QueryNode* node) {
    if (dynamic_cast<const CodeRequestNode*>(node)) {
        // CodeRequestNode represents both language and description
        // for validation, we need to count it as both
        return TokenType::LANGUAGE; // we'll handle the description separately
    }
    if (dynamic_cast<const ContextNode*>(node))      return TokenType::CONTEXT;
    if (dynamic_cast<const TestNode*>(node))         return TokenType::TEST;
    if (dynamic_cast<const DependencyNode*>(node))   return TokenType::DEPENDENCY;
    if (dynamic_cast<const PerformanceNode*>(node))  return TokenType::PERFORMANCE;
    if (dynamic_cast<const CopyrightNode*>(node))    return TokenType::COPYRIGHT;
    if (dynamic_cast<const ArchitectureNode*>(node)) return TokenType::ARCHITECTURE;
    if (dynamic_cast<const ConstraintNode*>(node))   return TokenType::CONSTRAINT;
    if (dynamic_cast<const ExampleNode*>(node))      return TokenType::EXAMPLE;
    if (dynamic_cast<const SecurityNode*>(node))     return TokenType::SECURITY;
    if (dynamic_cast<const ComplexityNode*>(node))   return TokenType::COMPLEXITY;
    if (dynamic_cast<const ModelNode*>(node))        return TokenType::MODEL;
    if (dynamic_cast<const FormatNode*>(node))       return TokenType::FORMAT;
    if (dynamic_cast<const VariableNode*>(node))     return TokenType::VARIABLE;
    
    // default fallback
    return TokenType::IDENTIFIER;
}

QueryValidator::QueryValidator() {
    // set up default rules
    
    // language, description, and copyright are always required
    m_required_directives = {
        TokenType::LANGUAGE,
        TokenType::DESCRIPTION,
        TokenType::COPYRIGHT
    };
    
    // model and format are exclusive (only one allowed)
    m_exclusive_directives = {
        TokenType::MODEL,
        TokenType::FORMAT
    };
    
    // example dependency rule: if using architecture, should have context
    m_dependency_rules.emplace_back(TokenType::ARCHITECTURE, TokenType::CONTEXT);
    
    // example incompatibility: example and constraint don't make sense together
    // (this is just for demonstration)
    //m_incompatibility_rules.emplace_back(tokentype::example, tokentype::constraint);

    // the add_custome_rule() methods adds a custom validation rule that checks if any test nodes exist in the query node
    // list returns a warning ValidationIssue if no test cases are found, suggesting to add @test directives
    // Returns std::nullopt (no issue) if at least one TestNode is present
    
    // add a custom rule to check if at least one test is specified...
    add_custom_rule([](const std::vector<std::unique_ptr<QueryNode>>& nodes) -> std::optional<ValidationIssue> {
        bool has_test = false;
        for (const auto& node : nodes) {
            if (dynamic_cast<const TestNode*>(node.get())) {
                has_test = true;
                break;
            }
        }
        
        if (!has_test) {
            return ValidationIssue(
                ValidationSeverity::WARNING,
                "No test cases specified. Consider adding tests with @test directive."
            );
        }
        
        return std::nullopt;
    });
    
    // Add a custom rule to validate architecture pattern compatibility
    add_custom_rule([](const std::vector<std::unique_ptr<QueryNode>>& nodes) -> std::optional<ValidationIssue> {
        // Collect all architecture nodes
        std::vector<const ArchitectureNode*> architecture_nodes;
        for (const auto& node : nodes) {
            if (auto* arch_node = dynamic_cast<const ArchitectureNode*>(node.get())) {
                architecture_nodes.push_back(arch_node);
            }
        }
        
        // Skip validation if fewer than 2 architecture patterns
        if (architecture_nodes.size() < 2) {
            return std::nullopt;
        }
        
        // Check pattern compatibility
        PatternCompatibilityManager compatibility_manager;
        auto issues = compatibility_manager.check_compatibility(architecture_nodes);
        
        // Return a validation issue if incompatibilities are found
        if (!issues.empty()) {
            std::string message = "Architecture pattern compatibility issues found: ";
            for (size_t ndx = 0; ndx < issues.size(); ++ndx) {
                if (ndx > 0) message += "; ";
                message += issues[ndx].to_string();
            }
            
            return ValidationIssue(
                ValidationSeverity::WARNING, // Use WARNING to provide guidance but not prevent code generation
                message
            );
        }
        
        return std::nullopt;
    });
}

std::map<TokenType, int> QueryValidator::count_directives(const std::vector<std::unique_ptr<QueryNode>>& nodes) {
    std::map<TokenType, int> counts;
    
    for (const auto& node : nodes) {
        // special handling for CodeRequestNode which contains both language and description
        if (auto* code_node = dynamic_cast<const CodeRequestNode*>(node.get())) {
            // Only count language if it's not empty
            if (!code_node->language().empty()) {
                counts[TokenType::LANGUAGE]++;
            }
            counts[TokenType::DESCRIPTION]++;
        } else {
            TokenType type = get_node_type(node.get());
            counts[type]++;
        }
    }
    
    return counts;
}

std::vector<ValidationIssue> QueryValidator::check_required(const std::map<TokenType, int>& counts) const {
    std::vector<ValidationIssue> issues;
    
    for (auto directive : m_required_directives) {
        if (!counts.contains(directive) || counts.at(directive) == 0) {
            issues.emplace_back(
                ValidationSeverity::ERROR,
                "Required directive @" + token_type_to_string(directive) + " is missing."
            );
        }
    }
    
    return issues;
}

std::vector<ValidationIssue> QueryValidator::check_exclusive(const std::map<TokenType, int>& counts) const {
    std::vector<ValidationIssue> issues;
    
    for (auto directive : m_exclusive_directives) {
        if (counts.contains(directive) && counts.at(directive) > 1) {
            issues.emplace_back(
                ValidationSeverity::WARNING,
                "Multiple @" + token_type_to_string(directive) + " directives found. Only the last one will be used."
            );
        }
    }
    
    return issues;
}

std::vector<ValidationIssue> QueryValidator::check_dependencies(const std::map<TokenType, int>& counts) {
    std::vector<ValidationIssue> issues;
    
    for (const auto& [dependent, dependency] : m_dependency_rules) {
        if (counts.contains(dependent) && counts.at(dependent) > 0) {
            if (!counts.contains(dependency) || counts.at(dependency) == 0) {
                issues.emplace_back(
                    ValidationSeverity::WARNING,
                    "Directive @" + token_type_to_string(dependent) + 
                    " works best with @" + token_type_to_string(dependency) + "."
                );
            }
        }
    }
    
    return issues;
}

std::vector<ValidationIssue> QueryValidator::check_incompatibilities(const std::map<TokenType, int>& counts) {
    std::vector<ValidationIssue> issues;
    
    for (const auto& [directive_a, directive_b] : m_incompatibility_rules) {
        if (counts.contains(directive_a) && counts.at(directive_a) > 0 &&
            counts.contains(directive_b) && counts.at(directive_b) > 0) {
            issues.emplace_back(
                ValidationSeverity::WARNING,
                "Directives @" + token_type_to_string(directive_a) + 
                " and @" + token_type_to_string(directive_b) + 
                " may conflict with each other."
            );
        }
    }
    
    return issues;
}

std::vector<ValidationIssue> QueryValidator::run_custom_rules(const std::vector<std::unique_ptr<QueryNode>>& nodes) const {
    std::vector<ValidationIssue> issues;
    
    for (const auto& rule : m_custom_rules) {
        if (std::optional<ValidationIssue> issue = rule(nodes)) {
            issues.push_back(*issue);
        }
    }
    
    return issues;
}

/**
 * Validate the query structure and content
 * 
 * This method performs comprehensive validation of the query nodes,
 * checking for required directives, exclusive directives, dependencies,
 * incompatibilities, and custom validation rules.
 * 
 * If any ERROR-level issues are found, it will throw a ValidationException.
 * Otherwise, it returns a list of all validation issues for reporting.
 * 
 * @param nodes The query nodes to validate
 * @return Vector of validation issues (warnings and infos)
 * @throws ValidationException if any ERROR-level issues are found
 */
std::vector<ValidationIssue> QueryValidator::validate(const std::vector<std::unique_ptr<QueryNode>>& nodes) {
    std::vector<ValidationIssue> issues;
    
    // Count directive occurrences - this handles empty nodes gracefully
    const std::map<TokenType, int> counts = count_directives(nodes);
    
    // Run all validation checks
    auto required_issues = check_required(counts);
    auto exclusive_issues = check_exclusive(counts);
    auto dependency_issues = check_dependencies(counts);
    auto incompatibility_issues = check_incompatibilities(counts);
    auto custom_issues = run_custom_rules(nodes);
    
    // Combine all issues, prioritizing required directive issues first
    issues.insert(issues.end(), required_issues.begin(), required_issues.end());
    issues.insert(issues.end(), exclusive_issues.begin(), exclusive_issues.end());
    issues.insert(issues.end(), dependency_issues.begin(), dependency_issues.end());
    issues.insert(issues.end(), incompatibility_issues.begin(), incompatibility_issues.end());
    issues.insert(issues.end(), custom_issues.begin(), custom_issues.end());
    
    // Throw ValidationException if there are any ERROR issues
    // We collect all errors first instead of throwing on the first one
    std::vector<std::string> error_messages;
    for (const auto& issue : issues) {
        if (issue.severity == ValidationSeverity::ERROR) {
            error_messages.push_back(issue.message);
        }
    }
    
    if (!error_messages.empty()) {
        // Join all error messages into a single error message
        std::string combined_message;
        for (size_t ndx = 0; ndx < error_messages.size(); ++ndx) {
            if (ndx > 0) {
                combined_message += "; ";
            }
            combined_message += error_messages[ndx];
        }
        
        // Determine the appropriate error code
        std::string error_code = validation_errors::GENERAL_ERROR;
        
        // Use the MULTIPLE_ERRORS code if there are multiple errors
        if (error_messages.size() > 1) {
            error_code = validation_errors::MULTIPLE_ERRORS;
        }
        // Otherwise, use a specific code based on the directive that's missing
        else if (combined_message.find("Required directive @LANGUAGE") != std::string::npos) {
            error_code = validation_errors::MISSING_LANGUAGE;
        }
        else if (combined_message.find("Required directive @DESCRIPTION") != std::string::npos) {
            error_code = validation_errors::MISSING_DESCRIPTION;
        }
        else if (combined_message.find("Required directive @COPYRIGHT") != std::string::npos) {
            error_code = validation_errors::MISSING_COPYRIGHT;
        }
        
        throw ValidationException(combined_message, error_code);
    }
    
    return issues;
}

void QueryValidator::configure_required(const std::vector<TokenType>& required_directives) {
    m_required_directives = required_directives;
}

void QueryValidator::configure_exclusive(const std::vector<TokenType>& exclusive_directives) {
    m_exclusive_directives = exclusive_directives;
}

void QueryValidator::configure_dependency(TokenType dependent, TokenType dependency) {
    m_dependency_rules.emplace_back(dependent, dependency);
}

void QueryValidator::configure_incompatible(TokenType a, TokenType b) {
    m_incompatibility_rules.emplace_back(a, b);
}

void QueryValidator::add_custom_rule(std::function<std::optional<ValidationIssue>(const std::vector<std::unique_ptr<QueryNode>>&)> rule) {
    m_custom_rules.push_back(std::move(rule));
}

} // namespace cql
