// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>

namespace cql {

/**
 * @class DocumentationHandler
 * @brief Handles all documentation-related operations including generation and export
 * 
 * This class encapsulates documentation functionality that was previously
 * scattered throughout main.cpp, providing a clean interface for documentation operations.
 */
class DocumentationHandler {
public:
    /**
     * @brief Generate documentation for a specific template
     * @param argc Argument count
     * @param argv Argument values
     * @return int Return code (0 for success, 1 for error)
     */
    static int handle_docs_command(int argc, char* argv[]);

    /**
     * @brief Generate documentation for all templates
     * @return int Return code (0 for success, 1 for error)
     */
    static int handle_docs_all_command();

    /**
     * @brief Export documentation to a file
     * @param argc Argument count
     * @param argv Argument values
     * @return int Return code (0 for success, 1 for error)
     */
    static int handle_export_command(int argc, char* argv[]);
};

} // namespace cql