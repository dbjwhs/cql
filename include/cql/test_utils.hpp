// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_TEST_UTILS_HPP
#define CQL_TEST_UTILS_HPP

#include <string>
#include <sstream>
#include <iostream>
#include "cql.hpp"

// Include Google Test in test mode
#ifdef CQL_TESTING
  #include <gtest/gtest.h>
#endif

// Google Test compatibility macros
#ifdef CQL_TESTING
  #define TEST_ASSERT_GTEST(condition, message) ASSERT_TRUE(condition) << message
  #define TEST_ASSERT_MESSAGE_GTEST(condition, message) ASSERT_TRUE(condition) << message
#else
  #define TEST_ASSERT_GTEST(condition, message) do { if(!(condition)) { std::cerr << message; } } while(false)
  #define TEST_ASSERT_MESSAGE_GTEST(condition, message) do { if(!(condition)) { std::cerr << message; } } while(false)
#endif

// Legacy macros for backward compatibility - these create proper TestResult returns
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            return cql::test::TestResult::fail(message, __FILE__, __LINE__); \
        } \
    } while (false)

#define TEST_ASSERT_MESSAGE(condition, message) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << message; \
            return cql::test::TestResult::fail(ss.str(), __FILE__, __LINE__); \
        } \
    } while (false)

namespace cql::test {
    
// Function to print test results (will be redirected to Google Test output)
inline void print_test_result(const std::string& test_name, const TestResult& result) {
    if (result.passed()) {
        std::cout << "[       OK ] " << test_name << std::endl;
    } else {
        std::cout << "[  FAILED  ] " << test_name << std::endl;
        std::cout << "  Error: " << result.get_error_message() << std::endl;
        if (!result.get_file_name().empty()) {
            std::cout << "  Location: " << result.get_file_name() 
                     << ":" << result.get_line_number() << std::endl;
        }
    }
}

// Legacy test functions have been removed

} // namespace cql::test

#endif // CQL_TEST_UTILS_HPP
