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

# Define source files
set(CQL_SOURCES
    src/cql/main.cpp
    src/cql/nodes.cpp
    src/cql/lexer.cpp
    src/cql/parser.cpp
    src/cql/compiler.cpp
    src/cql/util.cpp
    src/cql/cli.cpp
    src/cql/tests.cpp
    src/cql/validator.cpp
    src/cql/template_manager.cpp
    src/cql/template_validator.cpp
    src/cql/template_validator_schema.cpp
    src/cql/pattern_compatibility.cpp
    src/cql/test_architecture_patterns.cpp
    src/cql/api_client.cpp
    src/cql/response_processor.cpp
    src/cql/mock_server.cpp
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

# Add executable
add_executable(cql ${CQL_SOURCES})

# Link libraries
target_link_libraries(cql PRIVATE CURL::libcurl nlohmann_json)

# Installation
install(TARGETS cql DESTINATION bin)
install(FILES LICENSE DESTINATION share/doc/cql)


# Enable testing
enable_testing()
add_test(NAME cql_tests COMMAND cql --test)

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
