// MIT License
// Copyright (c) 2025 dbjwhs

#include <fstream>
#include <stdexcept>
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

} // namespace cql