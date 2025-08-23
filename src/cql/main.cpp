// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/application_controller.hpp"

/**
 * @brief Main application entry point
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return int Return code (0 for success, 1 for error)
 */
int main(const int argc, char* argv[]) {
    return cql::ApplicationController::run(argc, argv);
}