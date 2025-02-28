// MIT License
// Copyright (c) 2025 dbjwhs

#include <map>
#include <algorithm>
#include "../../include/cql/validator.hpp"
#include "../../include/cql/lexer.hpp"

namespace cql {

// helper function to get token type for a node
TokenType get_node_type(const QueryNode* node) {
    if (dynamic_cast<const CodeRequestNode*>(node)) {
        // coderequestnode represents both language and description
        // for validation, we need to count it as both
        return TokenType::LANGUAGE; // we'll handle description separately
    }
    if (dynamic_cast<const ContextNode*>(node)) return TokenType::CONTEXT;
    if (dynamic_cast<const TestNode*>(node)) return TokenType::TEST;
    if (dynamic_cast<const DependencyNode*>(node)) return TokenType::DEPENDENCY;
    if (dynamic_cast<const PerformanceNode*>(node)) return TokenType::PERFORMANCE;
    if (dynamic_cast<const CopyrightNode*>(node)) return TokenType::COPYRIGHT;
    if (dynamic_cast<const ArchitectureNode*>(node)) return TokenType::ARCHITECTURE;
    if (dynamic_cast<const ConstraintNode*>(node)) return TokenType::CONSTRAINT;
    if (dynamic_cast<const ExampleNode*>(node)) return TokenType::EXAMPLE;
    if (dynamic_cast<const SecurityNode*>(node)) return TokenType::SECURITY;
    if (dynamic_cast<const ComplexityNode*>(node)) return TokenType::COMPLEXITY;
    if (dynamic_cast<const ModelNode*>(node)) return TokenType::MODEL;
    if (dynamic_cast<const FormatNode*>(node)) return TokenType::FORMAT;
    if (dynamic_cast<const VariableNode*>(node)) return TokenType::VARIABLE;
    
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
    
    // add a custom rule to check if at least one test is specified
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
}

std::map<TokenType, int> QueryValidator::count_directives(const std::vector<std::unique_ptr<QueryNode>>& nodes) {
    std::map<TokenType, int> counts;
    
    for (const auto& node : nodes) {
        // special handling for coderequestnode which contains both language and description
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

std::vector<ValidationIssue> QueryValidator::check_required(const std::map<TokenType, int>& counts) {
    std::vector<ValidationIssue> issues;
    
    for (auto directive : m_required_directives) {
        if (counts.find(directive) == counts.end() || counts.at(directive) == 0) {
            issues.emplace_back(
                ValidationSeverity::ERROR,
                "Required directive @" + token_type_to_string(directive) + " is missing."
            );
        }
    }
    
    return issues;
}

std::vector<ValidationIssue> QueryValidator::check_exclusive(const std::map<TokenType, int>& counts) {
    std::vector<ValidationIssue> issues;
    
    for (auto directive : m_exclusive_directives) {
        if (counts.find(directive) != counts.end() && counts.at(directive) > 1) {
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
        if (counts.find(dependent) != counts.end() && counts.at(dependent) > 0) {
            if (counts.find(dependency) == counts.end() || counts.at(dependency) == 0) {
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
        if (counts.find(directive_a) != counts.end() && counts.at(directive_a) > 0 &&
            counts.find(directive_b) != counts.end() && counts.at(directive_b) > 0) {
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

std::vector<ValidationIssue> QueryValidator::run_custom_rules(const std::vector<std::unique_ptr<QueryNode>>& nodes) {
    std::vector<ValidationIssue> issues;
    
    for (const auto& rule : m_custom_rules) {
        std::optional<ValidationIssue> issue = rule(nodes);
        if (issue) {
            issues.push_back(*issue);
        }
    }
    
    return issues;
}

std::vector<ValidationIssue> QueryValidator::validate(const std::vector<std::unique_ptr<QueryNode>>& nodes) {
    std::vector<ValidationIssue> issues;
    
    // count directive occurrences
    std::map<TokenType, int> counts = count_directives(nodes);
    
    // run all validation checks
    auto required_issues = check_required(counts);
    auto exclusive_issues = check_exclusive(counts);
    auto dependency_issues = check_dependencies(counts);
    auto incompatibility_issues = check_incompatibilities(counts);
    auto custom_issues = run_custom_rules(nodes);
    
    // combine all issues
    issues.insert(issues.end(), required_issues.begin(), required_issues.end());
    issues.insert(issues.end(), exclusive_issues.begin(), exclusive_issues.end());
    issues.insert(issues.end(), dependency_issues.begin(), dependency_issues.end());
    issues.insert(issues.end(), incompatibility_issues.begin(), incompatibility_issues.end());
    issues.insert(issues.end(), custom_issues.begin(), custom_issues.end());
    
    // throw ValidationException if there are any ERROR issues
    for (const auto& issue : issues) {
        if (issue.severity == ValidationSeverity::ERROR) {
            throw ValidationException(issue.message);
        }
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
