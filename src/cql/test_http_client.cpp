// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "../../include/cql/http/client.hpp"
#include "../../include/cql/project_utils.hpp"
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
    
    // Launch multiple async requests
    for (int i = 0; i < 5; ++i) {
        Request req;
        req.url = "https://httpbin.org/uuid";
        req.method = "GET";
        futures.push_back(m_client->send_async(req));
    }
    
    // Wait for all to complete
    for (auto& future : futures) {
        auto response = future.get();
        EXPECT_TRUE(response.is_success());
        EXPECT_EQ(response.status_code, 200);
        EXPECT_FALSE(response.body.empty());
    }
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
