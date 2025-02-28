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

void QueryCompiler::visit(const ArchitectureNode& node) {
    if (!m_result_sections.contains("architecture")) {
        m_result_sections["architecture"] = "Architecture Requirements:\n";
    }
    m_result_sections["architecture"] += "- " + node.architecture() + "\n";
}

void QueryCompiler::visit(const ConstraintNode& node) {
    if (!m_result_sections.contains("constraints")) {
        m_result_sections["constraints"] = "Constraints:\n";
    }
    m_result_sections["constraints"] += "- " + node.constraint() + "\n";
}

void QueryCompiler::visit(const ExampleNode& node) {
    m_examples.emplace_back(node.label(), node.code());
}

void QueryCompiler::visit(const SecurityNode& node) {
    if (!m_result_sections.contains("security")) {
        m_result_sections["security"] = "Security Requirements:\n";
    }
    m_result_sections["security"] += "- " + node.requirement() + "\n";
}

void QueryCompiler::visit(const ComplexityNode& node) {
    if (!m_result_sections.contains("complexity")) {
        m_result_sections["complexity"] = "Algorithmic Complexity Requirements:\n";
    }
    m_result_sections["complexity"] += "- " + node.complexity() + "\n";
}

void QueryCompiler::visit(const ModelNode& node) {
    m_target_model = node.model_name();
}

void QueryCompiler::visit(const FormatNode& node) {
    m_output_format = node.format_type();
}

void QueryCompiler::visit(const VariableNode& node) {
    m_variables[node.name()] = node.value();
}

// Process a string with variable interpolation (${var})
std::string QueryCompiler::interpolate_variables(const std::string& input) const {
    std::string result = input;
    size_t start_pos = 0;
    
    // Find all occurrences of ${variable_name} and replace them
    while ((start_pos = result.find("${", start_pos)) != std::string::npos) {
        size_t end_pos = result.find("}", start_pos);
        if (end_pos == std::string::npos) {
            break; // No closing bracket, stop processing
        }
        
        // Extract the variable name
        std::string var_name = result.substr(start_pos + 2, end_pos - start_pos - 2);
        
        // Look up the variable value
        auto it = m_variables.find(var_name);
        if (it != m_variables.end()) {
            // Replace the variable reference with its value
            result.replace(start_pos, end_pos - start_pos + 1, it->second);
            // Move past the replacement
            start_pos += it->second.length();
        } else {
            // Variable not found, leave it as is and move past it
            start_pos = end_pos + 1;
        }
    }
    
    return result;
}

std::string QueryCompiler::get_compiled_query() const {
    std::string query_string;

    // Add model-specific preamble if not using the default model
    if (m_target_model != "claude-3-opus") {
        query_string += "Target Model: " + m_target_model + "\n\n";
    }

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

    // Add an architecture section if it exists
    auto architecture_it = m_result_sections.find("architecture");
    if (architecture_it != m_result_sections.end()) {
        query_string += architecture_it->second + "\n";
    }

    // Add a constraints section if it exists
    auto constraints_it = m_result_sections.find("constraints");
    if (constraints_it != m_result_sections.end()) {
        query_string += constraints_it->second + "\n";
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

    // Add a security section if it exists
    auto security_it = m_result_sections.find("security");
    if (security_it != m_result_sections.end()) {
        query_string += security_it->second + "\n";
    }

    // Add a complexity section if it exists
    auto complexity_it = m_result_sections.find("complexity");
    if (complexity_it != m_result_sections.end()) {
        query_string += complexity_it->second + "\n";
    }

    // Add code examples if we have any
    if (!m_examples.empty()) {
        query_string += "Please reference these examples:\n";
        for (const auto& [label, code] : m_examples) {
            query_string += "Example - " + label + ":\n";
            query_string += "```\n" + code + "\n```\n\n";
        }
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

    // Format the output appropriately
    if (m_output_format == "json") {
        // Convert to JSON format (simplified)
        std::string json_output = "{\n";
        json_output += "  \"query\": " + std::string(query_string.empty() ? "\"\"" : "\"" + query_string + "\"") + ",\n";
        json_output += "  \"model\": \"" + m_target_model + "\",\n";
        json_output += "  \"format\": \"" + m_output_format + "\"\n";
        json_output += "}\n";
        return json_output;
    }
    
    // Process template variables
    if (!m_variables.empty()) {
        query_string = interpolate_variables(query_string);
    }

    return query_string;
}

void QueryCompiler::print_compiled_query(std::ostream& out) const {
    out << "\n=== Compiled Query ===\n\n"
        << get_compiled_query()
        << "===================\n";
}

} // namespace cql
