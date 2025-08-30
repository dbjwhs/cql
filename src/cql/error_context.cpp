// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/error_context.hpp"
#include "../../include/cql/project_utils.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>

namespace cql {

std::string ContextualException::format_full_message() const {
    std::ostringstream oss;
    
    // Start with original error
    oss << "Error: " << m_original_message;
    
    // Add context chain in reverse order (most recent first)
    if (!m_context_chain.empty()) {
        oss << "\n\nContext chain (most recent first):";
        
        for (auto it = m_context_chain.rbegin(); it != m_context_chain.rend(); ++it) {
            const auto& layer = *it;
            oss << "\n  → " << layer.operation;
            
            if (!layer.location.empty()) {
                oss << " at " << layer.location;
            }
            
            // Add details if any
            if (!layer.details.empty()) {
                oss << "\n    Details:";
                for (const auto& [key, value] : layer.details) {
                    oss << "\n      " << key << ": " << value;
                }
            }
            
            // Add timestamp for debugging
            auto time_t = std::chrono::system_clock::to_time_t(layer.timestamp);
            oss << "\n    Time: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        }
    }
    
    return oss.str();
}

std::string ContextualException::get_structured_info() const {
    std::ostringstream oss;
    
    // Original error
    oss << "original_error: " << m_original_message << "\n";
    
    // Context layers
    oss << "context_layers: " << m_context_chain.size() << "\n";
    for (size_t i = 0; i < m_context_chain.size(); ++i) {
        const auto& layer = m_context_chain[i];
        oss << "layer_" << i << "_operation: " << layer.operation << "\n";
        oss << "layer_" << i << "_location: " << layer.location << "\n";
        
        for (const auto& [key, value] : layer.details) {
            oss << "layer_" << i << "_" << key << ": " << value << "\n";
        }
        
        auto time_t = std::chrono::system_clock::to_time_t(layer.timestamp);
        oss << "layer_" << i << "_timestamp: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
    }
    
    return oss.str();
}

std::string ContextualException::get_user_summary() const {
    // For user display, show a simplified version
    if (m_context_chain.empty()) {
        return m_original_message;
    }
    
    // Get the most recent context layer
    const auto& recent = m_context_chain.back();
    std::ostringstream oss;
    
    // Show what was being done and any relevant details
    oss << recent.operation;
    
    // Add key details that are useful for users
    if (auto it = recent.details.find("file"); it != recent.details.end()) {
        oss << " (file: " << it->second << ")";
    }
    if (auto it = recent.details.find("template"); it != recent.details.end()) {
        oss << " (template: " << it->second << ")";
    }
    
    oss << ": " << m_original_message;
    
    return oss.str();
}

namespace error_context_utils {

std::string sanitize_error_for_user(const std::string& error_message) {
    std::string sanitized = error_message;
    
    // Remove potentially sensitive information
    // First, remove potential API keys or tokens (before path processing)
    std::regex token_regex(R"(\b[a-zA-Z0-9_-]{20,}\b)");
    sanitized = std::regex_replace(sanitized, token_regex, "<redacted>");
    
    // Remove memory addresses
    std::regex addr_regex(R"(\b0x[0-9a-fA-F]+\b)");
    sanitized = std::regex_replace(sanitized, addr_regex, "<address>");
    
    // Remove file system paths but preserve filenames
    // Match paths like /path/to/file.txt and replace with just file.txt
    std::regex path_regex(R"((?:[/\\][^/\\]+)+[/\\]([^/\\]+))");
    sanitized = std::regex_replace(sanitized, path_regex, "$1");
    
    return sanitized;
}

void log_contextual_exception(const ContextualException& exception) {
    auto& logger = Logger::getInstance();
    
    // Log the user-friendly summary at ERROR level
    logger.log(LogLevel::ERROR, "Error occurred: ", exception.get_user_summary());
    
    // Log structured information at DEBUG level for troubleshooting
    logger.log(LogLevel::DEBUG, "Full error context:\n", exception.get_structured_info());
    
    // Log the complete formatted message at DEBUG level
    logger.log(LogLevel::DEBUG, "Complete error details:\n", exception.what());
}

} // namespace error_context_utils

} // namespace cql
