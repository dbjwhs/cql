// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/template_validator_schema.hpp"
#include "../../cpp-snippets/headers/project_utils.hpp"

namespace cql {

// directiveschema constructor
TemplateValidatorSchema::DirectiveSchema::DirectiveSchema(
    const std::string& name,
    bool required,
    int max_occurrences,
    const std::vector<std::string>& dependencies,
    const std::vector<std::string>& incompatible,
    const std::string& format_regex,
    const std::string& description
) : name(name),
    required(required),
    max_occurrences(max_occurrences),
    dependencies(dependencies),
    incompatible(incompatible),
    format(format_regex.empty() ? std::regex() : std::regex(format_regex)),
    description(description) {
}

// register a directive schema
void TemplateValidatorSchema::register_directive(const DirectiveSchema& schema) {
    m_directives[schema.name] = schema;
}

// get directive schema by name
std::optional<TemplateValidatorSchema::DirectiveSchema> 
TemplateValidatorSchema::get_directive_schema(const std::string& name) const {
    auto it = m_directives.find(name);
    if (it != m_directives.end()) {
        return it->second;
    }
    return std::nullopt;
}

// get all registered directive schemas
const std::map<std::string, TemplateValidatorSchema::DirectiveSchema>& 
TemplateValidatorSchema::get_all_directives() const {
    return m_directives;
}

// get all required directives
std::vector<std::string> TemplateValidatorSchema::get_required_directives() const {
    std::vector<std::string> required_directives;
    
    for (const auto& [name, schema] : m_directives) {
        if (schema.required) {
            required_directives.push_back(name);
        }
    }
    
    return required_directives;
}

// add a custom validation rule
void TemplateValidatorSchema::add_validation_rule(const std::string& name, ValidationRule rule) {
    m_validation_rules[name] = std::move(rule);
}

// get all validation rules
const std::map<std::string, TemplateValidatorSchema::ValidationRule>& 
TemplateValidatorSchema::get_validation_rules() const {
    return m_validation_rules;
}

// create a default schema with standard directives and rules
TemplateValidatorSchema TemplateValidatorSchema::create_default_schema() {
    TemplateValidatorSchema schema;
    
    // register directives
    schema.register_directive(DirectiveSchema(
        "@description",
        true,  // required
        1,     // max occurrences
        {},    // dependencies
        {},    // incompatible
        "^@description\\s+\"[^\"]{1,}\"$",  // format
        "Main description of the template"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@copyright",
        false,
        1,
        {},
        {},
        "^@copyright\\s+\"[^\"]+\"\\s+\"[^\"]+\"$",
        "Copyright information in the format: @copyright \"LICENSE\" \"OWNER\""
    ));
    
    schema.register_directive(DirectiveSchema(
        "@language",
        false,
        1,
        {},
        {},
        "^@language\\s+\"[^\"]+\"$",
        "Programming language for the implementation"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@context",
        false,
        0,  // unlimited
        {},
        {},
        "^@context\\s+\"[^\"]+\"$",
        "Additional context for the implementation"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@dependency",
        false,
        0,
        {},
        {},
        "^@dependency\\s+\"[^\"]+\"$",
        "External dependencies required"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@test",
        false,
        0,
        {},
        {},
        "^@test\\s+\"[^\"]+\"$",
        "Test cases to verify implementation"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@architecture",
        false,
        1,
        {},
        {},
        "^@architecture\\s+\"[^\"]+\"$",
        "Architectural pattern for implementation"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@constraint",
        false,
        0,
        {},
        {},
        "^@constraint\\s+\"[^\"]+\"$",
        "Constraints or requirements for implementation"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@security",
        false,
        0,
        {},
        {},
        "^@security\\s+\"[^\"]+\"$",
        "Security considerations"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@complexity",
        false,
        1,
        {},
        {},
        "^@complexity\\s+\"[^\"]+\"$",
        "Time/space complexity information"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@example",
        false,
        0,
        {},
        {},
        "^@example\\s+\"[^\"]+\"\\s+\"[^\"]+\"$",
        "Example usage in the format: @example \"NAME\" \"CODE\""
    ));
    
    schema.register_directive(DirectiveSchema(
        "@variable",
        false,
        0,
        {},
        {},
        "^@variable\\s+\"[^\"]+\"\\s+\"[^\"]*\"$",
        "Variable declaration in the format: @variable \"NAME\" \"DEFAULT_VALUE\""
    ));
    
    schema.register_directive(DirectiveSchema(
        "@inherit",
        false,
        1,
        {},
        {},
        "^@inherit\\s+\"[^\"]+\"$",
        "Parent template to inherit from"
    ));
    
    schema.register_directive(DirectiveSchema(
        "@performance",
        false,
        0,
        {},
        {},
        "^@performance\\s+\"[^\"]+\"$",
        "Performance requirements or expectations"
    ));
    
    // add custom validation rules
    
    // rule: variable references should use alphanumeric and underscore only
    schema.add_validation_rule("variable_naming", [](const std::string& content) {
        std::vector<TemplateValidationIssue> issues;
        std::regex var_ref_regex("\\$\\{([^}]+)\\}");
        std::regex valid_name_regex("^[a-zA-Z0-9_]+$");
        
        std::string::const_iterator search_start(content.begin());
        std::string::const_iterator content_end(content.end());
        std::smatch match;
        
        while (std::regex_search(search_start, content_end, match, var_ref_regex)) {
            if (match.size() > 1) {
                std::string var_name = match[1].str();
                if (!std::regex_match(var_name, valid_name_regex)) {
                    issues.push_back(TemplateValidationIssue(
                        TemplateValidationLevel::ERROR,
                        "Variable name must contain only alphanumeric characters and underscores",
                        var_name
                    ));
                }
            }
            search_start = match.suffix().first;
        }
        
        // also check variable declarations
        std::regex var_decl_regex("@variable\\s+\"([^\"]+)\"");
        search_start = content.begin();
        
        while (std::regex_search(search_start, content_end, match, var_decl_regex)) {
            if (match.size() > 1) {
                std::string var_name = match[1].str();
                if (!std::regex_match(var_name, valid_name_regex)) {
                    issues.push_back(TemplateValidationIssue(
                        TemplateValidationLevel::ERROR,
                        "Declared variable name must contain only alphanumeric characters and underscores",
                        var_name
                    ));
                }
            }
            search_start = match.suffix().first;
        }
        
        return issues;
    });
    
    // rule: description should be at least 10 characters
    schema.add_validation_rule("description_length", [](const std::string& content) {
        std::vector<TemplateValidationIssue> issues;
        std::regex desc_regex("@description\\s+\"([^\"]*)\"");
        
        std::smatch match;
        if (std::regex_search(content, match, desc_regex) && match.size() > 1) {
            std::string desc = match[1].str();
            if (desc.length() < 10) {
                issues.push_back(TemplateValidationIssue(
                    TemplateValidationLevel::ERROR,
                    "Description must be at least 10 characters long",
                    std::nullopt,
                    "@description"
                ));
            }
        }
        
        return issues;
    });
    
    // rule: check for deprecated directives
    schema.add_validation_rule("deprecated_directives", [](const std::string& content) {
        std::vector<TemplateValidationIssue> issues;
        std::set<std::string> deprecated_directives = {
            "@deprecated", "@author"
        };
        
        for (const auto& deprecated : deprecated_directives) {
            std::regex directive_regex(deprecated + "\\s+");
            if (std::regex_search(content, directive_regex)) {
                issues.push_back(TemplateValidationIssue(
                    TemplateValidationLevel::ERROR,
                    "Deprecated directive found: " + deprecated,
                    std::nullopt,
                    deprecated
                ));
            }
        }
        
        return issues;
    });
    
    // rule: check for unknown directives (malformed)
    schema.add_validation_rule("unknown_directives", [](const std::string& content) {
        std::vector<TemplateValidationIssue> issues;
        std::regex directive_regex("^@([a-zA-Z0-9_]+)\\s+", std::regex::multiline);
        std::set<std::string> known_directives = {
            "@copyright", "@language", "@description", "@context", "@dependency", 
            "@test", "@architecture", "@constraint", "@security", "@complexity",
            "@example", "@variable", "@inherit", "@performance"
        };
        
        std::string::const_iterator search_start(content.begin());
        std::string::const_iterator content_end(content.end());
        std::smatch match;
        
        while (std::regex_search(search_start, content_end, match, directive_regex)) {
            if (match.size() > 1) {
                std::string directive = "@" + match[1].str();
                if (known_directives.find(directive) == known_directives.end()) {
                    issues.push_back(TemplateValidationIssue(
                        TemplateValidationLevel::ERROR,
                        "Unknown directive: " + directive,
                        std::nullopt,
                        directive
                    ));
                }
            }
            search_start = match.suffix().first;
        }
        
        return issues;
    });
    
    return schema;
}

} // namespace cql
