// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include "../../include/cql/template_validator.hpp"
#include "../../include/cql/template_validator_schema.hpp"

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

// ---------------------------------------------------------------------------
// TemplateValidatorSchema — the directive-schema registry used by the CLI's
// template documentation/validation. Previously had no direct test coverage.
// These tests assert the public contract without pinning the exact directive
// set (which is known to be incomplete relative to the lexer).
// ---------------------------------------------------------------------------

class TemplateValidatorSchemaTest : public ::testing::Test {};

TEST_F(TemplateValidatorSchemaTest, DefaultSchemaRegistersStandardDirectives) {
    const auto schema = TemplateValidatorSchema::create_default_schema();
    const auto& directives = schema.get_all_directives();
    EXPECT_GE(directives.size(), 14u);
    EXPECT_TRUE(directives.contains("@description"));
    EXPECT_TRUE(directives.contains("@copyright"));
    EXPECT_TRUE(directives.contains("@variable"));
    EXPECT_TRUE(directives.contains("@performance"));
}

TEST_F(TemplateValidatorSchemaTest, DirectiveLookupReturnsSchemaOrNullopt) {
    const auto schema = TemplateValidatorSchema::create_default_schema();
    const auto description = schema.get_directive_schema("@description");
    ASSERT_TRUE(description.has_value());
    EXPECT_EQ(description->name, "@description");
    EXPECT_FALSE(schema.get_directive_schema("@no_such_directive").has_value());
}

TEST_F(TemplateValidatorSchemaTest, DefaultSchemaHasRequiredDirectives) {
    const auto schema = TemplateValidatorSchema::create_default_schema();
    EXPECT_FALSE(schema.get_required_directives().empty());
}

TEST_F(TemplateValidatorSchemaTest, RegisterCustomDirectiveRoundTrips) {
    TemplateValidatorSchema schema;
    schema.register_directive(TemplateValidatorSchema::DirectiveSchema("@custom", /*required=*/true));
    const auto found = schema.get_directive_schema("@custom");
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->required);
    const auto required = schema.get_required_directives();
    EXPECT_NE(std::find(required.begin(), required.end(), "@custom"), required.end());
}

TEST_F(TemplateValidatorSchemaTest, ValidationRulesRoundTrip) {
    const auto default_schema = TemplateValidatorSchema::create_default_schema();
    EXPECT_FALSE(default_schema.get_validation_rules().empty());

    TemplateValidatorSchema schema;
    schema.add_validation_rule("my_rule",
        [](const std::string&) { return std::vector<TemplateValidationIssue>{}; });
    EXPECT_TRUE(schema.get_validation_rules().contains("my_rule"));
}

} // namespace cql::test
