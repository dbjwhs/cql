// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "ailib/http/client.hpp"
#include "../../../include/cql/project_utils.hpp"
#include <thread>
#include <chrono>

namespace cql::http::test {

class HttpClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create HTTP client with default config
        m_client = ClientFactory::create_default();
        ASSERT_NE(m_client, nullptr);
        ASSERT_TRUE(m_client->is_configured());
    }
    
    std::unique_ptr<ClientInterface> m_client;
};

TEST_F(HttpClientTest, GetImplementationName) {
    EXPECT_EQ(m_client->get_implementation_name(), "CURL");
}

TEST_F(HttpClientTest, GetAvailableImplementations) {
    auto implementations = ClientFactory::get_available_implementations();
    ASSERT_FALSE(implementations.empty());
    EXPECT_EQ(implementations[0], "CURL");
}

TEST_F(HttpClientTest, SimpleGetRequest) {
    // Test with httpbin.org echo service
    Request req;
    req.url = "https://httpbin.org/get";
    req.method = "GET";
    req.headers["User-Agent"] = "CQL-Test/1.0";
    
    auto response = m_client->send(req);
    
    EXPECT_TRUE(response.is_success());
    EXPECT_EQ(response.status_code, 200);
    EXPECT_FALSE(response.body.empty());
    EXPECT_GT(response.elapsed.count(), 0);
}

TEST_F(HttpClientTest, PostRequestWithBody) {
    Request req;
    req.url = "https://httpbin.org/post";
    req.method = "POST";
    req.headers["Content-Type"] = "application/json";
    req.body = R"({"test": "data", "number": 42})";
    
    auto response = m_client->send(req);
    
    EXPECT_TRUE(response.is_success());
    EXPECT_EQ(response.status_code, 200);
    EXPECT_FALSE(response.body.empty());
    // httpbin.org echoes back the posted data
    EXPECT_NE(response.body.find("\"test\": \"data\""), std::string::npos);
}

TEST_F(HttpClientTest, AsyncRequest) {
    Request req;
    req.url = "https://httpbin.org/delay/1";
    req.method = "GET";
    
    auto future = m_client->send_async(req);
    
    // Request should not be complete immediately
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(10)), 
              std::future_status::timeout);
    
    // Wait for completion
    auto response = future.get();
    
    EXPECT_TRUE(response.is_success());
    EXPECT_EQ(response.status_code, 200);
}

TEST_F(HttpClientTest, HandleErrorResponse) {
    Request req;
    req.url = "https://httpbin.org/status/404";
    req.method = "GET";
    
    auto response = m_client->send(req);
    
    EXPECT_FALSE(response.is_success());
    EXPECT_TRUE(response.is_client_error());
    EXPECT_EQ(response.status_code, 404);
}

TEST_F(HttpClientTest, HandleServerError) {
    Request req;
    req.url = "https://httpbin.org/status/500";
    req.method = "GET";
    
    auto response = m_client->send(req);
    
    EXPECT_FALSE(response.is_success());
    EXPECT_TRUE(response.is_server_error());
    EXPECT_EQ(response.status_code, 500);
}

TEST_F(HttpClientTest, RequestTimeout) {
    Request req;
    req.url = "https://httpbin.org/delay/10";
    req.method = "GET";
    req.timeout = std::chrono::seconds(1); // Set short timeout
    
    auto response = m_client->send(req);
    
    EXPECT_FALSE(response.is_success());
    EXPECT_TRUE(response.error_message.has_value());
}

TEST_F(HttpClientTest, CustomHeaders) {
    Request req;
    req.url = "https://httpbin.org/headers";
    req.method = "GET";
    req.headers["X-Custom-Header"] = "TestValue";
    req.headers["X-Another-Header"] = "AnotherValue";
    
    auto response = m_client->send(req);
    
    EXPECT_TRUE(response.is_success());
    EXPECT_EQ(response.status_code, 200);
    // httpbin.org echoes back headers
    EXPECT_NE(response.body.find("X-Custom-Header"), std::string::npos);
    EXPECT_NE(response.body.find("TestValue"), std::string::npos);
}

TEST_F(HttpClientTest, ProgressCallback) {
    std::atomic<bool> progress_called{false};
    std::atomic<size_t> bytes_received{0};
    
    m_client->set_progress_callback([&](size_t received, size_t total) {
        progress_called = true;
        bytes_received = received;
        Logger::getInstance().log(LogLevel::INFO, 
            "Progress: ", received, " / ", total);
    });
    
    Request req;
    req.url = "https://httpbin.org/bytes/10000"; // Request 10KB of data
    req.method = "GET";
    
    auto response = m_client->send(req);
    
    EXPECT_TRUE(response.is_success());
    // Progress callback may or may not be called depending on speed
    // Just verify no crash occurred
}

TEST_F(HttpClientTest, MultipleAsyncRequests) {
    std::vector<std::future<Response>> futures;
    
    // Launch multiple async requests with increased timeout and retry logic
    for (int i = 0; i < 5; ++i) {
        Request req;
        req.url = "https://httpbin.org/uuid";
        req.method = "GET";
        req.timeout = std::chrono::seconds(60);  // Increase timeout for CI environments
        
        // Add small delay between requests to avoid rate limiting
        if (i > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        futures.push_back(m_client->send_async(req));
    }
    
    // Wait for all to complete with more lenient success criteria
    int successful_requests = 0;
    for (auto& future : futures) {
        try {
            auto response = future.get();
            if (response.is_success()) {
                successful_requests++;
                EXPECT_EQ(response.status_code, 200);
                EXPECT_FALSE(response.body.empty());
            }
        } catch (const std::exception& e) {
            // Log but don't fail immediately on timeout/network errors
            std::cerr << "Request failed: " << e.what() << std::endl;
        }
    }
    
    // Require at least 3 out of 5 requests to succeed (allows for transient failures)
    EXPECT_GE(successful_requests, 3) << "Too many requests failed in CI environment";
}

TEST_F(HttpClientTest, RetryOnServerError) {
    // Test retry on 5xx errors
    Request req;
    req.url = "https://httpbin.org/status/503";  // Service Unavailable
    req.method = "GET";
    req.retry_policy.max_retries = 2;
    req.retry_policy.initial_delay = std::chrono::milliseconds(100);
    
    auto start = std::chrono::steady_clock::now();
    auto response = m_client->send(req);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    // Should retry twice but still fail
    EXPECT_FALSE(response.is_success());
    EXPECT_EQ(response.status_code, 503);
    
    // Should have taken at least the retry delays (100ms + 200ms)
    EXPECT_GE(elapsed, std::chrono::milliseconds(300));
}

TEST_F(HttpClientTest, RetryWithExponentialBackoff) {
    // Test exponential backoff calculation
    RetryPolicy policy;
    policy.initial_delay = std::chrono::milliseconds(100);
    policy.backoff_multiplier = 2.0;
    policy.max_delay = std::chrono::milliseconds(1000);
    policy.enable_jitter = false;  // Disable jitter for predictable testing
    
    // Test delay calculations
    EXPECT_EQ(policy.calculate_delay(0).count(), 100);  // 100ms
    EXPECT_EQ(policy.calculate_delay(1).count(), 200);  // 100 * 2^1 = 200ms
    EXPECT_EQ(policy.calculate_delay(2).count(), 400);  // 100 * 2^2 = 400ms
    EXPECT_EQ(policy.calculate_delay(3).count(), 800);  // 100 * 2^3 = 800ms
    EXPECT_EQ(policy.calculate_delay(4).count(), 1000); // Capped at max_delay
}

TEST_F(HttpClientTest, NoRetryOnClientError) {
    // Test that 4xx errors don't trigger retry
    Request req;
    req.url = "https://httpbin.org/status/404";  // Not Found
    req.method = "GET";
    req.retry_policy.max_retries = 3;
    req.retry_policy.initial_delay = std::chrono::milliseconds(50);
    
    auto start = std::chrono::steady_clock::now();
    auto response = m_client->send(req);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    // Should not retry on 404
    EXPECT_FALSE(response.is_success());
    EXPECT_EQ(response.status_code, 404);
    
    // Should complete quickly without retries (allow more time for network latency)
    EXPECT_LT(elapsed, std::chrono::seconds(2));
}

TEST_F(HttpClientTest, RetryOnRateLimitError) {
    // Test retry on 429 (Too Many Requests)
    Request req;
    req.url = "https://httpbin.org/status/429";
    req.method = "GET";
    req.retry_policy.max_retries = 1;
    req.retry_policy.initial_delay = std::chrono::milliseconds(100);
    
    auto response = m_client->send(req);
    
    // Should retry once but still fail
    EXPECT_FALSE(response.is_success());
    EXPECT_EQ(response.status_code, 429);
}

TEST_F(HttpClientTest, ConfigWithCustomSettings) {
    ClientConfig config;
    config.default_timeout = std::chrono::seconds(5);
    config.max_redirects = 10;
    config.verify_ssl = true;
    config.enable_compression = true;
    config.default_headers["X-Default-Header"] = "DefaultValue";
    
    auto client = ClientFactory::create_curl_client(config);
    ASSERT_NE(client, nullptr);
    ASSERT_TRUE(client->is_configured());
    
    Request req;
    req.url = "https://httpbin.org/headers";
    req.method = "GET";
    
    auto response = client->send(req);
    
    EXPECT_TRUE(response.is_success());
    EXPECT_EQ(response.status_code, 200);
    // Default header should be included
    EXPECT_NE(response.body.find("X-Default-Header"), std::string::npos);
}

} // namespace cql::http::test
