// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <string>
#include <gtest/gtest.h>
#include "../../include/cql/cql.hpp"

// Main entry point for Google Test executable
int main(int argc, char** argv) {
    std::cout << "Running CQL tests with Google Test..." << std::endl;
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run all tests
    return RUN_ALL_TESTS();
}
