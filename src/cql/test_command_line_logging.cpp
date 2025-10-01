// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "../../include/cql/command_line_handler.hpp"
#include "../../include/cql/logger_interface.hpp"
#include "../../include/cql/logger_manager.hpp"
#include "../../include/cql/logger_adapters.hpp"
#include "../../include/cql/application_controller.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>

namespace cql::test {

/**
 * @class CommandLineLoggingTest
 * @brief Test suite for command-line logging configuration
 *
 * Tests the new logging flags:
 * - --log-console: Enable console logging
 * - --log-file PATH: Specify custom log file path
 * - find_and_remove_flag() method for boolean flags
 */
class CommandLineLoggingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        m_temp_dir = std::filesystem::temp_directory_path() / "cql_logging_test";
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

        // Clean up argv memory
        cleanup_argv();
    }

    // Helper to create argv array from vector of strings
    void setup_argv(const std::vector<std::string>& args) {
        cleanup_argv();

        m_argc = static_cast<int>(args.size());
        m_argv = new char*[m_argc];

        for (int i = 0; i < m_argc; ++i) {
            m_argv[i] = new char[args[i].length() + 1];
            std::strcpy(m_argv[i], args[i].c_str());
        }
    }

    void cleanup_argv() {
        if (m_argv) {
            for (int i = 0; i < m_argc; ++i) {
                delete[] m_argv[i];
            }
            delete[] m_argv;
            m_argv = nullptr;
            m_argc = 0;
        }
    }

    std::filesystem::path m_temp_dir;
    int m_argc{0};
    char** m_argv{nullptr};
};

// ============================================================================
// find_and_remove_flag() Tests
// ============================================================================

TEST_F(CommandLineLoggingTest, FindAndRemoveFlag_FlagPresent) {
    setup_argv({"cql", "--log-console", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    // Flag should be found and removed
    EXPECT_TRUE(handler.find_and_remove_flag("--log-console"));

    // After removal, argc should be decreased
    EXPECT_EQ(handler.get_argc(), 2);  // cql, input.cql

    // Flag should not be found again
    EXPECT_FALSE(handler.find_and_remove_flag("--log-console"));
}

TEST_F(CommandLineLoggingTest, FindAndRemoveFlag_FlagAbsent) {
    setup_argv({"cql", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    // Flag should not be found
    EXPECT_FALSE(handler.find_and_remove_flag("--log-console"));

    // argc should remain unchanged
    EXPECT_EQ(handler.get_argc(), 2);
}

TEST_F(CommandLineLoggingTest, FindAndRemoveFlag_MultipleFlagsInOrder) {
    setup_argv({"cql", "--log-console", "--debug-level", "DEBUG", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    // Remove first flag
    EXPECT_TRUE(handler.find_and_remove_flag("--log-console"));
    EXPECT_EQ(handler.get_argc(), 4);  // cql, --debug-level, DEBUG, input.cql

    // Verify other arguments remain
    EXPECT_TRUE(handler.has_option("--debug-level"));
}

TEST_F(CommandLineLoggingTest, FindAndRemoveFlag_FlagAtEnd) {
    setup_argv({"cql", "input.cql", "--log-console"});

    CommandLineHandler handler(m_argc, m_argv);

    EXPECT_TRUE(handler.find_and_remove_flag("--log-console"));
    EXPECT_EQ(handler.get_argc(), 2);  // cql, input.cql
}

TEST_F(CommandLineLoggingTest, FindAndRemoveFlag_PreservesOtherArgs) {
    setup_argv({"cql", "--log-console", "--log-file", "test.log", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    EXPECT_TRUE(handler.find_and_remove_flag("--log-console"));

    // Other options should be preserved
    auto log_file_value = handler.get_option_value("--log-file");
    ASSERT_TRUE(log_file_value.has_value());
    EXPECT_EQ(log_file_value.value(), "test.log");

    // Positional argument should be preserved
    auto positional = handler.get_positional_args();
    ASSERT_EQ(positional.size(), 1);
    EXPECT_EQ(positional[0], "input.cql");
}

// ============================================================================
// --log-console Flag Tests
// ============================================================================

TEST_F(CommandLineLoggingTest, LogConsoleFlag_Default) {
    setup_argv({"cql"});

    CommandLineHandler handler(m_argc, m_argv);

    // By default, --log-console should not be present
    EXPECT_FALSE(handler.has_option("--log-console"));
}

TEST_F(CommandLineLoggingTest, LogConsoleFlag_Present) {
    setup_argv({"cql", "--log-console"});

    CommandLineHandler handler(m_argc, m_argv);

    EXPECT_TRUE(handler.has_option("--log-console"));

    // Can be removed
    EXPECT_TRUE(handler.find_and_remove_flag("--log-console"));
    EXPECT_FALSE(handler.has_option("--log-console"));
}

TEST_F(CommandLineLoggingTest, LogConsoleFlag_WithOtherFlags) {
    setup_argv({"cql", "--log-console", "--debug-level", "INFO"});

    CommandLineHandler handler(m_argc, m_argv);

    EXPECT_TRUE(handler.has_option("--log-console"));
    EXPECT_TRUE(handler.has_option("--debug-level"));
}

// ============================================================================
// --log-file Option Tests
// ============================================================================

TEST_F(CommandLineLoggingTest, LogFileOption_DefaultValue) {
    setup_argv({"cql"});

    CommandLineHandler handler(m_argc, m_argv);

    // Should not have --log-file by default
    EXPECT_FALSE(handler.has_option("--log-file"));

    // Simulate application default
    std::string log_file = "cql.log";
    bool found = handler.find_and_remove_option("--log-file", log_file);
    EXPECT_FALSE(found);
    EXPECT_EQ(log_file, "cql.log");  // Default preserved
}

TEST_F(CommandLineLoggingTest, LogFileOption_CustomPath) {
    setup_argv({"cql", "--log-file", "custom.log"});

    CommandLineHandler handler(m_argc, m_argv);

    auto value = handler.get_option_value("--log-file");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "custom.log");
}

TEST_F(CommandLineLoggingTest, LogFileOption_FindAndRemove) {
    setup_argv({"cql", "--log-file", "test.log", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    std::string log_file;
    EXPECT_TRUE(handler.find_and_remove_option("--log-file", log_file));
    EXPECT_EQ(log_file, "test.log");

    // After removal, should not be found
    EXPECT_FALSE(handler.has_option("--log-file"));

    // Positional arg should remain
    auto positional = handler.get_positional_args();
    ASSERT_EQ(positional.size(), 1);
    EXPECT_EQ(positional[0], "input.cql");
}

TEST_F(CommandLineLoggingTest, LogFileOption_AbsolutePath) {
    auto abs_path = m_temp_dir / "test.log";
    setup_argv({"cql", "--log-file", abs_path.string()});

    CommandLineHandler handler(m_argc, m_argv);

    auto value = handler.get_option_value("--log-file");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), abs_path.string());
}

TEST_F(CommandLineLoggingTest, LogFileOption_RelativePath) {
    setup_argv({"cql", "--log-file", "./logs/app.log"});

    CommandLineHandler handler(m_argc, m_argv);

    auto value = handler.get_option_value("--log-file");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "./logs/app.log");
}

// ============================================================================
// MultiLogger Configuration Tests
// ============================================================================

TEST_F(CommandLineLoggingTest, MultiLogger_FileAndConsole) {
    auto log_file = m_temp_dir / "multi.log";

    // Create multi-logger
    auto multi_logger = std::make_unique<cql::adapters::MultiLogger>();

    // Add file logger
    auto file_logger = std::make_unique<cql::adapters::FileLogger>(log_file.string());
    file_logger->set_min_level(LogLevel::DEBUG);
    multi_logger->add_logger(std::move(file_logger));

    // Add console logger
    auto console_logger = std::make_unique<cql::DefaultConsoleLogger>();
    console_logger->set_min_level(LogLevel::DEBUG);
    multi_logger->add_logger(std::move(console_logger));

    // Initialize
    LoggerManager::initialize(std::move(multi_logger));

    // Write a log message
    auto& logger = Logger::getInstance();
    logger.log(LogLevel::INFO, "MultiLogger test message");
    LoggerManager::flush();

    // Verify file was created and contains message
    ASSERT_TRUE(std::filesystem::exists(log_file));

    std::ifstream file(log_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("MultiLogger test message") != std::string::npos);
}

TEST_F(CommandLineLoggingTest, MultiLogger_FileOnly) {
    auto log_file = m_temp_dir / "file_only.log";

    // Create file-only logger
    auto file_logger = std::make_unique<cql::adapters::FileLogger>(log_file.string());
    file_logger->set_min_level(LogLevel::DEBUG);

    LoggerManager::initialize(std::move(file_logger));

    // Write a log message
    auto& logger = Logger::getInstance();
    logger.log(LogLevel::INFO, "File only test message");
    LoggerManager::flush();

    // Verify file was created and contains message
    ASSERT_TRUE(std::filesystem::exists(log_file));

    std::ifstream file(log_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("File only test message") != std::string::npos);
}

TEST_F(CommandLineLoggingTest, MultiLogger_DifferentLogLevels) {
    auto log_file = m_temp_dir / "levels.log";

    auto multi_logger = std::make_unique<cql::adapters::MultiLogger>();

    // File logger accepts DEBUG and above
    auto file_logger = std::make_unique<cql::adapters::FileLogger>(log_file.string());
    file_logger->set_min_level(LogLevel::DEBUG);
    multi_logger->add_logger(std::move(file_logger));

    // Console logger accepts ERROR and above only
    auto console_logger = std::make_unique<cql::DefaultConsoleLogger>();
    console_logger->set_min_level(LogLevel::ERROR);
    multi_logger->add_logger(std::move(console_logger));

    LoggerManager::initialize(std::move(multi_logger));

    auto& logger = Logger::getInstance();

    // Log at different levels
    logger.log(LogLevel::DEBUG, "Debug message");
    logger.log(LogLevel::INFO, "Info message");
    logger.log(LogLevel::ERROR, "Error message");
    LoggerManager::flush();

    // Verify file contains all messages
    std::ifstream file(log_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("Debug message") != std::string::npos);
    EXPECT_TRUE(content.find("Info message") != std::string::npos);
    EXPECT_TRUE(content.find("Error message") != std::string::npos);
}

TEST_F(CommandLineLoggingTest, MultiLogger_EmptyLoggerList) {
    // Create multi-logger without adding any loggers
    auto multi_logger = std::make_unique<cql::adapters::MultiLogger>();

    // Should not crash
    EXPECT_NO_THROW(LoggerManager::initialize(std::move(multi_logger)));

    auto& logger = Logger::getInstance();
    EXPECT_NO_THROW(logger.log(LogLevel::INFO, "Test"));
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(CommandLineLoggingTest, Integration_DefaultBehavior) {
    // Default behavior: file logging only
    setup_argv({"cql", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    // Should not have console flag
    EXPECT_FALSE(handler.find_and_remove_flag("--log-console"));

    // Default log file should be used
    std::string log_file = "cql.log";
    EXPECT_FALSE(handler.find_and_remove_option("--log-file", log_file));
    EXPECT_EQ(log_file, "cql.log");
}

TEST_F(CommandLineLoggingTest, Integration_ConsoleLoggingEnabled) {
    setup_argv({"cql", "--log-console", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    // Console flag should be found
    bool log_to_console = handler.find_and_remove_flag("--log-console");
    EXPECT_TRUE(log_to_console);

    // Default log file
    std::string log_file = "cql.log";
    handler.find_and_remove_option("--log-file", log_file);

    // Verify we would configure multi-logger
    EXPECT_TRUE(log_to_console);
    EXPECT_EQ(log_file, "cql.log");
}

TEST_F(CommandLineLoggingTest, Integration_CustomLogFile) {
    setup_argv({"cql", "--log-file", "custom.log", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    bool log_to_console = handler.find_and_remove_flag("--log-console");
    EXPECT_FALSE(log_to_console);

    std::string log_file = "cql.log";
    EXPECT_TRUE(handler.find_and_remove_option("--log-file", log_file));
    EXPECT_EQ(log_file, "custom.log");
}

TEST_F(CommandLineLoggingTest, Integration_BothConsoleAndCustomFile) {
    setup_argv({"cql", "--log-console", "--log-file", "my.log", "input.cql"});

    CommandLineHandler handler(m_argc, m_argv);

    bool log_to_console = handler.find_and_remove_flag("--log-console");
    EXPECT_TRUE(log_to_console);

    std::string log_file = "cql.log";
    EXPECT_TRUE(handler.find_and_remove_option("--log-file", log_file));
    EXPECT_EQ(log_file, "my.log");

    // Verify multi-logger would be configured with custom file
    EXPECT_TRUE(log_to_console);
    EXPECT_EQ(log_file, "my.log");
}

TEST_F(CommandLineLoggingTest, Integration_WithDebugLevel) {
    setup_argv({"cql", "--log-console", "--debug-level", "DEBUG", "--log-file", "debug.log"});

    CommandLineHandler handler(m_argc, m_argv);

    bool log_to_console = handler.find_and_remove_flag("--log-console");
    EXPECT_TRUE(log_to_console);

    std::string log_file = "cql.log";
    EXPECT_TRUE(handler.find_and_remove_option("--log-file", log_file));
    EXPECT_EQ(log_file, "debug.log");

    std::string debug_level;
    EXPECT_TRUE(handler.find_and_remove_option("--debug-level", debug_level));
    EXPECT_EQ(debug_level, "DEBUG");
}

} // namespace cql::test
