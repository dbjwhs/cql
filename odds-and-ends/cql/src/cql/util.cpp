// MIT License
// Copyright (c) 2025 dbjwhs

#include <fstream>
#include <stdexcept>
#include <regex>
#include <set>
#include <optional>
#include "../../include/cql/cql.hpp"
#include "../../../headers/project_utils.hpp"

namespace cql::util {

// file utility functions implementation
std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void write_file(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filepath);
    }
    file << content;
}

bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

// extract regex matches from text using a pattern
std::vector<std::vector<std::string>> extract_regex_matches(
    const std::string& content, 
    const std::string& pattern,
    size_t expected_groups
) {
    std::vector<std::vector<std::string>> results;
    std::regex regex_pattern(pattern);
    
    auto begin = std::sregex_iterator(content.begin(), content.end(), regex_pattern);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator it = begin; it != end; ++it) {
        std::smatch match = *it;
        
        // skip if we don't have the expected number of groups
        if (expected_groups > 0 && match.size() <= expected_groups) {
            continue;
        }
        
        // each match has multiple groups
        std::vector<std::string> groups;
        for (size_t i = 0; i < match.size(); ++i) {
            groups.push_back(match[i].str());
        }
        
        results.push_back(groups);
    }
    
    return results;
}

// extract string values that match a specific regex group
std::set<std::string> extract_regex_group_values(
    const std::string& content, 
    const std::string& pattern,
    size_t group_index
) {
    std::set<std::string> values;
    std::regex regex_pattern(pattern);
    
    auto begin = std::sregex_iterator(content.begin(), content.end(), regex_pattern);
    auto end = std::sregex_iterator();
    
    for (std::sregex_iterator it = begin; it != end; ++it) {
        std::smatch match = *it;
        if (match.size() > group_index) {
            values.insert(match[group_index].str());
        }
    }
    
    return values;
}

} // namespace cql::util

namespace cql {

/**
 * Enhanced query processor implementation that separates parsing and validation phases.
 * 
 * This implementation follows these steps:
 * 1. Parse the query, capturing any parser errors but allowing partial parsing
 * 2. Validate the parsed nodes, even if parsing was incomplete
 * 3. Report validation issues first, as they're often more important than syntax errors
 * 4. Report parser errors if validation passed
 * 5. Compile the validated nodes into a query
 *
 * This separation allows us to provide more meaningful error messages to users,
 * focusing on content issues rather than just syntax problems.
 */
std::string QueryProcessor::compile(const std::string_view query_str) {
    // Store parser errors to report them after validation
    std::optional<std::string> parser_error;
    std::vector<std::unique_ptr<QueryNode>> nodes;
    
    // Try to parse the query string, but catch parser errors
    try {
        Parser parser(query_str);
        nodes = parser.parse();
    } catch (const ParserError& e) {
        // Store the error message but continue with validation on any nodes we did parse
        parser_error = e.what();
    } catch (const std::exception& e) {
        // For other errors, just rethrow
        throw;
    }
    
    // If no nodes were parsed at all, we can't validate, so just report the parser error
    if (nodes.empty() && parser_error) {
        throw std::runtime_error(*parser_error);
    }
    
    // Always attempt validation, even if parsing had errors
    std::vector<ValidationIssue> validation_issues;
    try {
        // validate the query structure with whatever nodes we have
        QueryValidator validator;
        validation_issues = validator.validate(nodes);
    } catch (const ValidationException& e) {
        // Transform validation exceptions to runtime errors
        throw std::runtime_error("Validation error: " + std::string(e.what()));
    }
    
    // Report validation issues
    for (const auto& issue : validation_issues) {
        std::string level;
        switch (issue.severity) {
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
        
        Logger::getInstance().log(
            issue.severity == ValidationSeverity::ERROR ? LogLevel::ERROR : LogLevel::NORMAL,
            "Validation ", level, ": ", issue.message
        );
        
        // For errors, throw an exception - validation errors take precedence
        if (issue.severity == ValidationSeverity::ERROR) {
            throw std::runtime_error("Validation error: " + issue.message);
        }
    }
    
    // After reporting validation issues, report any parser errors
    if (parser_error) {
        throw std::runtime_error(*parser_error);
    }
    
    // If we got here, we have valid nodes, so compile the query
    QueryCompiler compiler;
    for (const auto& node : nodes) {
        node->accept(compiler);
    }
    
    return compiler.get_compiled_query();
}

std::string QueryProcessor::compile_file(const std::string& filepath) {
    std::string query = util::read_file(filepath);
    return compile(query);
}

void QueryProcessor::save_compiled(const std::string_view query_str, const std::string& filepath) {
    std::string compiled = compile(query_str);
    util::write_file(filepath, compiled);
}

std::string QueryProcessor::compile_template(
    const std::string& template_name, 
    const std::map<std::string, std::string>& variables
) {
    TemplateManager manager;
    std::string instantiated = manager.instantiate_template(template_name, variables);
    return compile(instantiated);
}

} // namespace cql
