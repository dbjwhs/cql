cmake_minimum_required(VERSION 3.20)
project(ailib VERSION 0.1.0 LANGUAGES CXX)

# Modern C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(AILIB_BUILD_TESTS "Build AILib tests" ON)
option(AILIB_BUILD_EXAMPLES "Build AILib examples" OFF)

# Find dependencies
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# AILib source files
set(AILIB_SOURCES
    src/config.cpp
    src/providers/anthropic.cpp
    src/providers/factory.cpp
    src/http/curl_client.cpp
    src/auth/secure_store.cpp
    src/json_utils.cpp
)

# Create AILib library
add_library(ailib ${AILIB_SOURCES})

# Set target properties
set_target_properties(ailib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Public interface
target_include_directories(ailib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(ailib 
    PUBLIC 
        nlohmann_json::nlohmann_json
    PRIVATE
        CURL::libcurl
        Threads::Threads
)

# Compiler-specific options
if(MSVC)
    target_compile_options(ailib PRIVATE /W4)
else()
    target_compile_options(ailib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Tests
if(AILIB_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    
    add_executable(ailib_test
        tests/test_anthropic_provider.cpp
        tests/test_http_client.cpp
        tests/test_config.cpp
        tests/test_config_enhanced.cpp
    )
    
    target_link_libraries(ailib_test
        ailib
        GTest::gtest
        GTest::gtest_main
    )
    
    add_test(NAME ailib_tests COMMAND ailib_test)
endif()

# Examples
if(AILIB_BUILD_EXAMPLES)
    # Add example targets here when examples are created
endif()
