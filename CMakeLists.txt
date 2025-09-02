# MIT License
# Copyright (c) 2025 dbjwhs

cmake_minimum_required(VERSION 3.30)

# Disable deprecation warnings on minimum required version
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Suppress deprecation warnings" FORCE)
set(CMAKE_DEPRECATION_WARNINGS OFF CACHE BOOL "Suppress deprecation warnings" FORCE)

# Suppress warnings from GoogleTest and other dependencies
foreach(policy
    CMP0048 # project() command manages VERSION variables
    CMP0054 # Only interpret if() arguments as variables or keywords when unquoted
    CMP0056 # Honor link flags in try_compile() source-file signature
    CMP0057 # Support IN_LIST operator
    CMP0077 # Option() honors normal variables
    CMP0111 # Precompile option for INTERFACE libraries
    )
  if(POLICY ${policy})
    cmake_policy(SET ${policy} NEW)
  endif()
endforeach()

project(cql VERSION 1.0.0 DESCRIPTION "Claude Query Language")

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable precompiled headers support
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
endif()

# Build optimizations
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json for IDEs

# Enable parallel compilation for better build times
if(MSVC)
    # Use maximum number of parallel processes for MSVC
    add_compile_options(/MP)
else()
    # For GCC/Clang, let make/ninja handle parallelization
    # User can use make -j$(nproc) or ninja will auto-detect
endif()

# Optimize debug builds for faster compilation
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(NOT MSVC)
        # Reduce debug info level for faster compilation in debug builds
        add_compile_options(-g1)
    endif()
endif()

# Enable all warnings and make them errors
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Set include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/lib/ailib/include
    ${PROJECT_SOURCE_DIR}/../..
)

# Google Test integration
include(FetchContent)

# Suppress warnings from third-party dependencies
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(FETCHCONTENT_QUIET TRUE)
set(CMAKE_MESSAGE_LOG_LEVEL "WARNING")

# Build optimization: Use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Found ccache: ${CCACHE_PROGRAM} - enabling compiler caching")
endif()

# Define a function to download a dependency without showing deprecation warnings
function(fetch_dependency dep_name url)
  # Save current message level
  set(OLD_CMAKE_MESSAGE_LOG_LEVEL ${CMAKE_MESSAGE_LOG_LEVEL})
  # Temporarily set message level to ERROR to hide warnings
  set(CMAKE_MESSAGE_LOG_LEVEL "ERROR")
  
  FetchContent_Declare(
    ${dep_name}
    URL ${url}
  )
  
  FetchContent_MakeAvailable(${dep_name})
  
  # Restore message level
  set(CMAKE_MESSAGE_LOG_LEVEL ${OLD_CMAKE_MESSAGE_LOG_LEVEL})
endfunction()

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Optimization: Build GoogleTest only if we're testing
set(BUILD_GMOCK OFF CACHE BOOL "Disable GMock to speed up builds")
set(INSTALL_GTEST OFF CACHE BOOL "Disable GTest installation to speed up builds")

# Fetch GoogleTest with warnings suppressed
fetch_dependency(
  googletest
  https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)

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
    lib/ailib/src/providers/factory.cpp
    lib/ailib/src/providers/anthropic.cpp
    lib/ailib/src/config.cpp
    lib/ailib/src/http/curl_client.cpp
    src/cql/stubs.cpp
    src/cql/clipboard_impl.cpp
    src/cql/clipboard_stub.cpp
    src/cql/input_validator.cpp
    lib/ailib/src/json_utils.cpp
    lib/ailib/src/auth/secure_store.cpp
    src/cql/error_context.cpp
    src/cql/cli.cpp
    src/cql/logger_interface.cpp
    src/cql/logger_manager.cpp
    src/cql/logger_adapters.cpp
    src/cql/historic_logger_bridge.cpp
    src/cql/logger_bridge.cpp
    src/cql/meta_prompt/hybrid_compiler.cpp
    src/cql/meta_prompt/intelligent_cache.cpp
    src/cql/meta_prompt/prompt_compiler.cpp
    src/cql/meta_prompt/circuit_breaker.cpp
    src/cql/meta_prompt/cost_controller.cpp
    src/cql/meta_prompt/validation_framework.cpp
    src/cql/meta_prompt_handler.cpp
    src/cql/command_line_handler.cpp
)

set(CQL_MAIN_SOURCES
    src/cql/main.cpp
    src/cql/template_operations.cpp
    src/cql/documentation_handler.cpp
    src/cql/application_controller.cpp
)

# Define test source files
set(CQL_TEST_SOURCES
    src/cql/main_test.cpp
    src/cql/tests.cpp
    src/cql/test_architecture_patterns.cpp
    src/cql/test_api_integration.cpp
    lib/ailib/tests/test_config.cpp
    lib/ailib/tests/test_config_enhanced.cpp
    src/cql/test_security.cpp
    lib/ailib/tests/test_http_client.cpp
    lib/ailib/tests/test_anthropic_provider.cpp
    src/cql/test_pluggable_logger.cpp
    src/cql/test_logger_bridge.cpp
    src/cql/test_live_anthropic_integration.cpp
    src/cql/test_meta_prompt_foundation.cpp
    src/cql/test_hybrid_compiler.cpp
    src/cql/test_intelligent_cache.cpp
    src/cql/test_meta_prompt_cli.cpp
)

# Find required packages
find_package(CURL REQUIRED)

# Platform-specific configurations
if(APPLE)
    # Add macOS implementation for clipboard using Objective-C++
    set(PLATFORM_SOURCES src/cql/clipboard_macos.mm)
    # Link Cocoa framework
    find_library(COCOA_LIBRARY Cocoa)
    list(APPEND PLATFORM_LIBRARIES ${COCOA_LIBRARY})
    # Add Objective-C++ flags
    set_source_files_properties(src/cql/clipboard_macos.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")
elseif(WIN32)
    # Future Windows clipboard implementation
    set(PLATFORM_SOURCES "")
    set(PLATFORM_LIBRARIES "")
else()
    # Linux or other platforms - no platform-specific clipboard implementation
    set(PLATFORM_SOURCES "")
    set(PLATFORM_LIBRARIES "")
endif()

# Include files directly
include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include)

# JSON library version configuration
set(JSON_VERSION "3.11.2" CACHE STRING "Version of nlohmann_json to use")

# Download nlohmann_json if not already present (with error handling)
set(JSON_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/include/nlohmann")
if(NOT EXISTS "${JSON_INCLUDE_DIR}/json.hpp")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/include/nlohmann")
  message(STATUS "Downloading nlohmann/json.hpp v${JSON_VERSION}...")
  file(DOWNLOAD
    "https://github.com/nlohmann/json/releases/download/v${JSON_VERSION}/json.hpp"
    "${JSON_INCLUDE_DIR}/json.hpp"
    SHOW_PROGRESS
    STATUS download_status
    LOG download_log
  )
  list(GET download_status 0 download_result)
  if(NOT download_result EQUAL 0)
    message(FATAL_ERROR "Failed to download nlohmann/json.hpp: ${download_log}")
  endif()
  message(STATUS "Downloaded nlohmann/json.hpp v${JSON_VERSION} to ${JSON_INCLUDE_DIR}")
endif()

# Create interface library for nlohmann_json with compile features
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include)
# Ensure nlohmann_json uses the same C++ standard
target_compile_features(nlohmann_json INTERFACE cxx_std_20)

# Create a core library with shared code
add_library(cql_lib OBJECT ${CQL_CORE_SOURCES} ${PLATFORM_SOURCES})
set_property(TARGET cql_lib PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(cql_lib PUBLIC CURL::libcurl nlohmann_json ${PLATFORM_LIBRARIES})

# Enable precompiled headers for the core library (exclude Objective-C++ files)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(cql_lib PRIVATE include/cql/pch.hpp)
    # Exclude Objective-C++ files from using precompiled headers
    if(APPLE)
        set_source_files_properties(src/cql/clipboard_macos.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    endif()
endif()

# Add main executable
add_executable(cql ${CQL_MAIN_SOURCES} $<TARGET_OBJECTS:cql_lib>)
target_link_libraries(cql PRIVATE CURL::libcurl nlohmann_json ${PLATFORM_LIBRARIES})

# Reuse precompiled headers from cql_lib for main executable
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(cql REUSE_FROM cql_lib)
endif()

# Create test executable with properly defined object files
add_library(cql_lib_test OBJECT ${CQL_CORE_SOURCES} ${PLATFORM_SOURCES})
set_property(TARGET cql_lib_test PROPERTY POSITION_INDEPENDENT_CODE 1)
target_compile_definitions(cql_lib_test PUBLIC CQL_TESTING=1)
target_link_libraries(cql_lib_test PUBLIC CURL::libcurl nlohmann_json ${PLATFORM_LIBRARIES})
target_include_directories(cql_lib_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/googletest-src/googletest/include)

# Enable precompiled headers for the test library (exclude Objective-C++ files)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(cql_lib_test PRIVATE include/cql/pch.hpp)
    # Exclude Objective-C++ files from using precompiled headers
    if(APPLE)
        set_source_files_properties(src/cql/clipboard_macos.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    endif()
endif()

# Link tests with the test-specific version of the library
add_executable(cql_test ${CQL_TEST_SOURCES} $<TARGET_OBJECTS:cql_lib_test>)
target_compile_definitions(cql_test PRIVATE CQL_TESTING=1)
target_link_libraries(cql_test PRIVATE 
    CURL::libcurl 
    nlohmann_json
    ${PLATFORM_LIBRARIES}
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
