# MIT License
# Copyright (c) 2025 dbjwhs

cmake_minimum_required(VERSION 3.15)
project(DocumentProcessingSystem VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Build directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/headers)

# Find threads package
find_package(Threads REQUIRED)

# Find or fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()
include(GoogleTest)

# Common source files that should be included in all targets
set(COMMON_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/document_system_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/abstract_factory.cpp
)

# Main executable
add_executable(document_system_example ${CMAKE_CURRENT_SOURCE_DIR}/src/document_system_example.cpp ${COMMON_SOURCES})
target_link_libraries(document_system_example PRIVATE Threads::Threads)

# Test executables
set(TEST_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/test_factory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/test_builder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/test_singleton.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/test_abstract_factory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/test_observer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/test_strategy.cpp
)

# Add test executables
foreach(test_file ${TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file} ${COMMON_SOURCES})
    target_link_libraries(${test_name}
            PRIVATE
            gtest
            gtest_main
            Threads::Threads
    )
    gtest_discover_tests(${test_name})
endforeach()

# Install targets
install(TARGETS document_system_example
        RUNTIME DESTINATION bin
)

install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/document_types.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/document_factory.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/document_manager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/document_builder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/document_observer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/document_strategy.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/abstract_factory.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/concrete_documents.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/concrete_tools.h
        ${CMAKE_CURRENT_SOURCE_DIR}/headers/message_queue.h
        DESTINATION include/document_system
)

# Add a custom target for running all tests
add_custom_target(run_all_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS ${TEST_FILES}
        COMMENT "Running all tests"
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