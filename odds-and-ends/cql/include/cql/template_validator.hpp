// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_TEMPLATE_VALIDATOR_HPP
#define CQL_TEMPLATE_VALIDATOR_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <functional>
#include "template_manager.hpp"
#include <regex>

namespace cql {

/**
 * validation issue levels for template validation
 */
enum class TemplateValidationLevel {
    INFO,       // informational message, no action needed
    WARNING,    // warning, template can be used but may have issues
    ERROR       // error, template may not work correctly
};

/**
 * class to represent a template validation issue
 */
class TemplateValidationIssue {
public:
    TemplateValidationIssue(
        TemplateValidationLevel level,
        std::string message,
        std::optional<std::string> variable_name = std::nullopt,
        std::optional<std::string> directive = std::nullopt
    ) : m_level(level), 
        m_message(std::move(message)), 
        m_variable_name(std::move(variable_name)),
        m_directive(std::move(directive)) {}

    // getters
    TemplateValidationLevel get_level() const { return m_level; }
    const std::string& get_message() const { return m_message; }
    const std::optional<std::string>& get_variable_name() const { return m_variable_name; }
    const std::optional<std::string>& get_directive() const { return m_directive; }

    // format the issue as a string
    std::string to_string() const;

private:
    TemplateValidationLevel m_level;
    std::string m_message;
    std::optional<std::string> m_variable_name;
    std::optional<std::string> m_directive;
};

/**
 * container for template validation results
 */
class TemplateValidationResult {
public:
    TemplateValidationResult() = default;

    // add an issue to the results
    void add_issue(TemplateValidationIssue issue);
    
    // get all issues
    const std::vector<TemplateValidationIssue>& get_issues() const { return m_issues; }
    
    // get issues filtered by level
    std::vector<TemplateValidationIssue> get_issues(TemplateValidationLevel level) const;
    
    // check if there are any issues of a specific level or higher
    bool has_issues(TemplateValidationLevel min_level = TemplateValidationLevel::INFO) const;
    
    // get the highest severity level found
    TemplateValidationLevel get_highest_level() const;
    
    // count issues of each level
    size_t count_errors() const;
    size_t count_warnings() const;
    size_t count_infos() const;
    
    // return a formatted summary of issues
    std::string get_summary() const;

private:
    std::vector<TemplateValidationIssue> m_issues;
};

/**
 * class for validating cql templates
 */
class TemplateValidator {
public:
    TemplateValidator() = default;
    explicit TemplateValidator(const TemplateManager& template_manager);

    // validate a template by name
    TemplateValidationResult validate_template(const std::string& template_name);
    
    // validate a template from content
    TemplateValidationResult validate_content(const std::string& content);
    
    // validate inheritance chain
    TemplateValidationResult validate_inheritance(const std::string& template_name);
    
    // add a custom validation rule
    using ValidationRule = std::function<std::vector<TemplateValidationIssue>(const std::string&)>;
    void add_validation_rule(ValidationRule rule);

private:
    TemplateManager m_template_manager;
    std::vector<ValidationRule> m_validation_rules;
    
    // core validation checks
    std::vector<TemplateValidationIssue> check_variables(const std::string& content);
    std::vector<TemplateValidationIssue> check_directives(const std::string& content);
    std::vector<TemplateValidationIssue> check_inheritance_cycle(const std::string& template_name);
    
    // helper methods
    std::set<std::string> extract_declared_variables(const std::string& content);
    std::set<std::string> extract_referenced_variables(const std::string& content);
    std::set<std::string> extract_directives(const std::string& content);
};

} // namespace cql

#endif // cql_template_validator_hpp
