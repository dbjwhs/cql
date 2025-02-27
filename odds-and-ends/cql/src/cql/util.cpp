// MIT License
// Copyright (c) 2025 dbjwhs

#include <fstream>
#include <stdexcept>
#include <regex>
#include <set>
#include "../../include/cql/cql.hpp"
#include "../../../headers/project_utils.hpp"

namespace cql::util {

// File utility functions implementation
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

// Extract regex matches from text using a pattern
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
        
        // Skip if we don't have the expected number of groups
        if (expected_groups > 0 && match.size() <= expected_groups) {
            continue;
        }
        
        // Each match has multiple groups
        std::vector<std::string> groups;
        for (size_t i = 0; i < match.size(); ++i) {
            groups.push_back(match[i].str());
        }
        
        results.push_back(groups);
    }
    
    return results;
}

// Extract string values that match a specific regex group
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

// QueryProcessor implementation
std::string QueryProcessor::compile(const std::string_view query_str) {
    // Parse the query string
    Parser parser(query_str);
    auto nodes = parser.parse();
    
    // Validate the query structure
    QueryValidator validator;
    auto issues = validator.validate(nodes);
    
    // Report validation issues
    for (const auto& issue : issues) {
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
        
        // For errors, throw an exception
        if (issue.severity == ValidationSeverity::ERROR) {
            throw std::runtime_error("Validation error: " + issue.message);
        }
    }
    
    // Compile the query
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