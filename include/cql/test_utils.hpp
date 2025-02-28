// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_TEST_UTILS_HPP
#define CQL_TEST_UTILS_HPP

#include <string>
#include <sstream>
#include "cql.hpp"

namespace cql::test {

// define helper macros for tests
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            return TestResult::fail(message, __FILE__, __LINE__); \
        } \
    } while (false)

#define TEST_ASSERT_MESSAGE(condition, message) \
    do { \
        if (!(condition)) { \
            std::stringstream ss; \
            ss << message; \
            return TestResult::fail(ss.str(), __FILE__, __LINE__); \
        } \
    } while (false)

} // namespace cql::test

#endif // CQL_TEST_UTILS_HPP