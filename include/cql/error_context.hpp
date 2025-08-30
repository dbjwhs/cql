// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <chrono>

namespace cql {

/**
 * @brief Represents a single layer of error context information
 */
struct ErrorContextLayer {
    std::string operation;           ///< What operation was being performed
    std::string location;            ///< Where the operation was happening (file, function, etc.)
    std::unordered_map<std::string, std::string> details; ///< Additional contextual details
    std::chrono::system_clock::time_point timestamp;      ///< When this context was added
    
    ErrorContextLayer(std::string op, std::string loc) 
        : operation(std::move(op)), location(std::move(loc)), 
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Enhanced exception class that preserves error context chain
 */
class ContextualException : public std::exception {
private:
    std::string m_original_message;
    std::vector<ErrorContextLayer> m_context_chain;
    mutable std::string m_cached_message; ///< Cached formatted message
    mutable bool m_message_dirty = true;   ///< Whether cached message needs refresh

public:
    /**
     * @brief Construct from original exception
     */
    explicit ContextualException(const std::exception& original)
        : m_original_message(original.what()) {}
    
    /**
     * @brief Construct with original message
     */
    explicit ContextualException(std::string original_message)
        : m_original_message(std::move(original_message)) {}
    
    /**
     * @brief Add context layer to the error chain
     */
    ContextualException& add_context(const std::string& operation, const std::string& location) {
        m_context_chain.emplace_back(operation, location);
        m_message_dirty = true;
        return *this;
    }
    
    /**
     * @brief Add context with additional details
     */
    ContextualException& add_context(const std::string& operation, const std::string& location,
                                   const std::unordered_map<std::string, std::string>& details) {
        auto& layer = m_context_chain.emplace_back(operation, location);
        layer.details = details;
        m_message_dirty = true;
        return *this;
    }
    
    /**
     * @brief Add a single detail to the most recent context layer
     */
    ContextualException& add_detail(const std::string& key, const std::string& value) {
        if (!m_context_chain.empty()) {
            m_context_chain.back().details[key] = value;
            m_message_dirty = true;
        }
        return *this;
    }
    
    /**
     * @brief Get the original error message
     */
    [[nodiscard]] const std::string& get_original_message() const noexcept {
        return m_original_message;
    }
    
    /**
     * @brief Get the context chain
     */
    [[nodiscard]] const std::vector<ErrorContextLayer>& get_context_chain() const noexcept {
        return m_context_chain;
    }
    
    /**
     * @brief Get formatted error message with full context
     */
    [[nodiscard]] const char* what() const noexcept override {
        if (m_message_dirty) {
            m_cached_message = format_full_message();
            m_message_dirty = false;
        }
        return m_cached_message.c_str();
    }
    
    /**
     * @brief Get structured error information for logging
     */
    [[nodiscard]] std::string get_structured_info() const;
    
    /**
     * @brief Get compact error summary for user display
     */
    [[nodiscard]] std::string get_user_summary() const;

private:
    [[nodiscard]] std::string format_full_message() const;
};

/**
 * @brief Fluent builder for creating contextual exceptions
 */
class ErrorContextBuilder {
private:
    std::unique_ptr<ContextualException> m_exception;

public:
    /**
     * @brief Start building from an existing exception
     */
    static ErrorContextBuilder from(const std::exception& original) {
        ErrorContextBuilder builder;
        builder.m_exception = std::make_unique<ContextualException>(original);
        return builder;
    }
    
    /**
     * @brief Start building with an original message
     */
    static ErrorContextBuilder with_message(const std::string& message) {
        ErrorContextBuilder builder;
        builder.m_exception = std::make_unique<ContextualException>(message);
        return builder;
    }
    
    /**
     * @brief Add operation context
     */
    ErrorContextBuilder& operation(const std::string& op) {
        if (m_exception) {
            m_exception->add_context(op, "");
        }
        return *this;
    }
    
    /**
     * @brief Add location context
     */
    ErrorContextBuilder& at(const std::string& location) {
        if (m_exception && !m_exception->get_context_chain().empty()) {
            // Update the location of the most recent context layer
            auto& chain = const_cast<std::vector<ErrorContextLayer>&>(m_exception->get_context_chain());
            chain.back().location = location;
        }
        return *this;
    }
    
    /**
     * @brief Add contextual detail
     */
    ErrorContextBuilder& detail(const std::string& key, const std::string& value) {
        if (m_exception) {
            m_exception->add_detail(key, value);
        }
        return *this;
    }
    
    /**
     * @brief Add file context detail
     */
    ErrorContextBuilder& file(const std::string& filename) {
        return detail("file", filename);
    }
    
    /**
     * @brief Add template context detail
     */
    ErrorContextBuilder& template_name(const std::string& name) {
        return detail("template", name);
    }
    
    /**
     * @brief Add parameter context detail
     */
    ErrorContextBuilder& parameter(const std::string& param_name, const std::string& param_value) {
        return detail("parameter_" + param_name, param_value);
    }
    
    /**
     * @brief Build and return the contextual exception
     */
    ContextualException build() {
        if (m_exception) {
            return std::move(*m_exception);
        }
        return ContextualException("Unknown error");
    }
};

/**
 * @brief Utility macros for adding context with source location
 */
#define CQL_ERROR_CONTEXT(operation) \
    cql::ErrorContextBuilder::with_message("").operation(operation).at(__FILE__ ":" + std::to_string(__LINE__))

#define CQL_ADD_CONTEXT(exception, operation) \
    cql::ErrorContextBuilder::from(exception).operation(operation).at(__FILE__ ":" + std::to_string(__LINE__))

/**
 * @brief Utility functions for common error context patterns
 */
namespace error_context_utils {
    
    /**
     * @brief Wrap file operation with context preservation
     */
    template<typename Func>
    auto with_file_context(const std::string& filename, const std::string& operation, Func&& func) -> decltype(func()) {
        try {
            return func();
        } catch (const std::exception& e) {
            throw ErrorContextBuilder::from(e)
                .operation(operation)
                .file(filename)
                .at(__FILE__ ":" + std::to_string(__LINE__))
                .build();
        }
    }
    
    /**
     * @brief Wrap template operation with context preservation
     */
    template<typename Func>
    auto with_template_context(const std::string& template_name, const std::string& operation, Func&& func) -> decltype(func()) {
        try {
            return func();
        } catch (const std::exception& e) {
            throw ErrorContextBuilder::from(e)
                .operation(operation)
                .template_name(template_name)
                .at(__FILE__ ":" + std::to_string(__LINE__))
                .build();
        }
    }
    
    /**
     * @brief Create safe error message for user display (strips sensitive info)
     */
    [[nodiscard]] std::string sanitize_error_for_user(const std::string& error_message);
    
    /**
     * @brief Log contextual exception with structured information
     */
    void log_contextual_exception(const ContextualException& exception);
    
} // namespace error_context_utils

} // namespace cql
