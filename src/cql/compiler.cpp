// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/compiler.hpp"
#include "../../include/cql/nodes.hpp"

namespace cql {

// visitor interface implementation
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
    // store the test cases, we'll format them when getting the final query
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

void QueryCompiler::visit(const OutputFormatNode& node) {
    m_api_output_format = node.format_type();
    if (!m_result_sections.contains("model_parameters")) {
        m_result_sections["model_parameters"] = "Model Parameters:\n";
    }
    m_result_sections["model_parameters"] += "- Output Format: " + node.format_type() + "\n";
}

void QueryCompiler::visit(const MaxTokensNode& node) {
    m_max_tokens = node.token_limit();
    if (!m_result_sections.contains("model_parameters")) {
        m_result_sections["model_parameters"] = "Model Parameters:\n";
    }
    m_result_sections["model_parameters"] += "- Max Tokens: " + node.token_limit() + "\n";
}

void QueryCompiler::visit(const TemperatureNode& node) {
    m_temperature = node.temperature_value();
    if (!m_result_sections.contains("model_parameters")) {
        m_result_sections["model_parameters"] = "Model Parameters:\n";
    }
    m_result_sections["model_parameters"] += "- Temperature: " + node.temperature_value() + "\n";
}

void QueryCompiler::visit(const PatternNode& node) {
    if (!m_result_sections.contains("design_patterns")) {
        m_result_sections["design_patterns"] = "Design Patterns:\n";
    }
    m_result_sections["design_patterns"] += "- " + node.pattern_desc() + "\n";
}

void QueryCompiler::visit(const StructureNode& node) {
    if (!m_result_sections.contains("file_structure")) {
        m_result_sections["file_structure"] = "File Structure:\n";
    }
    m_result_sections["file_structure"] += "- " + node.structure_def() + "\n";
}

// process a string with variable interpolation (${var})
std::string QueryCompiler::interpolate_variables(const std::string& input) const {
    std::string result = input;
    size_t start_pos = 0;
    
    // find all occurrences of ${variable_name} and replace them
    while ((start_pos = result.find("${", start_pos)) != std::string::npos) {
        size_t end_pos = result.find("}", start_pos);
        if (end_pos == std::string::npos) {
            break; // no closing bracket, stop processing
        }
        
        // extract the variable name
        std::string var_name = result.substr(start_pos + 2, end_pos - start_pos - 2);
        
        // look up the variable value
        auto it = m_variables.find(var_name);
        if (it != m_variables.end()) {
            // replace the variable reference with its value
            result.replace(start_pos, end_pos - start_pos + 1, it->second);
            // move past the replacement
            start_pos += it->second.length();
        } else {
            // variable not found, leave it as is and move past it
            start_pos = end_pos + 1;
        }
    }
    
    return result;
}

std::string QueryCompiler::get_compiled_query() const {
    std::string query_string;

    // add model-specific preamble if not using the default model
    if (m_target_model != "claude-3-opus") {
        query_string += "Target Model: " + m_target_model + "\n\n";
    }

    // add a copyright section if it exists
    auto copyright_it = m_result_sections.find("copyright");
    if (copyright_it != m_result_sections.end()) {
        query_string += copyright_it->second;
    }

    // add a code section if it exists
    auto code_it = m_result_sections.find("code");
    if (code_it != m_result_sections.end()) {
        query_string += code_it->second;
    }

    // add a context section if it exists
    auto context_it = m_result_sections.find("context");
    if (context_it != m_result_sections.end()) {
        query_string += context_it->second + "\n";
    }

    // add an architecture section if it exists
    auto architecture_it = m_result_sections.find("architecture");
    if (architecture_it != m_result_sections.end()) {
        query_string += architecture_it->second + "\n";
    }

    // add a constraints section if it exists
    auto constraints_it = m_result_sections.find("constraints");
    if (constraints_it != m_result_sections.end()) {
        query_string += constraints_it->second + "\n";
    }

    // add a dependencies section if it exists
    auto dependencies_it = m_result_sections.find("dependencies");
    if (dependencies_it != m_result_sections.end()) {
        query_string += dependencies_it->second + "\n";
    }

    // add a performance section if it exists
    auto performance_it = m_result_sections.find("performance");
    if (performance_it != m_result_sections.end()) {
        query_string += performance_it->second + "\n";
    }

    // add a security section if it exists
    auto security_it = m_result_sections.find("security");
    if (security_it != m_result_sections.end()) {
        query_string += security_it->second + "\n";
    }

    // add a complexity section if it exists
    auto complexity_it = m_result_sections.find("complexity");
    if (complexity_it != m_result_sections.end()) {
        query_string += complexity_it->second + "\n";
    }
    
    // add model parameters section if it exists
    auto model_params_it = m_result_sections.find("model_parameters");
    if (model_params_it != m_result_sections.end()) {
        query_string += model_params_it->second + "\n";
    }
    
    // add design patterns section if it exists
    auto patterns_it = m_result_sections.find("design_patterns");
    if (patterns_it != m_result_sections.end()) {
        query_string += patterns_it->second + "\n";
    }
    
    // add file structure section if it exists
    auto structure_it = m_result_sections.find("file_structure");
    if (structure_it != m_result_sections.end()) {
        query_string += structure_it->second + "\n";
    }

    // add code examples if we have any
    if (!m_examples.empty()) {
        query_string += "Please reference these examples:\n";
        for (const auto& [label, code] : m_examples) {
            query_string += "Example - " + label + ":\n";
            query_string += "```\n" + code + "\n```\n\n";
        }
    }

    // add test cases if we have any
    if (!m_test_cases.empty()) {
        query_string += "Please include tests for the following cases:\n";
        for (const auto& test_case : m_test_cases) {
            query_string += "- " + test_case + "\n";
        }
        query_string += "\n";
    }

    // add a quality assurance section as a standard footer
    query_string += "Quality Assurance Requirements:\n";
    query_string += "- All code must be well-documented with comments\n";
    query_string += "- Follow modern C++ best practices\n";
    query_string += "- Ensure proper error handling\n";
    query_string += "- Optimize for readability and maintainability\n";

    // format the output appropriately
    if (m_output_format == "json") {
        try {
            // Create a JSON object using nlohmann/json
            nlohmann::json json_obj;

            // Set up JSON object with proper multi-line formatting for the query
            json_obj["query"] = query_string;
            json_obj["model"] = m_target_model;
            json_obj["format"] = m_output_format;
            
            // Add optional parameters if they exist
            if (!m_api_output_format.empty()) {
                json_obj["output_format"] = m_api_output_format;
            }
            if (!m_max_tokens.empty()) {
                // Convert max_tokens to integer if possible
                try {
                    json_obj["max_tokens"] = std::stoi(m_max_tokens);
                } catch (const std::exception&) {
                    json_obj["max_tokens"] = m_max_tokens;
                }
            }
            if (!m_temperature.empty()) {
                // Convert temperature to float if possible
                try {
                    json_obj["temperature"] = std::stof(m_temperature);
                } catch (const std::exception&) {
                    json_obj["temperature"] = m_temperature;
                }
            }
            
            // Convert to properly formatted JSON with indentation
            return json_obj.dump(2) + "\n";
        } catch (const nlohmann::json::exception& e) {
            // If JSON creation fails, fall back to manual string building with proper escaping
            std::string json_output = "{\n";
            
            // Properly escape quotes and newlines in query_string
            std::string escaped_query = query_string;
            
            // Replace backslashes first (important to do this first)
            size_t pos = 0;
            while ((pos = escaped_query.find("\\", pos)) != std::string::npos) {
                escaped_query.replace(pos, 1, "\\\\");
                pos += 2; // Skip the replacement
            }
            
            // Replace quotes
            pos = 0;
            while ((pos = escaped_query.find("\"", pos)) != std::string::npos) {
                escaped_query.replace(pos, 1, "\\\"");
                pos += 2; // Skip the replacement
            }
            
            // Replace newlines
            pos = 0;
            while ((pos = escaped_query.find('\n', pos)) != std::string::npos) {
                escaped_query.replace(pos, 1, "\\n");
                pos += 2; // Skip the replacement
            }
            
            json_output += "  \"query\": " + (escaped_query.empty() ? "\"\"" : "\"" + escaped_query + "\"") + ",\n";
            json_output += R"(  "model": ")" + m_target_model + "\",\n";
            json_output += R"(  "format": ")" + m_output_format + "\"";
            
            // Add new parameters if they exist
            if (!m_api_output_format.empty()) {
                json_output += ",\n  \"output_format\": \"" + m_api_output_format + "\"";
            }
            if (!m_max_tokens.empty()) {
                json_output += ",\n  \"max_tokens\": " + m_max_tokens;
            }
            if (!m_temperature.empty()) {
                json_output += ",\n  \"temperature\": " + m_temperature;
            }
            
            json_output += "\n}\n";
            return json_output;
        }
    }
    
    // process template variables
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
