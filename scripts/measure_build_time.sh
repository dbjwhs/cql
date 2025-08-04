#!/bin/bash
# MIT License
# Copyright (c) 2025 dbjwhs

# Script to measure build times for the CQL project

set -e

BUILD_DIR="build_perf_test"
RESULTS_FILE="build_times.log"

echo "CQL Build Performance Measurement Script"
echo "========================================"

# Function to measure build time
measure_build() {
    local build_type="$1"
    local clean_build="$2"
    
    echo "Measuring $build_type build time..."
    
    if [ "$clean_build" = "clean" ]; then
        echo "Performing clean build..."
        rm -rf "$BUILD_DIR"
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        # Configure with timing
        echo "Configuring..."
        time_config=$(TIMEFORMAT='%3R'; { time cmake .. -DCMAKE_BUILD_TYPE="$build_type" >/dev/null 2>&1; } 2>&1)
        echo "Configuration time: ${time_config}s"
        
        # Build with timing
        echo "Building..."
        start_time=$(date +%s.%N)
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) >/dev/null 2>&1
        end_time=$(date +%s.%N)
        build_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || python3 -c "print($end_time - $start_time)")
        
        cd ..
    else
        echo "Performing incremental build..."
        cd "$BUILD_DIR"
        
        # Touch a file to force rebuild
        touch ../src/cql/main.cpp
        
        start_time=$(date +%s.%N)
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) >/dev/null 2>&1
        end_time=$(date +%s.%N)
        build_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || python3 -c "print($end_time - $start_time)")
        
        cd ..
    fi
    
    printf "%.2f" "$build_time"
}

# Get system info
echo "System Information:"
echo "OS: $(uname -s)"
echo "CPU cores: $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 'unknown')"
echo "CMake version: $(cmake --version | head -n1)"
echo "Compiler: $(${CXX:-c++} --version | head -n1 2>/dev/null || echo 'unknown')"
echo ""

# Measure different build scenarios
echo "Build Time Measurements:"
echo "========================"

# Clean Debug build
debug_clean_time=$(measure_build "Debug" "clean")
echo "Clean Debug build: ${debug_clean_time}s"

# Incremental Debug build
debug_incremental_time=$(measure_build "Debug" "incremental")
echo "Incremental Debug build: ${debug_incremental_time}s"

# Clean Release build
release_clean_time=$(measure_build "Release" "clean")
echo "Clean Release build: ${release_clean_time}s"

# Save results to log file
{
    echo "Build Performance Results - $(date)"
    echo "===================================="
    echo "System: $(uname -s) - $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 'unknown') cores"
    echo "Compiler: $(${CXX:-c++} --version | head -n1 2>/dev/null || echo 'unknown')"
    echo ""
    echo "Clean Debug build: ${debug_clean_time}s"
    echo "Incremental Debug build: ${debug_incremental_time}s"
    echo "Clean Release build: ${release_clean_time}s"
    echo ""
} >> "$RESULTS_FILE"

echo ""
echo "Results saved to $RESULTS_FILE"
echo "Build measurement complete!"

# Cleanup
rm -rf "$BUILD_DIR"