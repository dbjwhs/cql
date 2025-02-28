// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_TEMPLATE_VALIDATOR_SCHEMA_HPP
#define CQL_TEMPLATE_VALIDATOR_SCHEMA_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <regex>
#include "template_validator.hpp"

namespace cql {

/**
 * Class that defines the schema and validation rules for templates
 */
class TemplateValidatorSchema {
public:
    TemplateValidatorSchema() = default;
    
    /**
     * Directive schema definition
     */
    struct DirectiveSchema {
        std::string name;                // Directive name (e.g., "@copyright")
        bool required;                   // Whether the directive is required
        int max_occurrences;             // Maximum number of occurrences (0 = unlimited)
        std::vector<std::string> dependencies;  // Other directives this depends on
        std::vector<std::string> incompatible;  // Directives that cannot appear with this one
        std::regex format;               // Format validation regex (if empty, no format validation)
        std::string description;         // Human-readable description

        // Default constructor required for map operations
        DirectiveSchema() : 
            name(""), 
            required(false), 
            max_occurrences(0), 
            format(std::regex()) {}

        DirectiveSchema(
            const std::string& name,
            bool required = false,
            int max_occurrences = 0,
            const std::vector<std::string>& dependencies = {},
            const std::vector<std::string>& incompatible = {},
            const std::string& format_regex = "",
            const std::string& description = ""
        );
    };

    // Register a directive schema
    void register_directive(const DirectiveSchema& schema);
    
    // Get directive schema by name
    std::optional<DirectiveSchema> get_directive_schema(const std::string& name) const;
    
    // Get all registered directive schemas
    const std::map<std::string, DirectiveSchema>& get_all_directives() const;
    
    // Get all required directives
    std::vector<std::string> get_required_directives() const;
    
    // Add a custom validation rule
    using ValidationRule = std::function<std::vector<TemplateValidationIssue>(const std::string&)>;
    void add_validation_rule(const std::string& name, ValidationRule rule);
    
    // Get all validation rules
    const std::map<std::string, ValidationRule>& get_validation_rules() const;

    // Create a default schema with standard directives and rules
    static TemplateValidatorSchema create_default_schema();

private:
    std::map<std::string, DirectiveSchema> m_directives;
    std::map<std::string, ValidationRule> m_validation_rules;
};

} // namespace cql

#endif // CQL_TEMPLATE_VALIDATOR_SCHEMA_HPP
