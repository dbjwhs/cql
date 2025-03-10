#!/bin/bash
# MIT License
# Copyright (c) 2025 dbjwhs

# Exit on error
set -e

# Default build type
BUILD_TYPE="Release"
BUILD_DOCS=false
BUILD_TESTS=true
CLEAN=false
INSTALL=false
CMAKE_OPTIONS=""

# Function to display help message
function show_help {
    echo "Build script for Distributed Task Scheduler"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -d, --debug         Build in debug mode"
    echo "  -r, --release       Build in release mode (default)"
    echo "  --docs              Generate documentation"
    echo "  --no-tests          Don't build or run tests"
    echo "  --clean             Clean build directory before building"
    echo "  --install           Install after building"
    echo "  --sanitize=TYPE     Build with sanitizer (address, thread, undefined)"
    echo "  --cmake-option=OPT  Pass custom option to CMake (can be used multiple times)"
    echo ""
    echo "Example:"
    echo "  $0 --debug --docs --clean --sanitize=address"
    exit 0
}

# Parse command line arguments
for arg in "$@"; do
    case $arg in
        -h|--help)
            show_help
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        --docs)
            BUILD_DOCS=true
            shift
            ;;
        --no-tests)
            BUILD_TESTS=false
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --install)
            INSTALL=true
            shift
            ;;
        --sanitize=*)
            SANITIZER="${arg#*=}"
            case $SANITIZER in
                address)
                    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_CXX_FLAGS='-fsanitize=address -fno-omit-frame-pointer'"
                    ;;
                thread)
                    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_CXX_FLAGS='-fsanitize=thread -fno-omit-frame-pointer'"
                    ;;
                undefined)
                    CMAKE_OPTIONS="$CMAKE_OPTIONS -DCMAKE_CXX_FLAGS='-fsanitize=undefined -fno-omit-frame-pointer'"
                    ;;
                *)
                    echo "Unknown sanitizer: $SANITIZER"
                    exit 1
                    ;;
            esac
            shift
            ;;
        --cmake-option=*)
            CMAKE_OPTIONS="$CMAKE_OPTIONS ${arg#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $arg"
            show_help
            ;;
    esac
done

# Create build directory
mkdir -p build
cd build

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf *
fi

# Configure with CMake
echo "Configuring with CMake for $BUILD_TYPE build..."
if [ "$BUILD_TESTS" = true ]; then
    eval cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTS=ON $CMAKE_OPTIONS
else
    eval cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTS=OFF $CMAKE_OPTIONS
fi

# Build
echo "Building..."
cmake --build . -j$(nproc)

# Build documentation if requested
if [ "$BUILD_DOCS" = true ]; then
    echo "Generating documentation..."
    cmake --build . --target documentation
    echo "Documentation built in $(pwd)/docs/html/"
fi

# Run tests if requested
if [ "$BUILD_TESTS" = true ]; then
    echo "Running tests..."
    ctest --output-on-failure -V
fi

# Install if requested
if [ "$INSTALL" = true ]; then
    echo "Installing..."
    sudo cmake --install .
fi

echo "Build completed successfully!"
