// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_HPP
#define CQL_HPP

/**
 * Claude Query Language (CQL) - Main Header
 * 
 * The Claude Query Language was developed in 2025 as a domain-specific language to formalize
 * and standardize how developers craft queries for Large Language Models (LLMs), specifically
 * Anthropic's Claude. It follows the compiler pattern where a high-level representation (the CQL)
 * is translated into a more detailed and structured query string.
 * 
 * This file provides the main public interface for the CQL library.
 */

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <fstream>
#include <optional>
#include <map>

#include "nodes.hpp"
#include "visitor.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "validator.hpp"
#include "template_manager.hpp"

namespace cql {

/**
 * File utility functions
 */
namespace util {
    // Read the contents of a file
    std::string read_file(const std::string& filepath);
    
    // Write content to a file
    void write_file(const std::string& filepath, const std::string& content);
    
    // Check if a string contains a substring
    bool contains(const std::string& str, const std::string& substr);
}

/**
 * Main CQL processor class
 * Provides a simplified interface for compiling CQL queries
 */
class QueryProcessor {
public:
    // Compile a CQL string to a structured query
    static std::string compile(const std::string_view query_str);
    
    // Compile a CQL file to a structured query
    static std::string compile_file(const std::string& filepath);
    
    // Save a compiled query to a file
    static void save_compiled(const std::string_view query_str, const std::string& filepath);
    
    // Compile a template with variable substitutions
    static std::string compile_template(const std::string& template_name, 
                                     const std::map<std::string, std::string>& variables);
};

/**
 * Test functions
 */
namespace test {
    // Run all tests
    void run_tests();
    
    // Test the lexer
    void test_lexer();
    
    // Test the parser
    void test_parser();
    
    // Test the compiler
    void test_compiler();
    
    // Test basic compilation
    void test_basic_compilation();
    
    // Test complex compilation
    void test_complex_compilation();
    
    // Test Phase 2 features
    void test_phase2_features();
    
    // Test the template manager
    void test_template_manager();
    
    // Test template management
    void test_template_management();
    
    // Test template inheritance
    void test_template_inheritance();
    
    // Example queries
    void query_examples();
}

/**
 * Interactive CLI functions
 */
namespace cli {
    // Run the interactive CLI
    void run_cli();
    
    // Process a query file
    bool process_file(const std::string& input_file, const std::string& output_file);
}

} // namespace cql

#endif // CQL_HPP