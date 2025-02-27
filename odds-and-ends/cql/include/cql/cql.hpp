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
#include "template_validator.hpp"
#include "template_validator_schema.hpp"

namespace cql {

/**
 * File and string utility functions
 */
namespace util {
    // Read the contents of a file
    std::string read_file(const std::string& filepath);
    
    // Write content to a file
    void write_file(const std::string& filepath, const std::string& content);
    
    // Check if a string contains a substring
    bool contains(const std::string& str, const std::string& substr);
    
    // Extract regex matches from text using a pattern
    // Returns a vector of matched groups for each match
    std::vector<std::vector<std::string>> extract_regex_matches(
        const std::string& content, 
        const std::string& pattern,
        size_t expected_groups = 0
    );
    
    // Extract string values that match a specific regex group
    // Returns a set of unique matched values from the specified group
    std::set<std::string> extract_regex_group_values(
        const std::string& content, 
        const std::string& pattern,
        size_t group_index = 1
    );
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
 * Test framework
 */
namespace test {
    /**
     * Class to represent test results
     */
    class TestResult {
    public:
        /**
         * Constructor for a passing test result
         */
        static TestResult pass();
        
        /**
         * Constructor for a failing test result
         * @param error_message The error message describing the failure
         * @param file_name The source file where the failure occurred
         * @param line_number The line number where the failure occurred
         */
        static TestResult fail(const std::string& error_message, 
                               const std::string& file_name = "", 
                               int line_number = 0);
        
        /**
         * Check if the test passed
         * @return true if the test passed, false otherwise
         */
        bool passed() const;
        
        /**
         * Get the error message for a failing test
         * @return The error message
         */
        const std::string& get_error_message() const;
        
        /**
         * Get the source file where the failure occurred
         * @return The source file name
         */
        const std::string& get_file_name() const;
        
        /**
         * Get the line number where the failure occurred
         * @return The line number
         */
        int get_line_number() const;
        
    private:
        bool m_passed;
        std::string m_error_message;
        std::string m_file_name;
        int m_line_number;
        
        // Private constructor used by static factory methods
        TestResult(bool passed, const std::string& error_message = "",
                  const std::string& file_name = "", int line_number = 0);
    };
    
    /**
     * Run all tests
     * @param fail_fast If true, stop testing after the first failure
     * @return true if all tests passed, false otherwise
     */
    bool run_tests(bool fail_fast = true);
    
    /**
     * Test the lexer
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_lexer();
    
    /**
     * Test the parser
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_parser();
    
    /**
     * Test the compiler
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_compiler();
    
    /**
     * Test basic compilation
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_basic_compilation();
    
    /**
     * Test complex compilation
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_complex_compilation();
    
    /**
     * Test Phase 2 features
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_phase2_features();
    
    /**
     * Test the template manager
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_template_manager();
    
    /**
     * Test template management
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_template_management();
    
    /**
     * Test template inheritance
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_template_inheritance();
    
    /**
     * Test template validator
     * @return TestResult indicating pass/fail with error details
     */
    TestResult test_template_validator();
    
    /**
     * Example queries
     * @return TestResult indicating pass/fail with error details
     */
    TestResult query_examples();
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