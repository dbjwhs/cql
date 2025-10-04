// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "../../include/cql/user_output.hpp"
#include "../../include/cql/user_output_manager.hpp"
#include <sstream>
#include <fstream>
#include <filesystem>

using namespace cql;

// ============================================================================
// Test Fixture
// ============================================================================

class UserOutputTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure clean state before each test
        if (UserOutputManager::is_initialized()) {
            UserOutputManager::shutdown();
        }
    }

    void TearDown() override {
        // Clean up after each test
        if (UserOutputManager::is_initialized()) {
            UserOutputManager::shutdown();
        }
    }

    // Helper to create a temporary file path
    std::string get_temp_file_path(const std::string& name) {
        return std::filesystem::temp_directory_path() / ("cql_test_" + name);
    }

    // Helper to read file contents
    std::string read_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

// ============================================================================
// ConsoleUserOutput Tests
// ============================================================================

TEST_F(UserOutputTest, ConsoleOutput_Basic) {
    ConsoleUserOutput output;

    // Should not throw
    EXPECT_NO_THROW(output.write(MessageType::INFO, "Test message"));
    EXPECT_NO_THROW(output.write(MessageType::SUCCESS, "Success message"));
    EXPECT_NO_THROW(output.write(MessageType::WARNING, "Warning message"));
    EXPECT_NO_THROW(output.write(MessageType::ERROR, "Error message"));
    EXPECT_NO_THROW(output.write(MessageType::PROGRESS, "Progress message"));
    EXPECT_NO_THROW(output.flush());
}

TEST_F(UserOutputTest, ConsoleOutput_TypeFiltering) {
    ConsoleUserOutput output;

    // All types enabled by default
    EXPECT_TRUE(output.is_enabled(MessageType::INFO));
    EXPECT_TRUE(output.is_enabled(MessageType::SUCCESS));
    EXPECT_TRUE(output.is_enabled(MessageType::WARNING));
    EXPECT_TRUE(output.is_enabled(MessageType::ERROR));
    EXPECT_TRUE(output.is_enabled(MessageType::PROGRESS));

    // Disable specific types
    output.set_type_enabled(MessageType::INFO, false);
    EXPECT_FALSE(output.is_enabled(MessageType::INFO));
    EXPECT_TRUE(output.is_enabled(MessageType::SUCCESS));

    // Re-enable
    output.set_type_enabled(MessageType::INFO, true);
    EXPECT_TRUE(output.is_enabled(MessageType::INFO));
}

TEST_F(UserOutputTest, ConsoleOutput_ColorControl) {
    ConsoleUserOutput output;

    // Should not throw
    EXPECT_NO_THROW(output.set_colored_output(true));
    EXPECT_NO_THROW(output.write(MessageType::SUCCESS, "Colored message"));

    EXPECT_NO_THROW(output.set_colored_output(false));
    EXPECT_NO_THROW(output.write(MessageType::SUCCESS, "Plain message"));
}

// ============================================================================
// FileUserOutput Tests
// ============================================================================

TEST_F(UserOutputTest, FileOutput_Basic) {
    const std::string temp_file = get_temp_file_path("output.txt");

    {
        FileUserOutput output(temp_file, false);
        EXPECT_TRUE(output.is_open());

        output.write(MessageType::INFO, "Test message");
        output.write(MessageType::SUCCESS, "Success message");
        output.flush();
    }

    // Verify file contents
    const std::string contents = read_file(temp_file);
    EXPECT_TRUE(contents.find("[INFO] Test message") != std::string::npos);
    EXPECT_TRUE(contents.find("[SUCCESS] Success message") != std::string::npos);

    // Clean up
    std::filesystem::remove(temp_file);
}

TEST_F(UserOutputTest, FileOutput_AppendMode) {
    const std::string temp_file = get_temp_file_path("append.txt");

    // Write initial content
    {
        FileUserOutput output(temp_file, false);
        output.write(MessageType::INFO, "First message");
    }

    // Append more content
    {
        FileUserOutput output(temp_file, true);
        output.write(MessageType::INFO, "Second message");
    }

    // Verify both messages present
    const std::string contents = read_file(temp_file);
    EXPECT_TRUE(contents.find("First message") != std::string::npos);
    EXPECT_TRUE(contents.find("Second message") != std::string::npos);

    // Clean up
    std::filesystem::remove(temp_file);
}

TEST_F(UserOutputTest, FileOutput_TypeFiltering) {
    const std::string temp_file = get_temp_file_path("filtered.txt");

    {
        FileUserOutput output(temp_file, false);

        // All types enabled by default
        EXPECT_TRUE(output.is_enabled(MessageType::INFO));
        EXPECT_TRUE(output.is_enabled(MessageType::ERROR));
    }

    std::filesystem::remove(temp_file);
}

// ============================================================================
// NullUserOutput Tests
// ============================================================================

TEST_F(UserOutputTest, NullOutput_DiscardsAllMessages) {
    NullUserOutput output;

    // All types disabled
    EXPECT_FALSE(output.is_enabled(MessageType::INFO));
    EXPECT_FALSE(output.is_enabled(MessageType::SUCCESS));
    EXPECT_FALSE(output.is_enabled(MessageType::WARNING));
    EXPECT_FALSE(output.is_enabled(MessageType::ERROR));
    EXPECT_FALSE(output.is_enabled(MessageType::PROGRESS));

    // Should not throw
    EXPECT_NO_THROW(output.write(MessageType::INFO, "Discarded message"));
    EXPECT_NO_THROW(output.flush());
}

// ============================================================================
// MultiUserOutput Tests
// ============================================================================

TEST_F(UserOutputTest, MultiOutput_WritesToMultipleDestinations) {
    const std::string temp_file1 = get_temp_file_path("multi1.txt");
    const std::string temp_file2 = get_temp_file_path("multi2.txt");

    {
        MultiUserOutput multi_output;

        auto file_output1 = std::make_unique<FileUserOutput>(temp_file1, false);
        auto file_output2 = std::make_unique<FileUserOutput>(temp_file2, false);

        multi_output.add_output(std::move(file_output1));
        multi_output.add_output(std::move(file_output2));

        EXPECT_EQ(multi_output.output_count(), 2);

        multi_output.write(MessageType::INFO, "Test message");
        multi_output.flush();
    }

    // Verify both files have the message
    const std::string contents1 = read_file(temp_file1);
    const std::string contents2 = read_file(temp_file2);

    EXPECT_TRUE(contents1.find("Test message") != std::string::npos);
    EXPECT_TRUE(contents2.find("Test message") != std::string::npos);

    // Clean up
    std::filesystem::remove(temp_file1);
    std::filesystem::remove(temp_file2);
}

TEST_F(UserOutputTest, MultiOutput_EmptyList) {
    MultiUserOutput multi_output;

    EXPECT_EQ(multi_output.output_count(), 0);

    // All types disabled when no outputs
    EXPECT_FALSE(multi_output.is_enabled(MessageType::INFO));

    // Should not throw
    EXPECT_NO_THROW(multi_output.write(MessageType::INFO, "Message"));
    EXPECT_NO_THROW(multi_output.flush());
}

TEST_F(UserOutputTest, MultiOutput_EnabledIfAnyOutputEnabled) {
    MultiUserOutput multi_output;

    auto console_output = std::make_unique<ConsoleUserOutput>();
    console_output->set_type_enabled(MessageType::INFO, true);
    console_output->set_type_enabled(MessageType::ERROR, false);

    auto null_output = std::make_unique<NullUserOutput>();

    multi_output.add_output(std::move(console_output));
    multi_output.add_output(std::move(null_output));

    // Enabled if ANY output has it enabled
    EXPECT_TRUE(multi_output.is_enabled(MessageType::INFO));
    EXPECT_FALSE(multi_output.is_enabled(MessageType::ERROR));
}

// ============================================================================
// CallbackUserOutput Tests
// ============================================================================

TEST_F(UserOutputTest, CallbackOutput_CallsCallback) {
    std::string captured_message;
    MessageType captured_type = MessageType::INFO;

    auto callback = [&](MessageType type, const char* message) {
        captured_type = type;
        captured_message = message;
    };

    CallbackUserOutput output(callback);

    output.write(MessageType::SUCCESS, "Callback test");

    EXPECT_EQ(captured_type, MessageType::SUCCESS);
    EXPECT_EQ(captured_message, "Callback test");
}

// ============================================================================
// UserOutputManager Tests
// ============================================================================

TEST_F(UserOutputTest, Manager_InitializeDefault) {
    EXPECT_FALSE(UserOutputManager::is_initialized());

    UserOutputManager::initialize();
    EXPECT_TRUE(UserOutputManager::is_initialized());

    EXPECT_NO_THROW(UserOutputManager::info("Test message"));
    EXPECT_NO_THROW(UserOutputManager::success("Success message"));
    EXPECT_NO_THROW(UserOutputManager::warning("Warning message"));
    EXPECT_NO_THROW(UserOutputManager::error("Error message"));
    EXPECT_NO_THROW(UserOutputManager::progress("Progress message"));
}

TEST_F(UserOutputTest, Manager_InitializeCustom) {
    const std::string temp_file = get_temp_file_path("manager.txt");

    {
        auto file_output = std::make_unique<FileUserOutput>(temp_file, false);
        UserOutputManager::initialize(std::move(file_output));

        UserOutputManager::info("Manager test message");
        UserOutputManager::flush();
        UserOutputManager::shutdown();
    }

    const std::string contents = read_file(temp_file);
    EXPECT_TRUE(contents.find("Manager test message") != std::string::npos);

    std::filesystem::remove(temp_file);
}

TEST_F(UserOutputTest, Manager_InitializeNull) {
    UserOutputManager::initialize_null();

    // All types should be disabled
    EXPECT_FALSE(UserOutputManager::is_enabled(MessageType::INFO));
    EXPECT_FALSE(UserOutputManager::is_enabled(MessageType::SUCCESS));

    // Should not throw
    EXPECT_NO_THROW(UserOutputManager::info("Should be discarded"));
}

TEST_F(UserOutputTest, Manager_InitializeWithCallback) {
    std::string captured_message;

    auto callback = [&](MessageType /*type*/, const char* message) {
        captured_message = message;
    };

    UserOutputManager::initialize_with_callback(callback);
    UserOutputManager::info("Callback message");

    EXPECT_EQ(captured_message, "Callback message");
}

TEST_F(UserOutputTest, Manager_VariadicTemplates) {
    std::string captured_message;

    auto callback = [&](MessageType /*type*/, const char* message) {
        captured_message = message;
    };

    UserOutputManager::initialize_with_callback(callback);

    // Test variadic template formatting
    UserOutputManager::info("Value: ", 42, ", Name: ", "test");

    EXPECT_EQ(captured_message, "Value: 42, Name: test");
}

TEST_F(UserOutputTest, Manager_Shutdown) {
    UserOutputManager::initialize();
    EXPECT_TRUE(UserOutputManager::is_initialized());

    UserOutputManager::shutdown();
    EXPECT_FALSE(UserOutputManager::is_initialized());

    // After shutdown, should use fallback and not throw
    EXPECT_NO_THROW(UserOutputManager::info("Fallback message"));
}

// ============================================================================
// TemporaryUserOutput Tests
// ============================================================================

TEST_F(UserOutputTest, TemporaryOutput_RestoresPrevious) {
    const std::string temp_file = get_temp_file_path("temp_restore.txt");

    // Initialize with console output
    UserOutputManager::initialize();

    {
        // Temporarily switch to file output
        auto temp_file_output = std::make_unique<FileUserOutput>(temp_file, false);
        TemporaryUserOutput temp(std::move(temp_file_output));

        UserOutputManager::info("Temporary message");
        UserOutputManager::flush();
    }
    // Should restore console output here

    EXPECT_TRUE(UserOutputManager::is_initialized());

    // Verify file was written during temporary period
    const std::string contents = read_file(temp_file);
    EXPECT_TRUE(contents.find("Temporary message") != std::string::npos);

    std::filesystem::remove(temp_file);
}

TEST_F(UserOutputTest, TemporaryOutput_WithNoInitialOutput) {
    EXPECT_FALSE(UserOutputManager::is_initialized());

    {
        auto null_output = std::make_unique<NullUserOutput>();
        TemporaryUserOutput temp(std::move(null_output));

        EXPECT_TRUE(UserOutputManager::is_initialized());
        UserOutputManager::info("Temporary message");
    }

    // Should shutdown after temporary output destroyed
    EXPECT_FALSE(UserOutputManager::is_initialized());
}

// ============================================================================
// Message Type String Conversion Tests
// ============================================================================

TEST_F(UserOutputTest, MessageTypeToString) {
    EXPECT_EQ(message_type_to_string(MessageType::INFO), "INFO");
    EXPECT_EQ(message_type_to_string(MessageType::SUCCESS), "SUCCESS");
    EXPECT_EQ(message_type_to_string(MessageType::WARNING), "WARNING");
    EXPECT_EQ(message_type_to_string(MessageType::ERROR), "ERROR");
    EXPECT_EQ(message_type_to_string(MessageType::PROGRESS), "PROGRESS");
}
