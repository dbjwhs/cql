// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "../../include/cql/response_processor.hpp"
#include "../../include/cql/api_client.hpp"

namespace cql::test {

namespace {
std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}
} // namespace

// ---------------------------------------------------------------------------
// process_response(): black-box code-block extraction via the public API.
// ---------------------------------------------------------------------------

class ResponseProcessorTest : public ::testing::Test {
protected:
    ResponseProcessor make_processor() { return ResponseProcessor(ApiClientConfig{}); }
};

TEST_F(ResponseProcessorTest, PlainTextWithNoCodeBlocksYieldsNoFiles) {
    auto processor = make_processor();
    const auto files = processor.process_response("Just some prose, no fenced code here.");
    EXPECT_TRUE(files.empty());
}

TEST_F(ResponseProcessorTest, ExtractsCppCodeBlock) {
    auto processor = make_processor();
    const std::string response =
        "Here is the implementation:\n"
        "```cpp\n"
        "class Widget {\n"
        "public:\n"
        "    void run();\n"
        "};\n"
        "```\n";
    const auto files = processor.process_response(response);
    ASSERT_EQ(files.size(), 1u);
    EXPECT_EQ(files[0].m_language, "C++");
    EXPECT_TRUE(contains(files[0].m_content, "class Widget"));
    EXPECT_FALSE(files[0].m_filename.empty());
}

TEST_F(ResponseProcessorTest, UsesExplicitFilenameHint) {
    auto processor = make_processor();
    const std::string response =
        "Save this to filename: my_widget.hpp\n"
        "```cpp\n"
        "struct Foo { int x; };\n"
        "```\n";
    const auto files = processor.process_response(response);
    ASSERT_EQ(files.size(), 1u);
    EXPECT_EQ(files[0].m_filename, "my_widget.hpp");
}

TEST_F(ResponseProcessorTest, MarksTestCodeBlockAsTest) {
    auto processor = make_processor();
    const std::string response =
        "```cpp\n"
        "TEST(WidgetTest, Runs) { run_the_test(); }\n"
        "```\n";
    const auto files = processor.process_response(response);
    ASSERT_FALSE(files.empty());
    bool any_test = false;
    for (const auto& file : files) {
        any_test = any_test || file.m_is_test;
    }
    EXPECT_TRUE(any_test);
}

// ---------------------------------------------------------------------------
// save_generated_file(): writes into a per-test temp directory (hermetic).
// A unique directory per test avoids collisions when ctest runs cases in
// parallel processes.
// ---------------------------------------------------------------------------

class ResponseProcessorSaveTest : public ::testing::Test {
protected:
    std::filesystem::path m_dir;

    void SetUp() override {
        m_dir = std::filesystem::path(::testing::TempDir()) /
                ("cql_rp_" +
                 std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()));
        std::error_code ec;
        std::filesystem::remove_all(m_dir, ec);
        std::filesystem::create_directories(m_dir);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(m_dir, ec);
    }
};

TEST_F(ResponseProcessorSaveTest, WritesFileWithContent) {
    ApiClientConfig config;
    config.set_create_missing_directories(true);
    const GeneratedFile file{"out.txt", "text", "hello world", false};

    EXPECT_TRUE(save_generated_file(file, m_dir.string(), config));
    const auto path = m_dir / "out.txt";
    ASSERT_TRUE(std::filesystem::exists(path));
    EXPECT_TRUE(contains(read_file(path), "hello world"));
}

TEST_F(ResponseProcessorSaveTest, DoesNotOverwriteWhenDisabled) {
    ApiClientConfig config;
    config.set_create_missing_directories(true);
    config.set_overwrite_existing_files(false);
    const GeneratedFile original{"out.txt", "text", "ORIGINAL_CONTENT", false};
    const GeneratedFile replacement{"out.txt", "text", "REPLACEMENT_CONTENT", false};

    ASSERT_TRUE(save_generated_file(original, m_dir.string(), config));
    // Second save must not clobber the original; it writes a ".new" sibling instead.
    EXPECT_TRUE(save_generated_file(replacement, m_dir.string(), config));

    const std::string original_file = read_file(m_dir / "out.txt");
    EXPECT_TRUE(contains(original_file, "ORIGINAL_CONTENT"));
    EXPECT_FALSE(contains(original_file, "REPLACEMENT_CONTENT"));

    ASSERT_TRUE(std::filesystem::exists(m_dir / "out.txt.new"));
    EXPECT_TRUE(contains(read_file(m_dir / "out.txt.new"), "REPLACEMENT_CONTENT"));
}

} // namespace cql::test
