// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include <string>
#include "../../include/cql/template_validator.hpp"

namespace cql::test {

namespace {
// Count validation issues whose message contains a given substring.
size_t count_messages_containing(const TemplateValidationResult& result,
                                 const std::string& needle) {
    size_t count = 0;
    for (const auto& issue : result.get_issues()) {
        if (issue.get_message().find(needle) != std::string::npos) {
            ++count;
        }
    }
    return count;
}
} // namespace

class TemplateValidatorTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Regression tests for multi-line directive extraction.
//
// Before the fix, extract_directives() used a non-multiline "^(@...)" pattern,
// so only the directive on the first line was ever seen. That let invalid
// directives after line 1 slip through and falsely reported a @description
// below the first line as "missing".
// ---------------------------------------------------------------------------

TEST_F(TemplateValidatorTest, InvalidDirectiveAfterFirstLineIsDetected) {
    const std::string content =
        "@description \"A valid description\"\n"
        "@bogus \"not a real directive\"\n";
    const TemplateValidator validator;
    const TemplateValidationResult result = validator.validate_content(content);

    EXPECT_EQ(count_messages_containing(result, "@bogus"), 1u)
        << "an invalid directive after the first line must be detected";
    EXPECT_GE(result.count_errors(), 1u);
}

TEST_F(TemplateValidatorTest, DescriptionOnLaterLineIsNotReportedMissing) {
    const std::string content =
        "@copyright \"2025 example\"\n"
        "@description \"A valid description\"\n";
    const TemplateValidator validator;
    const TemplateValidationResult result = validator.validate_content(content);

    EXPECT_EQ(count_messages_containing(result, "Essential directive is missing"), 0u)
        << "@description on a later line must satisfy the essential-directive check";
}

// ---------------------------------------------------------------------------
// The validator's valid-directive set must match the language the lexer accepts.
// These directives (accepted by Lexer::lex_keyword) were previously missing from
// the validator and, once multi-line extraction was fixed, would have been
// falsely flagged as invalid.
// ---------------------------------------------------------------------------

TEST_F(TemplateValidatorTest, KnownCompilerDirectivesAreNotFlaggedInvalid) {
    const std::string content =
        "@description \"Uses several valid directives\"\n"
        "@performance \"low latency\"\n"
        "@provider \"openai\"\n"
        "@model \"gpt-4\"\n"
        "@output_format \"json\"\n"
        "@max_tokens \"1024\"\n"
        "@temperature \"0.2\"\n";
    const TemplateValidator validator;
    const TemplateValidationResult result = validator.validate_content(content);

    EXPECT_EQ(count_messages_containing(result, "Invalid directive"), 0u) << result.get_summary();
    EXPECT_EQ(result.count_errors(), 0u) << result.get_summary();
}

// ---------------------------------------------------------------------------
// Core validate_content behavior.
// ---------------------------------------------------------------------------

TEST_F(TemplateValidatorTest, MissingDescriptionIsWarned) {
    const std::string content = "@copyright \"2025 example\"\n";
    const TemplateValidator validator;
    const TemplateValidationResult result = validator.validate_content(content);

    EXPECT_GE(count_messages_containing(result, "Essential directive is missing"), 1u);
}

TEST_F(TemplateValidatorTest, UnknownDirectiveIsAnError) {
    const std::string content =
        "@notadirective \"x\"\n"
        "@description \"d\"\n";
    const TemplateValidator validator;
    const TemplateValidationResult result = validator.validate_content(content);

    EXPECT_GE(result.count_errors(), 1u);
    EXPECT_EQ(count_messages_containing(result, "@notadirective"), 1u);
}

TEST_F(TemplateValidatorTest, UndeclaredVariableIsWarned) {
    const std::string content =
        "@description \"Uses an undeclared variable\"\n"
        "@context \"Say ${greeting} to the user\"\n";
    const TemplateValidator validator;
    const TemplateValidationResult result = validator.validate_content(content);

    EXPECT_GE(count_messages_containing(result, "not declared"), 1u);
}

TEST_F(TemplateValidatorTest, UnusedDeclaredVariableIsWarned) {
    const std::string content =
        "@description \"Declares a variable it never uses\"\n"
        "@variable \"unused\" \"value\"\n";
    const TemplateValidator validator;
    const TemplateValidationResult result = validator.validate_content(content);

    EXPECT_GE(count_messages_containing(result, "not used"), 1u);
}

TEST_F(TemplateValidatorTest, WellFormedTemplateHasNoErrors) {
    const std::string content =
        "@description \"A well-formed template\"\n"
        "@variable \"name\" \"world\"\n"
        "@context \"Greet ${name}\"\n";
    const TemplateValidator validator;
    const TemplateValidationResult result = validator.validate_content(content);

    EXPECT_EQ(result.count_errors(), 0u) << result.get_summary();
    EXPECT_FALSE(result.has_issues(TemplateValidationLevel::ERROR));
}

} // namespace cql::test
