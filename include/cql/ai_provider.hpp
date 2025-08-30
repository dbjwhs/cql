// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <future>
#include <functional>
#include <optional>
#include <chrono>
#include <map>

namespace cql {

/**
 * @brief Request configuration for AI providers
 * 
 * Unified request structure that works across different AI providers.
 * Providers will translate this to their specific API format.
 */
struct ProviderRequest {
    std::string prompt;                    ///< Main prompt/query
    std::string model;                     ///< Model identifier (provider-specific)
    std::optional<std::string> system_prompt; ///< System/context prompt
    int max_tokens = 4096;                 ///< Maximum tokens in response
    double temperature = 0.7;               ///< Temperature for randomness (0.0-1.0)
    std::optional<double> top_p;           ///< Top-p sampling parameter
    std::vector<std::pair<std::string, std::string>> messages; ///< Conversation history
    std::map<std::string, std::string> metadata; ///< Provider-specific parameters
};

/**
 * @brief Response from AI providers
 * 
 * Unified response structure containing the generated content
 * and metadata about the generation.
 */
struct ProviderResponse {
    bool success = false;                  ///< Whether request succeeded
    std::string content;                    ///< Generated content
    std::string model_used;                ///< Actual model used
    int tokens_used = 0;                   ///< Total tokens consumed
    int prompt_tokens = 0;                 ///< Tokens in prompt
    int completion_tokens = 0;              ///< Tokens in completion
    std::chrono::milliseconds latency{0};  ///< Request latency
    std::optional<std::string> error_message; ///< Error details if failed
    std::optional<int> http_status;        ///< HTTP status code
    std::map<std::string, std::string> metadata; ///< Provider-specific response data
};

/**
 * @brief Streaming response chunk from AI providers
 * 
 * Used for streaming responses where content arrives incrementally.
 */
struct StreamingChunk {
    std::string content;                   ///< Incremental content
    bool is_final = false;                 ///< Whether this is the last chunk
    std::optional<std::string> error;      ///< Error if stream failed
};

/**
 * @brief Provider capabilities and features
 * 
 * Describes what features a specific provider supports.
 */
struct ProviderCapabilities {
    bool supports_streaming = false;       ///< Supports streaming responses
    bool supports_functions = false;       ///< Supports function calling
    bool supports_vision = false;          ///< Supports image inputs
    bool supports_async = true;            ///< Supports async operations
    std::vector<std::string> available_models; ///< List of available models
    size_t max_context_length = 0;         ///< Maximum context window
    size_t max_output_tokens = 0;          ///< Maximum output tokens
};

// Forward declaration for streaming callback
using StreamingCallback = std::function<void(const StreamingChunk&)>;

/**
 * @brief Abstract interface for AI providers
 * 
 * This interface defines the contract that all AI provider implementations
 * must follow. It provides a unified API for interacting with different
 * AI services (Anthropic, OpenAI, Google, etc.).
 * 
 * @note Implementations should handle provider-specific authentication,
 *       request formatting, and response parsing internally.
 */
class AIProvider {
public:
    virtual ~AIProvider() = default;
    
    /**
     * @brief Generate a synchronous response
     * 
     * @param request The request configuration
     * @return ProviderResponse containing the generated content
     * @throws std::runtime_error on network or API errors
     */
    [[nodiscard]] virtual ProviderResponse generate(const ProviderRequest& request) = 0;
    
    /**
     * @brief Generate an asynchronous response
     * 
     * @param request The request configuration
     * @return Future that will contain the response
     * @note The future may throw exceptions on error
     */
    [[nodiscard]] virtual std::future<ProviderResponse> generate_async(const ProviderRequest& request) = 0;
    
    /**
     * @brief Generate a streaming response
     * 
     * @param request The request configuration
     * @param callback Function called for each chunk
     * @note Callback will be called from a background thread
     */
    virtual void generate_stream(const ProviderRequest& request, StreamingCallback callback) = 0;
    
    /**
     * @brief Get the provider name
     * 
     * @return Human-readable provider name (e.g., "Anthropic", "OpenAI")
     */
    [[nodiscard]] virtual std::string get_provider_name() const = 0;
    
    /**
     * @brief Get provider capabilities
     * 
     * @return Structure describing provider features and limits
     */
    [[nodiscard]] virtual ProviderCapabilities get_capabilities() const = 0;
    
    /**
     * @brief Check if the provider is properly configured
     * 
     * @return true if provider has valid configuration (API keys, etc.)
     */
    [[nodiscard]] virtual bool is_configured() const = 0;
    
    /**
     * @brief Validate a model name for this provider
     * 
     * @param model Model identifier to validate
     * @return true if model is supported by this provider
     */
    [[nodiscard]] virtual bool validate_model(const std::string& model) const = 0;
    
    /**
     * @brief Get estimated cost for a request
     * 
     * @param request The request to estimate
     * @return Estimated cost in USD, or nullopt if not available
     */
    [[nodiscard]] virtual std::optional<double> estimate_cost(const ProviderRequest& request) const = 0;
};

} // namespace cql