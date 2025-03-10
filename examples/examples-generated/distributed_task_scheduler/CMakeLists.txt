# MIT License
# Copyright (c) 2025 dbjwhs

cmake_minimum_required(VERSION 3.15)
project(DistributedTaskScheduler VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to enable or disable tests
option(BUILD_TESTS "Build test executable" ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build tests: ${BUILD_TESTS}")

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/external/jwt-cpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/picojson
)

# Find required packages
find_package(Boost COMPONENTS system thread)
if(NOT Boost_FOUND)
    message(WARNING "Boost not found, some functionality may be limited")
endif()

find_package(OpenSSL)
if(NOT OpenSSL_FOUND)
    message(WARNING "OpenSSL not found, security features may be limited")
endif()

find_package(nlohmann_json)
if(NOT nlohmann_json_FOUND)
    message(WARNING "nlohmann_json not found, using included version")
endif()

find_package(Threads REQUIRED)

# Try to find JWT-CPP (might need manual setup)
find_package(jwt-cpp QUIET)
if(NOT jwt-cpp_FOUND)
    message(STATUS "jwt-cpp not found via CMake - will need to ensure it's in include path")
endif()

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.hpp")

# Create main executable
add_executable(scheduler 
    main.cpp
    ${SOURCES}
)

# Link libraries
target_link_libraries(scheduler
    PRIVATE
    Threads::Threads
)

if(Boost_FOUND)
    target_link_libraries(scheduler PRIVATE Boost::system Boost::thread)
endif()

if(OpenSSL_FOUND)
    target_link_libraries(scheduler PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(scheduler PRIVATE nlohmann_json::nlohmann_json)
endif()

# Enable testing
include(CTest)

# Test executable (optional)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    if(GTest_FOUND)
        file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

        add_executable(scheduler_tests
            ${TEST_SOURCES}
            ${SOURCES}
        )

        target_link_libraries(scheduler_tests
            PRIVATE
            Threads::Threads
            GTest::GTest
            GTest::Main
            GTest::gmock
            GTest::gmock_main
        )

        if(Boost_FOUND)
            target_link_libraries(scheduler_tests PRIVATE Boost::system Boost::thread)
        endif()

        if(OpenSSL_FOUND)
            target_link_libraries(scheduler_tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        endif()

        if(nlohmann_json_FOUND)
            target_link_libraries(scheduler_tests PRIVATE nlohmann_json::nlohmann_json)
        endif()

        # Testing support
        enable_testing()
        add_test(NAME AllTests COMMAND scheduler_tests)
        
        # Make test discoverable by CTest
        set_tests_properties(AllTests PROPERTIES LABELS "AllTests")
        
        # Compiler warnings for tests
        if(MSVC)
            target_compile_options(scheduler_tests PRIVATE /W4)
        else()
            target_compile_options(scheduler_tests PRIVATE -Wall -Wextra -pedantic)
        endif()
    else()
        message(FATAL_ERROR "GTest not found - required for tests")
    endif()
endif()

# Installation targets
install(TARGETS scheduler
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ 
    DESTINATION include/distributed_scheduler
    FILES_MATCHING PATTERN "*.hpp"
)

# Compiler warnings and options
if(MSVC)
    target_compile_options(scheduler PRIVATE /W4)
else()
    target_compile_options(scheduler PRIVATE -Wall -Wextra -pedantic)
endif()

# Doxygen support
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN NO)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
    set(DOXYGEN_PROJECT_NAME "Distributed Task Scheduler")
    set(DOXYGEN_PROJECT_BRIEF "Modern C++20 distributed task scheduler with real-time monitoring")
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_EXTRACT_STATIC YES)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    
    doxygen_add_docs(docs 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        COMMENT "Generating documentation with Doxygen"
    )
    
    add_custom_target(documentation
        DEPENDS docs
        COMMENT "Building documentation"
    )
else()
    message(STATUS "Doxygen not found - documentation will not be built")
endif()

# Package configuration
include(CPack)
set(CPACK_PACKAGE_VENDOR "dbjwhs")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern C++20 distributed task scheduler")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
