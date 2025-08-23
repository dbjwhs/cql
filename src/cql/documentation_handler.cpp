// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/documentation_handler.hpp"
#include "../../include/cql/cql.hpp"
#include "../../include/cql/template_manager.hpp"
#include <iostream>

namespace cql {

int DocumentationHandler::handle_docs_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: template name required" << std::endl;
        std::cerr << "usage: cql --docs TEMPLATE_NAME" << std::endl;
        return CQL_ERROR;
    }

    std::string template_name = argv[2];

    try {
        TemplateManager manager;
        const std::string docs = manager.generate_template_documentation(template_name);
        std::cout << docs << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "error generating template documentation: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

int DocumentationHandler::handle_docs_all_command() {
    try {
        TemplateManager manager;
        const std::string docs = manager.generate_all_template_documentation();
        std::cout << docs << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "error generating template documentation: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

int DocumentationHandler::handle_export_command(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "error: output path required" << std::endl;
        std::cerr << "usage: cql --export OUTPUT_PATH [FORMAT]" << std::endl;
        return CQL_ERROR;
    }

    std::string output_path = argv[2];
    std::string format = "markdown"; // default format

    // Check if a format is specified
    if (argc > 3) {
        format = argv[3];
    }

    try {
        TemplateManager manager;

        if (manager.export_documentation(output_path, format)) {
            std::cout << "template documentation exported to " << output_path
                      << " in " << format << " format" << std::endl;
        } else {
            std::cerr << "failed to export template documentation" << std::endl;
            return CQL_ERROR;
        }
    } catch (const std::exception& e) {
        std::cerr << "error exporting template documentation: " << e.what() << std::endl;
        return CQL_ERROR;
    }
    return CQL_NO_ERROR;
}

} // namespace cql