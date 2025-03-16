# MIT License
# Copyright (c) 2025 dbjwhs

cmake_minimum_required(VERSION 3.30)
project(cql VERSION 1.0.0 DESCRIPTION "Claude Query Language")

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable all warnings and make them errors
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Set include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../..
)

# Google Test integration
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Define source files by category
set(CQL_CORE_SOURCES
    src/cql/nodes.cpp
    src/cql/lexer.cpp
    src/cql/parser.cpp
    src/cql/compiler.cpp
    src/cql/util.cpp
    src/cql/validator.cpp
    src/cql/template_manager.cpp
    src/cql/template_validator.cpp
    src/cql/template_validator_schema.cpp
    src/cql/pattern_compatibility.cpp
    src/cql/api_client.cpp
    src/cql/response_processor.cpp
    src/cql/mock_server.cpp
    src/cql/stubs.cpp  # Include stubs.cpp in core sources
)

set(CQL_MAIN_SOURCES
    src/cql/main.cpp
    src/cql/cli.cpp
)

# Define test source files
set(CQL_TEST_SOURCES
    src/cql/main_test.cpp
    src/cql/tests.cpp
    src/cql/test_architecture_patterns.cpp
    src/cql/test_api_integration.cpp
    src/cql/test_config.cpp
)

# Find required packages
find_package(CURL REQUIRED)

# Include files directly
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include)

# JSON library version configuration
set(JSON_VERSION "3.11.2" CACHE STRING "Version of nlohmann_json to use")

# Download nlohmann_json if not already present
set(JSON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/include/nlohmann")
if(NOT EXISTS "${JSON_INCLUDE_DIR}/json.hpp")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/include/nlohmann")
  file(DOWNLOAD
    "https://github.com/nlohmann/json/releases/download/v${JSON_VERSION}/json.hpp"
    "${JSON_INCLUDE_DIR}/json.hpp"
    SHOW_PROGRESS
  )
  message(STATUS "Downloaded nlohmann/json.hpp v${JSON_VERSION} to ${JSON_INCLUDE_DIR}")
endif()

# Create interface library for nlohmann_json
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include)

# Create a core library with shared code
add_library(cql_lib OBJECT ${CQL_CORE_SOURCES})
set_property(TARGET cql_lib PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(cql_lib PUBLIC CURL::libcurl nlohmann_json)

# Add main executable
add_executable(cql ${CQL_MAIN_SOURCES} $<TARGET_OBJECTS:cql_lib>)
target_link_libraries(cql PRIVATE CURL::libcurl nlohmann_json)

# Create test executable with properly defined object files
add_library(cql_lib_test OBJECT ${CQL_CORE_SOURCES})
set_property(TARGET cql_lib_test PROPERTY POSITION_INDEPENDENT_CODE 1)
target_compile_definitions(cql_lib_test PUBLIC CQL_TESTING=1)
target_link_libraries(cql_lib_test PUBLIC CURL::libcurl nlohmann_json)
target_include_directories(cql_lib_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/googletest-src/googletest/include)

# Link tests with the test-specific version of the library
add_executable(cql_test ${CQL_TEST_SOURCES} $<TARGET_OBJECTS:cql_lib_test>)
target_compile_definitions(cql_test PRIVATE CQL_TESTING=1)
target_link_libraries(cql_test PRIVATE 
    CURL::libcurl 
    nlohmann_json
    GTest::gtest_main 
    GTest::gtest
)

# Installation
install(TARGETS cql DESTINATION bin)
install(FILES LICENSE DESTINATION share/doc/cql)

# Enable testing
enable_testing()
include(GoogleTest)
gtest_discover_tests(cql_test)

# Add a custom target for running tests
add_custom_target(run_tests
    COMMAND cql_test
    DEPENDS cql_test
    COMMENT "Running CQL tests with GoogleTest"
)

# Add a custom target for documentation
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
                COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
        )
    endif()
endif()
