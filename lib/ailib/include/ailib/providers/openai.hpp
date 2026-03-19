// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "../core/provider.hpp"
#include "../core/config.hpp"
#include "../http/client.hpp"
#include <nlohmann/json.hpp>
#include <memory>

namespace cql {

/**
 * @brief OpenAI API provider implementation
 *
 * Implements the AIProvider interface for OpenAI's GPT models.
 * Supports synchronous, asynchronous, and streaming operations.
 */
class OpenAIProvider : public AIProvider {
public:
    explicit OpenAIProvider(const Config& config);
    ~OpenAIProvider() override = default;

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
    [[nodiscard]] nlohmann::json convert_request(const ProviderRequest& request) const;
    [[nodiscard]] ProviderResponse parse_response(const nlohmann::json& json_response,
                                                 std::chrono::milliseconds latency) const;
    [[nodiscard]] std::map<std::string, std::string> create_headers() const;
    [[nodiscard]] std::optional<StreamingChunk> parse_stream_chunk(const std::string& chunk) const;

    Config m_config;
    std::unique_ptr<http::ClientInterface> m_http_client;
    static constexpr const char* BASE_URL = "https://api.openai.com";
};

} // namespace cql
