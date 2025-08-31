// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "ai_provider.hpp"
#include "config.hpp"
#include "http/client.hpp"
#include <nlohmann/json.hpp>
#include <memory>

namespace cql {

/**
 * @brief Anthropic Claude API provider implementation
 * 
 * Implements the AIProvider interface for Anthropic's Claude models.
 * Supports synchronous, asynchronous, and streaming operations.
 */
class AnthropicProvider : public AIProvider {
public:
    /**
     * @brief Constructor
     * @param config Configuration containing API key and settings
     */
    explicit AnthropicProvider(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~AnthropicProvider() override = default;
    
    // AIProvider interface implementation
    [[nodiscard]] ProviderResponse generate(const ProviderRequest& request) override;
    [[nodiscard]] std::future<ProviderResponse> generate_async(const ProviderRequest& request) override;
    void generate_stream(const ProviderRequest& request, StreamingCallback callback) override;
    [[nodiscard]] std::string get_provider_name() const override;
    [[nodiscard]] ProviderCapabilities get_capabilities() const override;
    [[nodiscard]] bool is_configured() const override;
    [[nodiscard]] bool validate_model(const std::string& model) const override;
    [[nodiscard]] std::optional<double> estimate_cost(const ProviderRequest& request) const override;

private:
    /**
     * @brief Convert ProviderRequest to Anthropic API JSON format
     * @param request The unified request
     * @return JSON formatted for Anthropic API
     */
    [[nodiscard]] nlohmann::json convert_request(const ProviderRequest& request) const;
    
    /**
     * @brief Parse Anthropic API response to ProviderResponse
     * @param json_response Raw JSON response from API  
     * @param latency Request latency
     * @return Parsed provider response
     */
    [[nodiscard]] ProviderResponse parse_response(const nlohmann::json& json_response, 
                                                 std::chrono::milliseconds latency) const;
    
    /**
     * @brief Create HTTP headers for Anthropic API
     * @return Map of HTTP headers
     */
    [[nodiscard]] std::map<std::string, std::string> create_headers() const;
    
    /**
     * @brief Parse streaming response chunk
     * @param chunk Raw chunk data
     * @return StreamingChunk or nullopt if invalid
     */
    [[nodiscard]] std::optional<StreamingChunk> parse_stream_chunk(const std::string& chunk) const;

    Config m_config;                                    ///< Provider configuration
    std::unique_ptr<http::ClientInterface> m_http_client; ///< HTTP client for API calls
    static constexpr const char* API_VERSION = "2023-06-01"; ///< Anthropic API version
    static constexpr const char* BASE_URL = "https://api.anthropic.com"; ///< Default base URL
};

} // namespace cql
