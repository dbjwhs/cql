// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/template_validator.hpp"
#include "../../include/cql/template_validator_schema.hpp"
#include "../../../headers/project_utils.hpp"

#include <regex>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include "../../include/cql/cql.hpp" // for util namespace

namespace cql {

// templatevalidationissue implementation
std::string TemplateValidationIssue::to_string() const {
    std::stringstream ss;
    
    // add level prefix
    switch (m_level) {
        case TemplateValidationLevel::ERROR:
            ss << "ERROR: ";
            break;
        case TemplateValidationLevel::WARNING:
            ss << "WARNING: ";
            break;
        case TemplateValidationLevel::INFO:
            ss << "INFO: ";
            break;
    }
    
    // add main message
    ss << m_message;
    
    // add variable name if available
    if (m_variable_name.has_value()) {
        ss << " [Variable: " << *m_variable_name << "]";
    }
    
    // add directive if available
    if (m_directive.has_value()) {
        ss << " [Directive: " << *m_directive << "]";
    }
    
    return ss.str();
}

// templatevalidationresult implementation
void TemplateValidationResult::add_issue(TemplateValidationIssue issue) {
    m_issues.push_back(std::move(issue));
}

std::vector<TemplateValidationIssue> TemplateValidationResult::get_issues(TemplateValidationLevel level) const {
    std::vector<TemplateValidationIssue> filtered;
    
    std::copy_if(m_issues.begin(), m_issues.end(), std::back_inserter(filtered),
                [level](const TemplateValidationIssue& issue) {
                    return issue.get_level() == level;
                });
    
    return filtered;
}

bool TemplateValidationResult::has_issues(TemplateValidationLevel min_level) const {
    return std::any_of(m_issues.begin(), m_issues.end(),
                      [min_level](const TemplateValidationIssue& issue) {
                          return static_cast<int>(issue.get_level()) >= static_cast<int>(min_level);
                      });
}

TemplateValidationLevel TemplateValidationResult::get_highest_level() const {
    if (m_issues.empty()) {
        return TemplateValidationLevel::INFO;
    }
    
    return std::max_element(m_issues.begin(), m_issues.end(),
                           [](const TemplateValidationIssue& a, const TemplateValidationIssue& b) {
                               return static_cast<int>(a.get_level()) < static_cast<int>(b.get_level());
                           })->get_level();
}

size_t TemplateValidationResult::count_errors() const {
    return get_issues(TemplateValidationLevel::ERROR).size();
}

size_t TemplateValidationResult::count_warnings() const {
    return get_issues(TemplateValidationLevel::WARNING).size();
}

size_t TemplateValidationResult::count_infos() const {
    return get_issues(TemplateValidationLevel::INFO).size();
}

std::string TemplateValidationResult::get_summary() const {
    std::stringstream ss;
    
    size_t error_count = count_errors();
    size_t warning_count = count_warnings();
    size_t info_count = count_infos();
    
    ss << "Template validation summary: ";
    
    if (error_count == 0 && warning_count == 0 && info_count == 0) {
        ss << "No issues found.";
        return ss.str();
    }
    
    ss << error_count << " error(s), " << warning_count << " warning(s), " << info_count << " info message(s)";
    
    // use a structure to iterate through all issue levels with their counts
    const struct {
        TemplateValidationLevel level;
        size_t count;
        std::string label;
    } issue_types[] = {
        {TemplateValidationLevel::ERROR, error_count, "Errors"},
        {TemplateValidationLevel::WARNING, warning_count, "Warnings"},
        {TemplateValidationLevel::INFO, info_count, "Info"}
    };
    
    for (const auto& type : issue_types) {
        if (type.count > 0) {
            ss << "\n\n" << type.label << ":";
            for (const auto& issue : get_issues(type.level)) {
                ss << "\n- " << issue.get_message();
            }
        }
    }
    
    return ss.str();
}

// templatevalidator implementation
TemplateValidator::TemplateValidator(const TemplateManager& template_manager)
    : m_template_manager(template_manager) {
}

TemplateValidationResult TemplateValidator::validate_template(const std::string& template_name) {
    try {
        // load template content
        std::string content = m_template_manager.load_template(template_name);
        
        // start with content validation
        TemplateValidationResult result = validate_content(content);
        
        // add inheritance validation if applicable
        auto inheritance_result = validate_inheritance(template_name);
        for (const auto& issue : inheritance_result.get_issues()) {
            result.add_issue(issue);
        }
        
        return result;
    } catch (const std::exception& e) {
        TemplateValidationResult result;
        result.add_issue(TemplateValidationIssue(
            TemplateValidationLevel::ERROR,
            std::string("Failed to validate template: ") + e.what()
        ));
        return result;
    }
}

TemplateValidationResult TemplateValidator::validate_content(const std::string& content) {
    TemplateValidationResult result;
    
    // check variables
    for (const auto& issue : check_variables(content)) {
        result.add_issue(issue);
    }
    
    // check directives
    for (const auto& issue : check_directives(content)) {
        result.add_issue(issue);
    }
    
    // run custom validation rules
    for (const auto& rule : m_validation_rules) {
        for (const auto& issue : rule(content)) {
            result.add_issue(issue);
        }
    }
    
    return result;
}

TemplateValidationResult TemplateValidator::validate_inheritance(const std::string& template_name) {
    TemplateValidationResult result;
    
    // check for inheritance cycles
    for (const auto& issue : check_inheritance_cycle(template_name)) {
        result.add_issue(issue);
    }
    
    // get inheritance chain
    try {
        auto chain = m_template_manager.get_inheritance_chain(template_name);
        
        // validate each template in the chain
        if (chain.size() > 1) {
            // add info about inheritance chain
            std::stringstream chain_info;
            chain_info << "Template inherits from " << (chain.size() - 1) << " parent template(s): ";
            for (size_t i = 0; i < chain.size() - 1; ++i) {
                if (i > 0) chain_info << ", ";
                chain_info << chain[i];
            }
            
            result.add_issue(TemplateValidationIssue(
                TemplateValidationLevel::WARNING,
                chain_info.str()
            ));
            
            // check that all parent templates exist and are valid
            for (size_t i = 0; i < chain.size() - 1; ++i) {
                try {
                    std::string parent_content = m_template_manager.load_template(chain[i]);
                    
                    // validate parent content but ignore inheritance validation to avoid recursion
                    for (const auto& issue : check_variables(parent_content)) {
                        TemplateValidationIssue parent_issue(
                            issue.get_level(),
                            "In parent template '" + chain[i] + "': " + issue.get_message(),
                            issue.get_variable_name(),
                            issue.get_directive()
                        );
                        result.add_issue(parent_issue);
                    }
                    
                    for (const auto& issue : check_directives(parent_content)) {
                        TemplateValidationIssue parent_issue(
                            issue.get_level(),
                            "In parent template '" + chain[i] + "': " + issue.get_message(),
                            issue.get_variable_name(),
                            issue.get_directive()
                        );
                        result.add_issue(parent_issue);
                    }
                } catch (const std::exception& e) {
                    result.add_issue(TemplateValidationIssue(
                        TemplateValidationLevel::ERROR,
                        "Failed to validate parent template '" + chain[i] + "': " + e.what()
                    ));
                }
            }
        }
    } catch (const std::exception& e) {
        // only add an error if it's not already caught by check_inheritance_cycle
        if (result.count_errors() == 0) {
            result.add_issue(TemplateValidationIssue(
                TemplateValidationLevel::ERROR,
                std::string("Failed to validate inheritance chain: ") + e.what()
            ));
        }
    }
    
    return result;
}

void TemplateValidator::add_validation_rule(ValidationRule rule) {
    m_validation_rules.push_back(std::move(rule));
}

std::vector<TemplateValidationIssue> TemplateValidator::check_variables(const std::string& content) {
    std::vector<TemplateValidationIssue> issues;
    
    // extract variables
    auto declared_vars = extract_declared_variables(content);
    auto referenced_vars = extract_referenced_variables(content);
    
    // check for undeclared variables
    for (const auto& var : referenced_vars) {
        if (declared_vars.find(var) == declared_vars.end()) {
            issues.push_back(TemplateValidationIssue(
                TemplateValidationLevel::WARNING,
                "Referenced variable is not declared in the template",
                var
            ));
        }
    }
    
    // check for unused variables
    for (const auto& var : declared_vars) {
        if (referenced_vars.find(var) == referenced_vars.end()) {
            issues.push_back(TemplateValidationIssue(
                TemplateValidationLevel::WARNING,
                "Declared variable is not used in the template",
                var
            ));
        }
    }
    
    return issues;
}

std::vector<TemplateValidationIssue> TemplateValidator::check_directives(const std::string& content) {
    std::vector<TemplateValidationIssue> issues;
    
    // extract all directives
    auto directives = extract_directives(content);
    
    // check for essential directives
    static const std::unordered_set<std::string> essential_directives = {
        "@description"
    };
    
    for (const auto& directive : essential_directives) {
        if (directives.find(directive) == directives.end()) {
            issues.push_back(TemplateValidationIssue(
                TemplateValidationLevel::WARNING,
                "Essential directive is missing",
                std::nullopt,
                directive
            ));
        }
    }
    
    // check for uncommon or potentially incorrect directives
    static const std::unordered_set<std::string> common_directives = {
        "@copyright", "@language", "@description", "@context", "@dependency", 
        "@test", "@architecture", "@constraint", "@security", "@complexity",
        "@example", "@variable", "@inherit"
    };
    
    for (const auto& directive : directives) {
        if (common_directives.find(directive) == common_directives.end()) {
            issues.push_back(TemplateValidationIssue(
                TemplateValidationLevel::ERROR,
                "Invalid directive found: " + directive,
                std::nullopt,
                directive
            ));
        }
    }
    
    return issues;
}

std::vector<TemplateValidationIssue> TemplateValidator::check_inheritance_cycle(const std::string& template_name) {
    std::vector<TemplateValidationIssue> issues;
    
    try {
        // try to get inheritance chain, which will throw if there's a cycle
        m_template_manager.get_inheritance_chain(template_name);
    } catch (const std::exception& e) {
        std::string error_msg = e.what();
        
        // check if it's a circular inheritance error
        if (error_msg.find("circular") != std::string::npos || 
            error_msg.find("cycle") != std::string::npos) {
            issues.push_back(TemplateValidationIssue(
                TemplateValidationLevel::ERROR,
                "Circular inheritance detected: " + error_msg
            ));
        }
    }
    
    return issues;
}

std::set<std::string> TemplateValidator::extract_declared_variables(const std::string& content) {
    return cql::util::extract_regex_group_values(
        content,
        "@variable\\s+\"([^\"]*)\"\\s+\"[^\"]*\"",
        1
    );
}

std::set<std::string> TemplateValidator::extract_referenced_variables(const std::string& content) {
    return cql::util::extract_regex_group_values(
        content,
        "\\$\\{([^}]+)\\}",
        1
    );
}

std::set<std::string> TemplateValidator::extract_directives(const std::string& content) {
    return cql::util::extract_regex_group_values(
        content,
        "^(@[a-zA-Z_]+)\\s+",
        1
    );
}

} // namespace cql
