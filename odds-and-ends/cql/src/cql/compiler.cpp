// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/compiler.hpp"
#include "../../include/cql/nodes.hpp"

namespace cql {

// Visitor interface implementation
void QueryCompiler::visit(const CodeRequestNode& node) {
    m_result_sections["code"] = "Please generate " + node.language() + " code that:\n" +
                             node.description() + "\n\n";
}

void QueryCompiler::visit(const ContextNode& node) {
    if (!m_result_sections.contains("context")) {
        m_result_sections["context"] = "Context:\n";
    }
    m_result_sections["context"] += "- " + node.context() + "\n";
}

void QueryCompiler::visit(const TestNode& node) {
    // Store the test cases, we'll format them when getting the final query
    m_test_cases.insert(m_test_cases.end(),
                      node.test_cases().begin(),
                      node.test_cases().end());
}

void QueryCompiler::visit(const DependencyNode& node) {
    if (!m_result_sections.contains("dependencies")) {
        m_result_sections["dependencies"] = "Dependencies:\n";
    }
    for (const auto& dependency : node.dependencies()) {
        m_result_sections["dependencies"] += "- " + dependency + "\n";
    }
}

void QueryCompiler::visit(const PerformanceNode& node) {
    if (!m_result_sections.contains("performance")) {
        m_result_sections["performance"] = "Performance Requirements:\n";
    }
    m_result_sections["performance"] += "- " + node.requirement() + "\n";
}

void QueryCompiler::visit(const CopyrightNode& node) {
    std::string copyright_message = "Please include the following copyright header at the top of all generated files:\n";
    copyright_message += "```\n";
    copyright_message += "// " + node.license() + "\n";
    copyright_message += "// Copyright (c) " + node.owner() + "\n";
    copyright_message += "```\n\n";
    m_result_sections["copyright"] = copyright_message;
}

std::string QueryCompiler::get_compiled_query() const {
    std::string query_string;

    // Add a copyright section if it exists
    auto copyright_it = m_result_sections.find("copyright");
    if (copyright_it != m_result_sections.end()) {
        query_string += copyright_it->second;
    }

    // Add a code section if it exists
    auto code_it = m_result_sections.find("code");
    if (code_it != m_result_sections.end()) {
        query_string += code_it->second;
    }

    // Add a context section if it exists
    auto context_it = m_result_sections.find("context");
    if (context_it != m_result_sections.end()) {
        query_string += context_it->second + "\n";
    }

    // Add a dependencies section if it exists
    auto dependencies_it = m_result_sections.find("dependencies");
    if (dependencies_it != m_result_sections.end()) {
        query_string += dependencies_it->second + "\n";
    }

    // Add a performance section if it exists
    auto performance_it = m_result_sections.find("performance");
    if (performance_it != m_result_sections.end()) {
        query_string += performance_it->second + "\n";
    }

    // Add test cases if we have any
    if (!m_test_cases.empty()) {
        query_string += "Please include tests for the following cases:\n";
        for (const auto& test_case : m_test_cases) {
            query_string += "- " + test_case + "\n";
        }
        query_string += "\n";
    }

    // Add a quality assurance section as a standard footer
    query_string += "Quality Assurance Requirements:\n";
    query_string += "- All code must be well-documented with comments\n";
    query_string += "- Follow modern C++ best practices\n";
    query_string += "- Ensure proper error handling\n";
    query_string += "- Optimize for readability and maintainability\n";

    return query_string;
}

void QueryCompiler::print_compiled_query(std::ostream& out) const {
    out << "\n=== Compiled Query ===\n\n"
        << get_compiled_query()
        << "===================\n";
}

} // namespace cql