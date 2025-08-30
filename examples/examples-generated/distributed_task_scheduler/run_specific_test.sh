#!/bin/bash

# Script to run a specific test with debugging options

set -e  # Exit on error

# Create build directory if it doesn't exist
mkdir -p debug-build

# Navigate to build directory
cd debug-build

# Configure with debug options
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..

# Build
make

# Run the specific test with more verbose output
GTEST_FILTER="TaskSchedulerTest.WorkerNodeFailureAndRecovery" ./bin/scheduler_tests --gtest_break_on_failure --gtest_catch_exceptions=0
