// MIT License
// Copyright (c) 2025 dbjwhs

#include <fstream>
#include <stdexcept>
#include "../../include/cql/cql.hpp"

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
    Parser parser(query_str);
    auto nodes = parser.parse();
    
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