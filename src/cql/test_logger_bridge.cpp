// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "../../include/cql/project_utils.hpp" // This includes the bridged Logger
#include "../../include/cql/logger_manager.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <atomic>

namespace cql::test {

class LoggerBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        m_temp_dir = std::filesystem::temp_directory_path() / "cql_bridge_test";
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

TEST_F(LoggerBridgeTest, HistoricAPICompatibility) {
    // Test that all historic Logger API methods work unchanged
    
    // Test getInstance() and basic logging
    auto& logger = Logger::getInstance();
    EXPECT_NO_THROW(logger.log(LogLevel::INFO, "Test message"));
    
    // Test level management
    EXPECT_NO_THROW(logger.setLevelEnabled(LogLevel::DEBUG, false));
    EXPECT_FALSE(logger.isLevelEnabled(LogLevel::DEBUG));
    EXPECT_NO_THROW(logger.setLevelEnabled(LogLevel::DEBUG, true));
    EXPECT_TRUE(logger.isLevelEnabled(LogLevel::DEBUG));
    
    // Test setToLevelEnabled
    EXPECT_NO_THROW(logger.setToLevelEnabled(LogLevel::ERROR));
    EXPECT_FALSE(logger.isLevelEnabled(LogLevel::DEBUG));
    EXPECT_FALSE(logger.isLevelEnabled(LogLevel::INFO));
    EXPECT_TRUE(logger.isLevelEnabled(LogLevel::ERROR));
    
    // Test stderr control
    EXPECT_TRUE(logger.isStderrEnabled());
    EXPECT_NO_THROW(logger.disableStderr());
    EXPECT_FALSE(logger.isStderrEnabled());
    EXPECT_NO_THROW(logger.enableStderr());
    EXPECT_TRUE(logger.isStderrEnabled());
    
    // Test file output control
    EXPECT_TRUE(logger.isFileOutputEnabled());
    EXPECT_NO_THROW(logger.setFileOutputEnabled(false));
    EXPECT_NO_THROW(logger.setFileOutputEnabled(true));
}

TEST_F(LoggerBridgeTest, VariadicTemplateLogging) {
    // Test that variadic template logging works exactly like historic Logger
    auto& logger = Logger::getInstance();
    
    // Test multiple argument logging
    EXPECT_NO_THROW(logger.log(LogLevel::INFO, "Test with ", 42, " arguments"));
    EXPECT_NO_THROW(logger.log(LogLevel::DEBUG, "String: ", "test", " Number: ", 3.14));
    
    // Test depth logging
    EXPECT_NO_THROW(logger.log_with_depth(LogLevel::INFO, 2, "Indented message"));
    EXPECT_NO_THROW(logger.log_with_depth(LogLevel::ERROR, 0, "No indent"));
}

TEST_F(LoggerBridgeTest, StderrSuppressionGuard) {
    // Test that the StderrSuppressionGuard works
    auto& logger = Logger::getInstance();
    EXPECT_TRUE(logger.isStderrEnabled());
    
    {
        Logger::StderrSuppressionGuard guard;
        EXPECT_FALSE(logger.isStderrEnabled());
    }
    
    EXPECT_TRUE(logger.isStderrEnabled());
}

TEST_F(LoggerBridgeTest, FileLogging) {
    std::string log_file_path = (m_temp_dir / "bridge_test.log").string();
    
    // Create logger with specific file path
    auto& logger = Logger::getInstance(log_file_path);
    
    // Log some messages
    logger.log(LogLevel::INFO, "Bridge test message");
    logger.log(LogLevel::ERROR, "Error message");
    
    // Verify LoggerManager was initialized
    EXPECT_TRUE(LoggerManager::is_initialized());
}

TEST_F(LoggerBridgeTest, MultipleInstances) {
    // Test that getInstance() returns the same instance (singleton pattern)
    auto& logger1 = Logger::getInstance();
    auto& logger2 = Logger::getInstance();
    
    EXPECT_EQ(&logger1, &logger2);
    
    // Test shared_ptr versions
    auto ptr1 = Logger::getInstancePtr();
    auto ptr2 = Logger::getInstancePtr();
    
    EXPECT_EQ(ptr1.get(), ptr2.get());
}

TEST_F(LoggerBridgeTest, NamespaceCompatibility) {
    // Test that both LogLevel types work (historic enum and new cql::LogLevel)
    auto& logger = Logger::getInstance();
    
    // Historic LogLevel enum (aliased as global LogLevel)
    EXPECT_NO_THROW(logger.log(LogLevel::INFO, "Historic enum test"));
    EXPECT_NO_THROW(logger.setLevelEnabled(LogLevel::DEBUG, true));
    EXPECT_NO_THROW(logger.setToLevelEnabled(LogLevel::ERROR));
    
    // New cql::LogLevel enum should also work through overloads
    EXPECT_NO_THROW(logger.log(cql::LogLevel::INFO, "New enum test"));
    EXPECT_NO_THROW(logger.setLevelEnabled(cql::LogLevel::DEBUG, true));
    EXPECT_NO_THROW(logger.setToLevelEnabled(cql::LogLevel::ERROR));
}

TEST_F(LoggerBridgeTest, UnderlyingPluggableSystem) {
    // Test that the bridge is actually using the pluggable system underneath
    auto& logger = Logger::getInstance();
    
    // Log something to initialize the system
    logger.log(LogLevel::INFO, "Initialization message");
    
    // Verify that LoggerManager was initialized by the bridge
    EXPECT_TRUE(LoggerManager::is_initialized());
    
    // Test that we can still access the pluggable system directly
    EXPECT_NO_THROW(LoggerManager::log(cql::LogLevel::INFO, "Direct pluggable system call"));
    EXPECT_TRUE(LoggerManager::is_level_enabled(cql::LogLevel::INFO));
}

TEST_F(LoggerBridgeTest, ThreadSafety) {
    // Test that the bridge system is thread-safe
    auto& logger = Logger::getInstance();
    
    const int num_threads = 10;
    const int messages_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> completed_threads{0};
    
    // Launch multiple threads that log concurrently
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                // Test various log levels and operations concurrently
                logger.log(LogLevel::INFO, "Thread ", t, " message ", i);
                logger.log(LogLevel::DEBUG, "Debug from thread ", t, " msg ", i);
                logger.log_with_depth(LogLevel::ERROR, 1, "Depth msg ", t, ":", i);
                
                // Test level management operations
                if (i % 10 == 0) {
                    logger.setLevelEnabled(LogLevel::DEBUG, i % 20 == 0);
                    bool enabled = logger.isLevelEnabled(LogLevel::DEBUG);
                    (void)enabled; // Suppress unused variable warning
                }
                
                // Test stderr control
                if (i % 20 == 0) {
                    logger.disableStderr();
                    logger.enableStderr();
                }
            }
            completed_threads++;
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all threads completed
    EXPECT_EQ(completed_threads.load(), num_threads);
    
    // Verify the logger system is still functional
    EXPECT_NO_THROW(logger.log(LogLevel::INFO, "Post-threading test"));
    EXPECT_TRUE(LoggerManager::is_initialized());
}

TEST_F(LoggerBridgeTest, ConcurrentSingletonAccess) {
    // Test that getInstance() is thread-safe and returns the same instance
    const int num_threads = 20;
    std::vector<std::thread> threads;
    std::vector<Logger*> logger_instances(num_threads);
    std::vector<std::shared_ptr<Logger>> logger_ptrs(num_threads);
    
    // Launch threads that get the singleton instance concurrently
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            // Test both getInstance methods concurrently
            logger_instances[t] = &Logger::getInstance();
            logger_ptrs[t] = Logger::getInstancePtr();
            
            // Log something to test functionality
            logger_instances[t]->log(LogLevel::INFO, "Concurrent init test ", t);
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all instances are the same (singleton pattern)
    Logger* first_instance = logger_instances[0];
    for (int t = 1; t < num_threads; ++t) {
        EXPECT_EQ(logger_instances[t], first_instance) << "Thread " << t << " got different instance";
        EXPECT_EQ(logger_ptrs[t].get(), first_instance) << "Thread " << t << " shared_ptr differs";
    }
}

TEST_F(LoggerBridgeTest, ConcurrentLevelManagement) {
    // Test concurrent level enable/disable operations
    auto& logger = Logger::getInstance();
    
    const int num_threads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> operations_completed{0};
    
    // Launch threads that modify log levels concurrently
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 50; ++i) {
                // Each thread toggles different levels
                LogLevel level = static_cast<LogLevel>(t % 5);
                
                logger.setLevelEnabled(level, i % 2 == 0);
                bool enabled = logger.isLevelEnabled(level);
                
                // Log using the level we just modified
                if (enabled) {
                    logger.log(level, "Level test ", t, ":", i);
                }
                
                operations_completed++;
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify expected number of operations completed
    EXPECT_EQ(operations_completed.load(), num_threads * 50);
    
    // Test that the logger is still functional
    EXPECT_NO_THROW(logger.log(LogLevel::INFO, "Final level management test"));
}

TEST_F(LoggerBridgeTest, ConcurrentStderrControl) {
    // Test concurrent stderr enable/disable operations
    auto& logger = Logger::getInstance();
    
    const int num_threads = 6;
    std::vector<std::thread> threads;
    
    // Launch threads that toggle stderr concurrently
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 30; ++i) {
                // Test StderrSuppressionGuard in concurrent scenarios
                if (i % 3 == 0) {
                    Logger::StderrSuppressionGuard guard;
                    logger.log(LogLevel::ERROR, "Suppressed error ", t, ":", i);
                } else {
                    // Manual control
                    logger.disableStderr();
                    logger.log(LogLevel::ERROR, "Manual disable ", t, ":", i);
                    logger.enableStderr();
                    logger.log(LogLevel::ERROR, "Manual enable ", t, ":", i);
                }
                
                // Verify state is accessible (even if it may change due to concurrency)
                bool stderr_enabled = logger.isStderrEnabled();
                (void)stderr_enabled; // Suppress unused variable warning
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify system is still functional and stderr is enabled by default
    EXPECT_TRUE(logger.isStderrEnabled());
    EXPECT_NO_THROW(logger.log(LogLevel::ERROR, "Final stderr test"));
}

} // namespace cql::test
