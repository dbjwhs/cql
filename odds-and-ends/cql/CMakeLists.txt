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
)

# Add executable
add_executable(cql ${CQL_SOURCES})

# Installation
install(TARGETS cql DESTINATION bin)
install(FILES LICENSE DESTINATION share/doc/cql)

# Enable testing
enable_testing()
add_test(NAME cql_tests COMMAND cql --test)
