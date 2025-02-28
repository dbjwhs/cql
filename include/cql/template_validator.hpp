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
 * @enum TemplateValidationLevel
 * @brief Severity levels for template validation issues
 *
 * Defines the severity of validation issues found in templates:
 * - INFO: Informational messages that don't affect template functionality
 * - WARNING: Potential issues that might affect template behavior
 * - ERROR: Critical issues that will prevent the template from working correctly
 */
enum class TemplateValidationLevel {
    INFO,       ///< Informational message, no action needed
    WARNING,    ///< Warning, template can be used but may have issues
    ERROR       ///< Error, template may not work correctly
};

/**
 * @class TemplateValidationIssue
 * @brief Represents an issue found during template validation
 *
 * A TemplateValidationIssue contains details about a specific problem
 * found in a template, including:
 * - The severity level (INFO, WARNING, ERROR)
 * - A descriptive message explaining the issue
 * - Optional details about the variable or directive causing the issue
 */
class TemplateValidationIssue {
public:
    /**
     * @brief Construct a template validation issue
     *
     * @param level Severity level of the issue
     * @param message Descriptive error message
     * @param variable_name Optional variable name related to the issue
     * @param directive Optional directive name related to the issue
     */
    TemplateValidationIssue(
        TemplateValidationLevel level,
        std::string message,
        std::optional<std::string> variable_name = std::nullopt,
        std::optional<std::string> directive = std::nullopt
    ) : m_level(level), 
        m_message(std::move(message)), 
        m_variable_name(std::move(variable_name)),
        m_directive(std::move(directive)) {}

    /**
     * @brief Get the severity level of the issue
     * @return The TemplateValidationLevel
     */
    TemplateValidationLevel get_level() const { return m_level; }
    
    /**
     * @brief Get the descriptive message
     * @return The message string
     */
    const std::string& get_message() const { return m_message; }
    
    /**
     * @brief Get the variable name if applicable
     * @return Optional containing the variable name or empty
     */
    const std::optional<std::string>& get_variable_name() const { return m_variable_name; }
    
    /**
     * @brief Get the directive name if applicable
     * @return Optional containing the directive name or empty
     */
    const std::optional<std::string>& get_directive() const { return m_directive; }

    /**
     * @brief Format the issue as a string
     * @return Formatted string representation of the issue
     */
    std::string to_string() const;

private:
    TemplateValidationLevel m_level;           ///< Severity level
    std::string m_message;                     ///< Descriptive message
    std::optional<std::string> m_variable_name; ///< Variable name (if applicable)
    std::optional<std::string> m_directive;    ///< Directive name (if applicable)
};

/**
 * @class TemplateValidationResult
 * @brief Container for template validation results
 *
 * Collects and manages multiple validation issues found during template validation.
 * Provides methods to query, filter, and summarize the issues.
 */
class TemplateValidationResult {
public:
    /**
     * @brief Default constructor
     */
    TemplateValidationResult() = default;

    /**
     * @brief Add a validation issue to the results
     * @param issue The validation issue to add
     */
    void add_issue(TemplateValidationIssue issue);
    
    /**
     * @brief Get all validation issues
     * @return Vector of all validation issues
     */
    const std::vector<TemplateValidationIssue>& get_issues() const { return m_issues; }
    
    /**
     * @brief Get issues filtered by severity level
     * @param level The minimum severity level to include
     * @return Vector of filtered validation issues
     */
    std::vector<TemplateValidationIssue> get_issues(TemplateValidationLevel level) const;
    
    /**
     * @brief Check if there are any issues of a specific level or higher
     * @param min_level The minimum severity level to check for
     * @return true if issues exist at specified level, false otherwise
     */
    bool has_issues(TemplateValidationLevel min_level = TemplateValidationLevel::INFO) const;
    
    /**
     * @brief Get the highest severity level found in the results
     * @return The highest TemplateValidationLevel found
     */
    TemplateValidationLevel get_highest_level() const;
    
    /**
     * @brief Count issues with ERROR severity
     * @return Number of error issues
     */
    size_t count_errors() const;
    
    /**
     * @brief Count issues with WARNING severity
     * @return Number of warning issues
     */
    size_t count_warnings() const;
    
    /**
     * @brief Count issues with INFO severity
     * @return Number of info issues
     */
    size_t count_infos() const;
    
    /**
     * @brief Get a formatted summary of validation results
     * @return Formatted string summary of all issues
     */
    std::string get_summary() const;

private:
    std::vector<TemplateValidationIssue> m_issues; ///< Collection of validation issues
};

/**
 * @class TemplateValidator
 * @brief Validates CQL templates for correctness and usability
 *
 * The TemplateValidator performs various checks on templates to ensure they
 * are well-formed and can be used correctly. Checks include:
 * 
 * 1. Variable declaration and usage consistency
 * 2. Required directive presence
 * 3. Inheritance hierarchy correctness
 * 4. Custom rule compliance
 *
 * The validator can be extended with custom validation rules for specific requirements.
 */
class TemplateValidator {
public:
    /**
     * @brief Default constructor
     */
    TemplateValidator() = default;
    
    /**
     * @brief Constructor with template manager
     * @param template_manager Reference to a TemplateManager for loading templates
     */
    explicit TemplateValidator(const TemplateManager& template_manager);

    /**
     * @brief Validate a template by name
     *
     * Loads the template from the template manager and validates it.
     *
     * @param template_name The name of the template to validate
     * @return Validation results containing any issues found
     */
    TemplateValidationResult validate_template(const std::string& template_name);
    
    /**
     * @brief Validate template content directly
     *
     * @param content The template content string to validate
     * @return Validation results containing any issues found
     */
    TemplateValidationResult validate_content(const std::string& content);
    
    /**
     * @brief Validate the inheritance chain of a template
     *
     * Checks for issues in the inheritance hierarchy, such as circular dependencies.
     *
     * @param template_name The name of the template to validate
     * @return Validation results for the inheritance chain
     */
    TemplateValidationResult validate_inheritance(const std::string& template_name);
    
    /**
     * @brief Type alias for custom validation rule functions
     * 
     * A validation rule takes template content and returns any issues found.
     */
    using ValidationRule = std::function<std::vector<TemplateValidationIssue>(const std::string&)>;
    
    /**
     * @brief Add a custom validation rule
     *
     * @param rule The custom validation function
     */
    void add_validation_rule(ValidationRule rule);

private:
    TemplateManager m_template_manager; ///< Template manager for loading templates
    std::vector<ValidationRule> m_validation_rules; ///< Custom validation rules
    
    /**
     * @brief Check variable declaration and usage consistency
     *
     * Ensures all referenced variables are declared and all declared variables are used.
     *
     * @param content The template content to check
     * @return Vector of issues related to variables
     */
    std::vector<TemplateValidationIssue> check_variables(const std::string& content);
    
    /**
     * @brief Check for required directives and directive correctness
     *
     * @param content The template content to check
     * @return Vector of issues related to directives
     */
    std::vector<TemplateValidationIssue> check_directives(const std::string& content);
    
    /**
     * @brief Check for circular dependencies in template inheritance
     *
     * @param template_name The template name to check
     * @return Vector of issues related to inheritance cycles
     */
    std::vector<TemplateValidationIssue> check_inheritance_cycle(const std::string& template_name);
    
    /**
     * @brief Extract all declared variables from template content
     *
     * @param content The template content to analyze
     * @return Set of declared variable names
     */
    std::set<std::string> extract_declared_variables(const std::string& content);
    
    /**
     * @brief Extract all referenced variables from template content
     *
     * @param content The template content to analyze
     * @return Set of referenced variable names
     */
    std::set<std::string> extract_referenced_variables(const std::string& content);
    
    /**
     * @brief Extract all directives from template content
     *
     * @param content The template content to analyze
     * @return Set of directive names
     */
    std::set<std::string> extract_directives(const std::string& content);
};

} // namespace cql

#endif // cql_template_validator_hpp
