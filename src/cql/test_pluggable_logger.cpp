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
            std::error_code ec;
            std::filesystem::remove_all(m_temp_dir, ec);
            // Ignore errors - directory may have issues with locked files
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

TEST_F(PluggableLoggerTest, FileLoggerRotation) {
    auto log_file = m_temp_dir / "rotate_test.log";

    // Create file logger with rotation enabled (100 bytes max, keep 3 files)
    auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
    file_logger->enable_rotation(100, 3);
    file_logger->set_min_level(LogLevel::DEBUG);

    EXPECT_TRUE(file_logger->is_rotation_enabled());

    // Write enough data to trigger rotation
    for (int i = 0; i < 20; ++i) {
        file_logger->log(LogLevel::INFO, "This is a test message number " + std::to_string(i));
    }
    file_logger->flush();

    // Check that rotation occurred - should have main file and rotated files
    EXPECT_TRUE(std::filesystem::exists(log_file));

    // Current file should be smaller than max size after rotation
    auto current_size = std::filesystem::file_size(log_file);
    EXPECT_LT(current_size, 100);

    // Verify rotated files exist
    auto rotated_1 = log_file.string() + ".1";
    EXPECT_TRUE(std::filesystem::exists(rotated_1));
}

TEST_F(PluggableLoggerTest, FileLoggerRotationDisabled) {
    auto log_file = m_temp_dir / "no_rotate_test.log";

    {
        auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
        EXPECT_FALSE(file_logger->is_rotation_enabled());

        // Write data
        for (int i = 0; i < 10; ++i) {
            file_logger->log(LogLevel::INFO, "Message");
        }
        // File logger goes out of scope and closes file
    }

    // Should not have rotated files
    EXPECT_TRUE(std::filesystem::exists(log_file));
    EXPECT_FALSE(std::filesystem::exists(log_file.string() + ".1"));
}

TEST_F(PluggableLoggerTest, FileLoggerMultipleRotations) {
    auto log_file = m_temp_dir / "multi_rotate.log";

    // Create file logger with rotation enabled (50 bytes max, keep 3 files)
    auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
    file_logger->enable_rotation(50, 3);
    file_logger->set_min_level(LogLevel::DEBUG);

    // Write enough data to trigger multiple rotations
    for (int i = 0; i < 50; ++i) {
        file_logger->log(LogLevel::INFO, "Message " + std::to_string(i));
    }
    file_logger->flush();

    // Check that multiple rotations occurred
    EXPECT_TRUE(std::filesystem::exists(log_file));
    EXPECT_TRUE(std::filesystem::exists(log_file.string() + ".1"));
    EXPECT_TRUE(std::filesystem::exists(log_file.string() + ".2"));
    EXPECT_TRUE(std::filesystem::exists(log_file.string() + ".3"));

    // Should not have more than max_files rotations
    EXPECT_FALSE(std::filesystem::exists(log_file.string() + ".4"));
}

TEST_F(PluggableLoggerTest, FileLoggerMaxFilesEnforcement) {
    auto log_file = m_temp_dir / "max_files.log";

    // Create file logger with rotation enabled (30 bytes max, keep only 2 files)
    auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
    file_logger->enable_rotation(30, 2);
    file_logger->set_min_level(LogLevel::DEBUG);

    // Write enough data to trigger many rotations
    for (int i = 0; i < 100; ++i) {
        file_logger->log(LogLevel::INFO, "Test message");
    }
    file_logger->flush();

    // Check that we have main file + max_files rotated files
    EXPECT_TRUE(std::filesystem::exists(log_file));
    EXPECT_TRUE(std::filesystem::exists(log_file.string() + ".1"));
    EXPECT_TRUE(std::filesystem::exists(log_file.string() + ".2"));

    // Should not have more than max_files rotations
    EXPECT_FALSE(std::filesystem::exists(log_file.string() + ".3"));
    EXPECT_FALSE(std::filesystem::exists(log_file.string() + ".4"));
}

TEST_F(PluggableLoggerTest, FileLoggerConcurrentLogging) {
    auto log_file = m_temp_dir / "concurrent.log";

    const int num_threads = 4;
    const int messages_per_thread = 25;

    // Create file logger with rotation enabled (large enough to hold all messages)
    {
        auto file_logger = std::make_shared<adapters::FileLogger>(log_file.string());
        file_logger->enable_rotation(2000, 5);  // Larger size to hold more messages
        file_logger->set_min_level(LogLevel::DEBUG);

        // Launch multiple threads writing concurrently
        std::vector<std::thread> threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&file_logger, t]() {
                for (int i = 0; i < messages_per_thread; ++i) {
                    file_logger->log(LogLevel::INFO,
                        "Thread " + std::to_string(t) + " msg " + std::to_string(i));
                }
            });
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
    } // Destroy file_logger to ensure all data is flushed and file is closed

    // Verify all files exist and are valid
    EXPECT_TRUE(std::filesystem::exists(log_file));

    // Count total messages across all files
    int total_messages = 0;
    std::ifstream main_file(log_file);
    std::string line;
    while (std::getline(main_file, line)) {
        if (!line.empty()) ++total_messages;
    }

    // Check rotated files
    for (int i = 1; i <= 5; ++i) {
        auto rotated = log_file.string() + "." + std::to_string(i);
        if (std::filesystem::exists(rotated)) {
            std::ifstream file(rotated);
            while (std::getline(file, line)) {
                if (!line.empty()) ++total_messages;
            }
        }
    }

    // Should have all messages (num_threads * messages_per_thread)
    EXPECT_EQ(total_messages, num_threads * messages_per_thread);
}

TEST_F(PluggableLoggerTest, FileLoggerTimestampFormats) {
    // Test ISO8601 format
    {
        auto log_file = m_temp_dir / "timestamp_iso8601.log";
        {
            auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
            file_logger->set_min_level(LogLevel::DEBUG);  // Enable all log levels
            file_logger->set_timestamp_format(adapters::TimestampFormat::ISO8601);
            file_logger->log(LogLevel::INFO, "Test message");
        } // Close file

        ASSERT_TRUE(std::filesystem::exists(log_file));
        ASSERT_GT(std::filesystem::file_size(log_file), 0u);

        std::ifstream file(log_file);
        ASSERT_TRUE(file.is_open());
        std::string line;
        ASSERT_TRUE(std::getline(file, line)) << "Failed to read from " << log_file;
        ASSERT_FALSE(line.empty()) << "Line is empty in " << log_file;
        // ISO8601 format should contain 'T' and 'Z'
        EXPECT_NE(line.find('T'), std::string::npos) << "Line: " << line;
        EXPECT_NE(line.find('Z'), std::string::npos) << "Line: " << line;
    }

    // Test EPOCH_MS format
    {
        auto log_file = m_temp_dir / "timestamp_epoch.log";
        {
            auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
            file_logger->set_min_level(LogLevel::DEBUG);  // Enable all log levels
            file_logger->set_timestamp_format(adapters::TimestampFormat::EPOCH_MS);
            file_logger->log(LogLevel::INFO, "Test message");
        } // Close file

        ASSERT_TRUE(std::filesystem::exists(log_file));
        ASSERT_GT(std::filesystem::file_size(log_file), 0u);

        std::ifstream file(log_file);
        ASSERT_TRUE(file.is_open());
        std::string line;
        ASSERT_TRUE(std::getline(file, line)) << "Failed to read from " << log_file;
        ASSERT_FALSE(line.empty()) << "Line is empty in " << log_file;
        // Epoch format should start with digits (milliseconds since epoch)
        EXPECT_TRUE(std::isdigit(line[0])) << "Line: " << line;
    }

    // Test NONE format
    {
        auto log_file = m_temp_dir / "timestamp_none.log";
        {
            auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
            file_logger->set_min_level(LogLevel::DEBUG);  // Enable all log levels
            file_logger->set_timestamp_format(adapters::TimestampFormat::NONE);
            file_logger->log(LogLevel::INFO, "Test message");
        } // Close file

        ASSERT_TRUE(std::filesystem::exists(log_file));
        ASSERT_GT(std::filesystem::file_size(log_file), 0u);

        std::ifstream file(log_file);
        ASSERT_TRUE(file.is_open());
        std::string line;
        ASSERT_TRUE(std::getline(file, line)) << "Failed to read from " << log_file;
        ASSERT_FALSE(line.empty()) << "Line is empty in " << log_file;
        // Should start with log level, not timestamp
        EXPECT_EQ(line.find('['), 0u) << "Line: " << line;
    }
}

TEST_F(PluggableLoggerTest, FileLoggerRotationThresholdAccuracy) {
    auto log_file = m_temp_dir / "threshold_test.log";

    // Create file logger with precise threshold (100 bytes)
    auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
    file_logger->enable_rotation(100, 3);
    file_logger->set_min_level(LogLevel::DEBUG);

    // Write messages until rotation happens
    std::string test_message = "Test";

    // Write until we trigger rotation
    for (int i = 0; i < 50; ++i) {
        file_logger->log(LogLevel::INFO, test_message);

        // Check if rotation occurred
        if (std::filesystem::exists(log_file.string() + ".1")) {
            break;
        }
    }

    file_logger.reset(); // Close file

    // Verify rotation happened
    EXPECT_TRUE(std::filesystem::exists(log_file.string() + ".1"));

    // Main file should be smaller than threshold after rotation
    auto main_size = std::filesystem::file_size(log_file);
    EXPECT_LT(main_size, 100u);

    // Rotated file should exist and not be empty
    auto rotated_size = std::filesystem::file_size(log_file.string() + ".1");
    EXPECT_GT(rotated_size, 0u);  // File should have content
    // Note: Actual size depends on timestamp/thread formatting, so just verify it's reasonable
    EXPECT_LE(rotated_size, 150u); // Should be close to threshold
}

TEST_F(PluggableLoggerTest, FileLoggerAppendModeWithRotation) {
    auto log_file = m_temp_dir / "append_rotate.log";

    // Phase 1: Create initial file with some content
    {
        auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string(), false);
        file_logger->set_min_level(LogLevel::DEBUG);
        file_logger->log(LogLevel::INFO, "Initial message 1");
        file_logger->log(LogLevel::INFO, "Initial message 2");
    }

    auto initial_size = std::filesystem::file_size(log_file);
    EXPECT_GT(initial_size, 0u);

    // Phase 2: Open in append mode with rotation enabled
    {
        auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string(), true);
        file_logger->enable_rotation(200, 3);
        file_logger->set_min_level(LogLevel::DEBUG);

        // Verify initial size was tracked
        EXPECT_GT(file_logger->get_current_file_size(), 0u);

        // Write more messages
        for (int i = 0; i < 20; ++i) {
            file_logger->log(LogLevel::INFO, "Appended message " + std::to_string(i));
        }
    }

    // Verify file grew from initial size
    auto final_size = std::filesystem::file_size(log_file);
    EXPECT_GT(final_size, initial_size);

    // Check if rotation occurred (depends on message sizes)
    bool rotated = std::filesystem::exists(log_file.string() + ".1");
    if (rotated) {
        // If rotation occurred, main file should be smaller than threshold
        EXPECT_LT(final_size, 200u);
    }
}

TEST_F(PluggableLoggerTest, FileLoggerUnlimitedRotation) {
    auto log_file = m_temp_dir / "unlimited_rotate.log";

    // Create file logger with unlimited rotation (max_files = 0)
    auto file_logger = std::make_unique<adapters::FileLogger>(log_file.string());
    file_logger->enable_rotation(100, 0);  // Unlimited rotation, larger threshold
    file_logger->set_min_level(LogLevel::DEBUG);
    file_logger->set_timestamp_format(adapters::TimestampFormat::NONE);  // Smaller messages

    EXPECT_TRUE(file_logger->is_rotation_enabled());

    // Write enough data to trigger multiple rotations
    for (int i = 0; i < 200; ++i) {
        file_logger->log(LogLevel::INFO, "Message " + std::to_string(i));
    }
    file_logger.reset(); // Close file

    // Count how many rotated files exist
    int rotated_count = 0;
    for (int i = 1; i <= 30; ++i) {  // Check up to 30 rotations
        if (std::filesystem::exists(log_file.string() + "." + std::to_string(i))) {
            rotated_count++;
        } else {
            break;
        }
    }

    // With unlimited rotation, we should have at least one rotated file
    EXPECT_GE(rotated_count, 1) << "Unlimited rotation should create rotated files";

    // Main file should exist
    EXPECT_TRUE(std::filesystem::exists(log_file));

    // Key test: verify max_files=0 allows unlimited rotations (no deletion of old files)
    // With limited rotation, old files would be deleted. Here they should all exist.
    EXPECT_TRUE(std::filesystem::exists(log_file.string() + ".1"));
    if (rotated_count > 1) {
        EXPECT_TRUE(std::filesystem::exists(log_file.string() + "." + std::to_string(rotated_count)));
    }
}

} // namespace cql::test
