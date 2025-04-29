// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <iomanip>
#include <utility>
#include "../../include/cql/cql.hpp"
#include "../../include/cql/test_utils.hpp"

namespace cql::test {

// TestResult implementation (needed by both main and test targets)
TestResult::TestResult(const bool passed, std::string error_message,
                     std::string file_name, const int line_number)
    : m_passed(passed),
      m_error_message(std::move(error_message)),
      m_file_name(std::move(file_name)),
      m_line_number(line_number) {
}

TestResult TestResult::pass() {
    return TestResult(true);
}

TestResult TestResult::fail(const std::string& error_message,
                        const std::string& file_name,
                        const int line_number) {
    return TestResult(false, error_message, file_name, line_number);
}

bool TestResult::passed() const {
    return m_passed;
}

const std::string& TestResult::get_error_message() const {
    return m_error_message;
}

const std::string& TestResult::get_file_name() const {
    return m_file_name;
}

int TestResult::get_line_number() const {
    return m_line_number;
}

// Only include these stubs when compiling for the main application, not in test mode
#ifndef CQL_TESTING

// Simple stubs for main.cpp compatibility
void list_tests() {
    std::cout << "This is a placeholder for the legacy list_tests function." << std::endl;
    std::cout << "Please use Google Test's --gtest_list_tests flag instead." << std::endl;
}

bool run_tests(bool, const std::string&) {
    std::cout << "This is a placeholder for the legacy run_tests function." << std::endl;
    std::cout << "Please use Google Test's --gtest_filter flag instead." << std::endl;
    return true;
}

// Placeholder for test_examples_compilation
TestResult test_examples_compilation() {
    return TestResult::pass();
}

#endif // CQL_TESTING

} // namespace cql::test