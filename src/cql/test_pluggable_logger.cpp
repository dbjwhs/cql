// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "../../include/cql/logger_interface.hpp"
#include "../../include/cql/logger_manager.hpp"
#include "../../include/cql/logger_adapters.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace cql::test {

class PluggableLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        m_temp_dir = std::filesystem::temp_directory_path() / "cql_logger_test";
        std::filesystem::create_directories(m_temp_dir);
        
        // Ensure clean state
        LoggerManager::shutdown();
    }
    
    void TearDown() override {
        // Cleanup
        LoggerManager::shutdown();
        
        // Clean up temporary files
        if (std::filesystem::exists(m_temp_dir)) {
            std::filesystem::remove_all(m_temp_dir);
        }
    }
    
    std::filesystem::path m_temp_dir;
};

// Custom test logger for verification
class TestLogger : public LoggerInterface {
public:
    struct LogEntry {
        LogLevel level;
        std::string message;
    };
    
    void log(LogLevel level, const std::string& message) override {
        if (is_level_enabled(level)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_entries.push_back({level, message});
        }
    }
    
    bool is_level_enabled(LogLevel level) const override {
        return level >= m_min_level;
    }
    
    void flush() override {
        m_flush_called = true;
    }
    
    // Test helpers
    void set_min_level(LogLevel level) { m_min_level = level; }
    
    std::vector<LogEntry> get_entries() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_entries;
    }
    
    void clear_entries() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.clear();
    }
    
    bool was_flush_called() const { return m_flush_called; }
    
private:
    mutable std::mutex m_mutex;
    std::vector<LogEntry> m_entries;
    LogLevel m_min_level{LogLevel::DEBUG};
    bool m_flush_called{false};
};

TEST_F(PluggableLoggerTest, DefaultConsoleLogger) {
    auto logger = std::make_unique<DefaultConsoleLogger>();
    
    // Test level filtering
    logger->set_min_level(LogLevel::INFO);
    EXPECT_FALSE(logger->is_level_enabled(LogLevel::DEBUG));
    EXPECT_TRUE(logger->is_level_enabled(LogLevel::INFO));
    EXPECT_TRUE(logger->is_level_enabled(LogLevel::ERROR));
    
    // Test colored output setting
    logger->set_colored_output(true);
    logger->set_colored_output(false);
    
    // Test logging (output goes to console, so we just verify it doesn't crash)
    EXPECT_NO_THROW(logger->log(LogLevel::INFO, "Test message"));
    EXPECT_NO_THROW(logger->flush());
}

TEST_F(PluggableLoggerTest, NullLogger) {
    auto logger = std::make_unique<NullLogger>();
    
    // Null logger should disable all levels
    EXPECT_FALSE(logger->is_level_enabled(LogLevel::DEBUG));
    EXPECT_FALSE(logger->is_level_enabled(LogLevel::INFO));
    EXPECT_FALSE(logger->is_level_enabled(LogLevel::ERROR));
    EXPECT_FALSE(logger->is_level_enabled(LogLevel::CRITICAL));
    
    // Should not throw on any operations
    EXPECT_NO_THROW(logger->log(LogLevel::ERROR, "This should be ignored"));
    EXPECT_NO_THROW(logger->flush());
}

TEST_F(PluggableLoggerTest, CallbackLogger) {
    std::vector<std::pair<LogLevel, std::string>> logged_messages;
    
    auto callback = [&logged_messages](LogLevel level, const char* message) {
        logged_messages.emplace_back(level, message);
    };
    
    auto level_filter = [](LogLevel level) {
        return level >= LogLevel::INFO; // Filter out DEBUG
    };
    
    auto logger = std::make_unique<CallbackLogger>(callback, level_filter);
    
    // Test level filtering
    EXPECT_FALSE(logger->is_level_enabled(LogLevel::DEBUG));
    EXPECT_TRUE(logger->is_level_enabled(LogLevel::INFO));
    
    // Test logging
    logger->log(LogLevel::DEBUG, "Debug message"); // Should be filtered
    logger->log(LogLevel::INFO, "Info message");   // Should be logged
    logger->log(LogLevel::ERROR, "Error message"); // Should be logged
    
    EXPECT_EQ(logged_messages.size(), 2);
    EXPECT_EQ(logged_messages[0].first, LogLevel::INFO);
    EXPECT_EQ(logged_messages[0].second, "Info message");
    EXPECT_EQ(logged_messages[1].first, LogLevel::ERROR);
    EXPECT_EQ(logged_messages[1].second, "Error message");
    
    // Test flush (should not throw)
    EXPECT_NO_THROW(logger->flush());
}

TEST_F(PluggableLoggerTest, LoggerManager_DefaultInitialization) {
    // Auto-initialization should work
    EXPECT_FALSE(LoggerManager::is_initialized());
    
    LoggerManager::log(LogLevel::NORMAL, "Test message");
    EXPECT_TRUE(LoggerManager::is_initialized());
    
    // Should be able to check levels and flush
    // Default minimum level is NORMAL, so INFO should be disabled, NORMAL+ should be enabled
    EXPECT_FALSE(LoggerManager::is_level_enabled(LogLevel::INFO));  // Below minimum
    EXPECT_TRUE(LoggerManager::is_level_enabled(LogLevel::NORMAL));  // At minimum
    EXPECT_TRUE(LoggerManager::is_level_enabled(LogLevel::ERROR));   // Above minimum
    EXPECT_NO_THROW(LoggerManager::flush());
}

TEST_F(PluggableLoggerTest, LoggerManager_CustomLogger) {
    auto test_logger = std::make_unique<TestLogger>();
    auto* logger_ptr = test_logger.get();
    
    LoggerManager::initialize(std::move(test_logger));
    EXPECT_TRUE(LoggerManager::is_initialized());
    
    // Test logging through manager
    LoggerManager::log(LogLevel::INFO, "Test message 1");
    LoggerManager::log(LogLevel::ERROR, "Test message 2");
    LoggerManager::flush();
    
    auto entries = logger_ptr->get_entries();
    EXPECT_EQ(entries.size(), 2);
    EXPECT_EQ(entries[0].level, LogLevel::INFO);
    EXPECT_EQ(entries[0].message, "Test message 1");
    EXPECT_EQ(entries[1].level, LogLevel::ERROR);
    EXPECT_EQ(entries[1].message, "Test message 2");
    EXPECT_TRUE(logger_ptr->was_flush_called());
}

TEST_F(PluggableLoggerTest, LoggerManager_CallbackInitialization) {
    std::vector<std::pair<LogLevel, std::string>> messages;
    
    auto callback = [&messages](LogLevel level, const char* message) {
        messages.emplace_back(level, message);
    };
    
    LoggerManager::initialize_with_callback(callback);
    EXPECT_TRUE(LoggerManager::is_initialized());
    
    LoggerManager::log(LogLevel::INFO, "Callback test");
    
    EXPECT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].first, LogLevel::INFO);
    EXPECT_EQ(messages[0].second, "Callback test");
}

TEST_F(PluggableLoggerTest, LoggerManager_NullInitialization) {
    LoggerManager::initialize_null();
    EXPECT_TRUE(LoggerManager::is_initialized());
    
    // All levels should be disabled
    EXPECT_FALSE(LoggerManager::is_level_enabled(LogLevel::DEBUG));
    EXPECT_FALSE(LoggerManager::is_level_enabled(LogLevel::INFO));
    EXPECT_FALSE(LoggerManager::is_level_enabled(LogLevel::ERROR));
    EXPECT_FALSE(LoggerManager::is_level_enabled(LogLevel::CRITICAL));
    
    // Logging should not crash
    EXPECT_NO_THROW(LoggerManager::log(LogLevel::ERROR, "This should be ignored"));
}

TEST_F(PluggableLoggerTest, LoggerManager_ConvenienceMethods) {
    auto test_logger = std::make_unique<TestLogger>();
    auto* logger_ptr = test_logger.get();
    
    LoggerManager::initialize(std::move(test_logger));
    
    // Test convenience methods
    LoggerManager::log_debug("Debug message");
    LoggerManager::log_info("Info message");
    LoggerManager::log_normal("Normal message");
    LoggerManager::log_error("Error message");
    LoggerManager::log_critical("Critical message");
    
    auto entries = logger_ptr->get_entries();
    EXPECT_EQ(entries.size(), 5);
    EXPECT_EQ(entries[0].level, LogLevel::DEBUG);
    EXPECT_EQ(entries[1].level, LogLevel::INFO);
    EXPECT_EQ(entries[2].level, LogLevel::NORMAL);
    EXPECT_EQ(entries[3].level, LogLevel::ERROR);
    EXPECT_EQ(entries[4].level, LogLevel::CRITICAL);
}

TEST_F(PluggableLoggerTest, TemporaryLogger) {
    auto main_logger = std::make_unique<TestLogger>();
    auto* main_logger_ptr = main_logger.get();
    
    LoggerManager::initialize(std::move(main_logger));
    LoggerManager::log(LogLevel::INFO, "Main logger message");
    
    {
        auto temp_logger = std::make_unique<TestLogger>();
        auto* temp_logger_ptr = temp_logger.get();
        
        TemporaryLogger temp_scope(std::move(temp_logger));
        LoggerManager::log(LogLevel::INFO, "Temporary logger message");
        
        // Temporary logger should have received the message
        auto temp_entries = temp_logger_ptr->get_entries();
        EXPECT_EQ(temp_entries.size(), 1);
        EXPECT_EQ(temp_entries[0].message, "Temporary logger message");
    }
    
    // After scope, should be back to main logger
    LoggerManager::log(LogLevel::INFO, "Back to main logger");
    
    auto main_entries = main_logger_ptr->get_entries();
    EXPECT_EQ(main_entries.size(), 2);
    EXPECT_EQ(main_entries[0].message, "Main logger message");
    EXPECT_EQ(main_entries[1].message, "Back to main logger");
}

TEST_F(PluggableLoggerTest, FileLogger) {
    std::string log_file_path = (m_temp_dir / "test.log").string();
    
    {
        auto file_logger = std::make_unique<adapters::FileLogger>(log_file_path);
        file_logger->set_min_level(LogLevel::INFO);
        file_logger->set_auto_flush(true);
        
        EXPECT_FALSE(file_logger->is_level_enabled(LogLevel::DEBUG));
        EXPECT_TRUE(file_logger->is_level_enabled(LogLevel::INFO));
        
        file_logger->log(LogLevel::DEBUG, "Debug message"); // Should be filtered
        file_logger->log(LogLevel::INFO, "Info message");
        file_logger->log(LogLevel::ERROR, "Error message");
        file_logger->flush();
    } // Logger destructor should close file
    
    // Verify file contents
    EXPECT_TRUE(std::filesystem::exists(log_file_path));
    
    std::ifstream file(log_file_path);
    std::string file_content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();
    
    // Should contain INFO and ERROR messages but not DEBUG
    EXPECT_NE(file_content.find("Info message"), std::string::npos);
    EXPECT_NE(file_content.find("Error message"), std::string::npos);
    EXPECT_EQ(file_content.find("Debug message"), std::string::npos);
    
    // Should contain level indicators
    EXPECT_NE(file_content.find("[INFO]"), std::string::npos);
    EXPECT_NE(file_content.find("[ERROR]"), std::string::npos);
}

TEST_F(PluggableLoggerTest, MultiLogger) {
    auto multi_logger = std::make_unique<adapters::MultiLogger>();
    
    auto test_logger1 = std::make_unique<TestLogger>();
    auto test_logger2 = std::make_unique<TestLogger>();
    auto* logger1_ptr = test_logger1.get();
    auto* logger2_ptr = test_logger2.get();
    
    // Configure different levels for each logger
    test_logger1->set_min_level(LogLevel::INFO);
    test_logger2->set_min_level(LogLevel::ERROR);
    
    multi_logger->add_logger(std::move(test_logger1));
    multi_logger->add_logger(std::move(test_logger2));
    
    EXPECT_EQ(multi_logger->logger_count(), 2);
    
    // Level should be enabled if any sub-logger has it enabled
    EXPECT_FALSE(multi_logger->is_level_enabled(LogLevel::DEBUG)); // Neither enabled
    EXPECT_TRUE(multi_logger->is_level_enabled(LogLevel::INFO));   // Logger1 enabled
    EXPECT_TRUE(multi_logger->is_level_enabled(LogLevel::ERROR));  // Both enabled
    
    // Test logging
    multi_logger->log(LogLevel::DEBUG, "Debug message");  // Neither should receive
    multi_logger->log(LogLevel::INFO, "Info message");    // Only logger1 should receive
    multi_logger->log(LogLevel::ERROR, "Error message");  // Both should receive
    multi_logger->flush();
    
    auto entries1 = logger1_ptr->get_entries();
    auto entries2 = logger2_ptr->get_entries();
    
    EXPECT_EQ(entries1.size(), 2); // INFO and ERROR
    EXPECT_EQ(entries2.size(), 1); // Only ERROR
    
    EXPECT_EQ(entries1[0].message, "Info message");
    EXPECT_EQ(entries1[1].message, "Error message");
    EXPECT_EQ(entries2[0].message, "Error message");
    
    // Test clear
    multi_logger->clear_loggers();
    EXPECT_EQ(multi_logger->logger_count(), 0);
}

TEST_F(PluggableLoggerTest, AsyncLogger) {
    auto sync_logger = std::make_unique<TestLogger>();
    auto* sync_logger_ptr = sync_logger.get();
    
    auto async_logger = std::make_unique<adapters::AsyncLogger>(std::move(sync_logger), 100);
    
    // Test level check (should delegate to underlying logger)
    EXPECT_TRUE(async_logger->is_level_enabled(LogLevel::INFO));
    
    // Log several messages
    for (int i = 0; i < 10; ++i) {
        async_logger->log(LogLevel::INFO, "Message " + std::to_string(i));
    }
    
    // Flush should wait for all messages to be processed
    async_logger->flush();
    
    auto entries = sync_logger_ptr->get_entries();
    EXPECT_EQ(entries.size(), 10);
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(entries[i].message, "Message " + std::to_string(i));
    }
}

TEST_F(PluggableLoggerTest, LogLevelConversion) {
    EXPECT_EQ(log_level_to_string(LogLevel::DEBUG), "DEBUG");
    EXPECT_EQ(log_level_to_string(LogLevel::INFO), "INFO");
    EXPECT_EQ(log_level_to_string(LogLevel::NORMAL), "NORMAL");
    EXPECT_EQ(log_level_to_string(LogLevel::ERROR), "ERROR");
    EXPECT_EQ(log_level_to_string(LogLevel::CRITICAL), "CRITICAL");
}

TEST_F(PluggableLoggerTest, ErrorHandling) {
    // Test null logger rejection
    EXPECT_THROW(LoggerManager::initialize(nullptr), std::invalid_argument);
    
    // Test null callback rejection
    EXPECT_THROW(LoggerManager::initialize_with_callback(nullptr), std::invalid_argument);
    
    // Test CallbackLogger null callback rejection
    EXPECT_THROW(CallbackLogger(nullptr), std::invalid_argument);
    
    // Test FileLogger with invalid path
    EXPECT_THROW(adapters::FileLogger("/invalid/path/file.log"), std::runtime_error);
    
    // Test MultiLogger null logger rejection
    auto multi_logger = std::make_unique<adapters::MultiLogger>();
    EXPECT_THROW(multi_logger->add_logger(nullptr), std::invalid_argument);
    
    // Test AsyncLogger null logger rejection
    EXPECT_THROW(adapters::AsyncLogger(nullptr), std::invalid_argument);
}

TEST_F(PluggableLoggerTest, MacroIntegration) {
    auto test_logger = std::make_unique<TestLogger>();
    auto* logger_ptr = test_logger.get();
    
    LoggerManager::initialize(std::move(test_logger));
    
    // Test convenience macros
    CQL_LOG_DEBUG("Debug macro test");
    CQL_LOG_INFO("Info macro test");
    CQL_LOG_NORMAL("Normal macro test");
    CQL_LOG_ERROR("Error macro test");
    CQL_LOG_CRITICAL("Critical macro test");
    
    auto entries = logger_ptr->get_entries();
    EXPECT_EQ(entries.size(), 5);
    
    // Test conditional macros
    logger_ptr->clear_entries();
    logger_ptr->set_min_level(LogLevel::ERROR);
    
    CQL_LOG_DEBUG_IF(true, "Debug conditional");  // Should be filtered
    CQL_LOG_ERROR_IF(true, "Error conditional");  // Should be logged
    CQL_LOG_INFO_IF(false, "Info conditional");   // Should not be logged due to condition
    
    entries = logger_ptr->get_entries();
    EXPECT_EQ(entries.size(), 1);
    EXPECT_EQ(entries[0].message, "Error conditional");
}

} // namespace cql::test
