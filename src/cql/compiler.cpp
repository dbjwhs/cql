// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/compiler.hpp"
#include "../../include/cql/nodes.hpp"

namespace cql {

// visitor interface implementation
void QueryCompiler::visit(const CodeRequestNode& node) {
    std::string& code_section = m_result_sections["code"];
    code_section.reserve(100 + node.language().size() + node.description().size());
    code_section = "Please generate ";
    code_section += node.language();
    code_section += " code that:\n";
    code_section += node.description();
    code_section += "\n\n";
}

void QueryCompiler::visit(const ContextNode& node) {
    if (!m_result_sections.contains("context")) {
        m_result_sections["context"] = "Context:\n";
    }
    std::string& context_section = m_result_sections["context"];
    context_section.reserve(context_section.size() + 3 + node.context().size());
    context_section += "- ";
    context_section += node.context();
    context_section += "\n";
}

void QueryCompiler::visit(const TestNode& node) {
    // to store the test cases, we'll format them when getting the final query
    m_test_cases.insert(m_test_cases.end(),
                      node.test_cases().begin(),
                      node.test_cases().end());
}

void QueryCompiler::visit(const DependencyNode& node) {
    if (!m_result_sections.contains("dependencies")) {
        m_result_sections["dependencies"] = "Dependencies:\n";
    }
    std::string& deps_section = m_result_sections["dependencies"];
    // Pre-calculate required space
    size_t additional_space = 0;
    for (const auto& dependency : node.dependencies()) {
        additional_space += 3 + dependency.size(); // "- " + dependency + "\n"
    }
    deps_section.reserve(deps_section.size() + additional_space);
    
    for (const auto& dependency : node.dependencies()) {
        deps_section += "- ";
        deps_section += dependency;
        deps_section += "\n";
    }
}

void QueryCompiler::visit(const PerformanceNode& node) {
    if (!m_result_sections.contains("performance")) {
        m_result_sections["performance"] = "Performance Requirements:\n";
    }
    std::string& perf_section = m_result_sections["performance"];
    perf_section.reserve(perf_section.size() + 3 + node.requirement().size());
    perf_section += "- ";
    perf_section += node.requirement();
    perf_section += "\n";
}

void QueryCompiler::visit(const CopyrightNode& node) {
    std::string& copyright_message = m_result_sections["copyright"];
    copyright_message.reserve(150 + node.license().size() + node.owner().size());
    copyright_message = "Please include the following copyright header at the top of all generated files:\n";
    copyright_message += "```\n// ";
    copyright_message += node.license();
    copyright_message += "\n// Copyright (c) ";
    copyright_message += node.owner();
    copyright_message += "\n```\n\n";
}

void QueryCompiler::visit(const ArchitectureNode& node) {
    if (!m_result_sections.contains("architecture")) {
        m_result_sections["architecture"] = "Architecture Requirements:\n";
    }
    std::string& arch_section = m_result_sections["architecture"];
    arch_section.reserve(arch_section.size() + 3 + node.architecture().size());
    arch_section += "- ";
    arch_section += node.architecture();
    arch_section += "\n";
}

void QueryCompiler::visit(const ConstraintNode& node) {
    if (!m_result_sections.contains("constraints")) {
        m_result_sections["constraints"] = "Constraints:\n";
    }
    std::string& constraints_section = m_result_sections["constraints"];
    constraints_section.reserve(constraints_section.size() + 3 + node.constraint().size());
    constraints_section += "- ";
    constraints_section += node.constraint();
    constraints_section += "\n";
}

void QueryCompiler::visit(const ExampleNode& node) {
    m_examples.emplace_back(node.label(), node.code());
}

void QueryCompiler::visit(const SecurityNode& node) {
    if (!m_result_sections.contains("security")) {
        m_result_sections["security"] = "Security Requirements:\n";
    }
    std::string& security_section = m_result_sections["security"];
    security_section.reserve(security_section.size() + 3 + node.requirement().size());
    security_section += "- ";
    security_section += node.requirement();
    security_section += "\n";
}

void QueryCompiler::visit(const ComplexityNode& node) {
    if (!m_result_sections.contains("complexity")) {
        m_result_sections["complexity"] = "Algorithmic Complexity Requirements:\n";
    }
    std::string& complexity_section = m_result_sections["complexity"];
    complexity_section.reserve(complexity_section.size() + 3 + node.complexity().size());
    complexity_section += "- ";
    complexity_section += node.complexity();
    complexity_section += "\n";
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
    std::string& params_section = m_result_sections["model_parameters"];
    params_section.reserve(params_section.size() + 18 + node.format_type().size());
    params_section += "- Output Format: ";
    params_section += node.format_type();
    params_section += "\n";
}

void QueryCompiler::visit(const MaxTokensNode& node) {
    m_max_tokens = node.token_limit();
    if (!m_result_sections.contains("model_parameters")) {
        m_result_sections["model_parameters"] = "Model Parameters:\n";
    }
    std::string& params_section = m_result_sections["model_parameters"];
    params_section.reserve(params_section.size() + 15 + node.token_limit().size());
    params_section += "- Max Tokens: ";
    params_section += node.token_limit();
    params_section += "\n";
}

void QueryCompiler::visit(const TemperatureNode& node) {
    m_temperature = node.temperature_value();
    if (!m_result_sections.contains("model_parameters")) {
        m_result_sections["model_parameters"] = "Model Parameters:\n";
    }
    std::string& params_section = m_result_sections["model_parameters"];
    params_section.reserve(params_section.size() + 15 + node.temperature_value().size());
    params_section += "- Temperature: ";
    params_section += node.temperature_value();
    params_section += "\n";
}

void QueryCompiler::visit(const PatternNode& node) {
    if (!m_result_sections.contains("design_patterns")) {
        m_result_sections["design_patterns"] = "Design Patterns:\n";
    }
    std::string& patterns_section = m_result_sections["design_patterns"];
    patterns_section.reserve(patterns_section.size() + 3 + node.pattern_desc().size());
    patterns_section += "- ";
    patterns_section += node.pattern_desc();
    patterns_section += "\n";
}

void QueryCompiler::visit(const StructureNode& node) {
    if (!m_result_sections.contains("file_structure")) {
        m_result_sections["file_structure"] = "File Structure:\n";
    }
    std::string& structure_section = m_result_sections["file_structure"];
    structure_section.reserve(structure_section.size() + 3 + node.structure_def().size());
    structure_section += "- ";
    structure_section += node.structure_def();
    structure_section += "\n";
}

// process a string with variable interpolation (${var})
std::string QueryCompiler::interpolate_variables(const std::string& input) const {
    std::string result;
    result.reserve(input.size() * 2); // Reserve extra space for variable expansion
    result = input;
    size_t start_pos = 0;
    
    // find all occurrences of ${variable_name} and replace them
    while ((start_pos = result.find("${", start_pos)) != std::string::npos) {
        const size_t end_pos = result.find('}', start_pos);
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
            // the variable isn't found, leave it as is and move past it
            start_pos = end_pos + 1;
        }
    }
    
    return result;
}

std::string QueryCompiler::get_compiled_query() const {
    std::string query_string;
    // Pre-allocate reasonable capacity to reduce reallocations
    query_string.reserve(2048);

    // add a model-specific preamble if not using the default model
    if (m_target_model != "claude-3-opus") {
        query_string += "Target Model: ";
        query_string += m_target_model;
        query_string += "\n\n";
    }

    // add a copyright section if it exists
    if (const auto copyright_it = m_result_sections.find("copyright"); copyright_it != m_result_sections.end()) {
        query_string += copyright_it->second;
    }

    // add a code section if it exists
    if (const auto code_it = m_result_sections.find("code"); code_it != m_result_sections.end()) {
        query_string += code_it->second;
    }

    // add a context section if it exists
    if (const auto context_it = m_result_sections.find("context"); context_it != m_result_sections.end()) {
        query_string += context_it->second;
        query_string += "\n";
    }

    // add an architecture section if it exists
    if (const auto architecture_it = m_result_sections.find("architecture"); architecture_it != m_result_sections.end()) {
        query_string += architecture_it->second;
        query_string += "\n";
    }

    // add a constraints section if it exists
    if (const auto constraints_it = m_result_sections.find("constraints"); constraints_it != m_result_sections.end()) {
        query_string += constraints_it->second;
        query_string += "\n";
    }

    // add a dependencies section if it exists
    if (const auto dependencies_it = m_result_sections.find("dependencies"); dependencies_it != m_result_sections.end()) {
        query_string += dependencies_it->second;
        query_string += "\n";
    }

    // add a performance section if it exists
    if (const auto performance_it = m_result_sections.find("performance"); performance_it != m_result_sections.end()) {
        query_string += performance_it->second;
        query_string += "\n";
    }

    // add a security section if it exists
    if (const auto security_it = m_result_sections.find("security"); security_it != m_result_sections.end()) {
        query_string += security_it->second;
        query_string += "\n";
    }

    // add a complexity section if it exists
    if (const auto complexity_it = m_result_sections.find("complexity"); complexity_it != m_result_sections.end()) {
        query_string += complexity_it->second;
        query_string += "\n";
    }
    
    // add a model parameters section if it exists
    if (const auto model_params_it = m_result_sections.find("model_parameters"); model_params_it != m_result_sections.end()) {
        query_string += model_params_it->second;
        query_string += "\n";
    }
    
    // add design patterns section if it exists
    if (const auto patterns_it = m_result_sections.find("design_patterns"); patterns_it != m_result_sections.end()) {
        query_string += patterns_it->second;
        query_string += "\n";
    }
    
    // add a file structure section if it exists
    if (const auto structure_it = m_result_sections.find("file_structure"); structure_it != m_result_sections.end()) {
        query_string += structure_it->second;
        query_string += "\n";
    }

    // add code examples if we have any
    if (!m_examples.empty()) {
        query_string += "Please reference these examples:\n";
        for (const auto& [label, code] : m_examples) {
            query_string += "Example - ";
            query_string += label;
            query_string += ":\n```\n";
            query_string += code;
            query_string += "\n```\n\n";
        }
    }

    // add test cases if we have any
    if (!m_test_cases.empty()) {
        query_string += "Please include tests for the following cases:\n";
        for (const auto& test_case : m_test_cases) {
            query_string += "- ";
            query_string += test_case;
            query_string += "\n";
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

            // Set up a JSON object with proper multi-line formatting for the query
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
        } catch (const nlohmann::json::exception&) {
            // If JSON creation fails, fall back to manual string building with proper escaping
            std::string json_output;
            json_output.reserve(query_string.size() * 2 + 500); // Reserve extra space for JSON formatting
            json_output = "{\n";
            
            // Properly escape quotes and newlines in query_string
            std::string escaped_query;
            escaped_query.reserve(query_string.size() * 2); // Reserve extra space for escaping
            escaped_query = query_string;
            
            // Replace backslashes first (important to do this first)
            size_t pos = 0;
            while ((pos = escaped_query.find('\\', pos)) != std::string::npos) {
                escaped_query.replace(pos, 1, "\\\\");
                pos += 2; // Skip the replacement
            }
            
            // Replace quotes
            pos = 0;
            while ((pos = escaped_query.find('\"', pos)) != std::string::npos) {
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
                json_output += ",\n  \"output_format\": \"";
                json_output += m_api_output_format;
                json_output += "\"";
            }
            if (!m_max_tokens.empty()) {
                json_output += ",\n  \"max_tokens\": ";
                json_output += m_max_tokens;
            }
            if (!m_temperature.empty()) {
                json_output += ",\n  \"temperature\": ";
                json_output += m_temperature;
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
