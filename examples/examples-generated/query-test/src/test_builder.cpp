// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "document_builder.h"
#include "document_factory.h"
#include "concrete_documents.h"
#include "concrete_tools.h"
#include <vector>
#include <string>

using namespace doc_system;

// Test fixture for builder pattern tests
class BuilderTest : public ::testing::Test {
protected:
    DocumentPtr testDocument;
    DocumentBuilder builder;

    void SetUp() override {
        testDocument = DocumentFactory::createDocument("test.pdf");
        builder.reset(testDocument);
    }
};

// Test building document components individually
TEST_F(BuilderTest, BuildsComponentsIndividually) {
    // Build each component and check the build steps
    builder.buildHeader("Test Header");
    EXPECT_EQ(builder.getBuildSteps(), std::vector<std::string>{"Header"});

    builder.buildContent("Test Content");
    EXPECT_EQ(builder.getBuildSteps(), std::vector<std::string>({"Header", "Content"}));

    builder.buildFooter("Test Footer");
    EXPECT_EQ(builder.getBuildSteps(), std::vector<std::string>({"Header", "Content", "Footer"}));

    MetadataMap metadata = {{"author", "Test Author"}, {"date", "2025-03-05"}};
    builder.buildMetadata(metadata);
    EXPECT_EQ(builder.getBuildSteps(), std::vector<std::string>({"Header", "Content", "Footer", "Metadata"}));
}

// Test method chaining
TEST_F(BuilderTest, SupportsMethodChaining) {
    // Create a new builder instance for this test
    DocumentBuilder chainBuilder;
    chainBuilder.reset(testDocument);

    // Method chaining call
    DocumentBuilder& result = chainBuilder.buildHeader("Test Header")
                                        .buildContent("Test Content")
                                        .buildFooter("Test Footer");

    // Check that method chaining returns the builder itself
    EXPECT_EQ(&result, &chainBuilder);

    // Check that the build steps are recorded in the correct order
    EXPECT_EQ(chainBuilder.getBuildSteps(), std::vector<std::string>({"Header", "Content", "Footer"}));
}

// Test standard document building sequence through director
TEST_F(BuilderTest, DirectorBuildsStandardDocument) {
    // Create a new builder
    DocumentBuilder standardBuilder;
    standardBuilder.reset(testDocument);

    // Build a standard document using the director
    DocumentDirector::buildStandardDocument(
        standardBuilder,
        "Standard Header",
        "Standard Content",
        "Standard Footer",
        {{"key1", "value1"}, {"key2", "value2"}}
    );

    // Verify build order matches expected standard document flow
    EXPECT_EQ(standardBuilder.getBuildSteps(),
              std::vector<std::string>({"Header", "Content", "Footer", "Metadata"}));
}

// Test minimal document building sequence through director
TEST_F(BuilderTest, DirectorBuildsMinimalDocument) {
    // Create a new builder
    DocumentBuilder minimalBuilder;
    minimalBuilder.reset(testDocument);

    // Build a minimal document using the director
    DocumentDirector::buildMinimalDocument(
        minimalBuilder,
        "Minimal Content Only"
    );

    // Verify build order contains only content
    EXPECT_EQ(minimalBuilder.getBuildSteps(), std::vector<std::string>({"Content"}));
}

// Test business document building sequence through director
TEST_F(BuilderTest, DirectorBuildsBusinessDocument) {
    // Create a new builder
    DocumentBuilder businessBuilder;
    businessBuilder.reset(testDocument);

    // Build a business document using the director
    DocumentDirector::buildBusinessDocument(
        businessBuilder,
        "Business Header",
        "Business Content",
        {{"author", "Business User"}, {"department", "Finance"}}
    );

    // Verify business document has metadata first, then header, then content
    EXPECT_EQ(businessBuilder.getBuildSteps(),
              std::vector<std::string>({"Metadata", "Header", "Content"}));
}

// Test that getDocument returns the constructed document
TEST_F(BuilderTest, GetDocumentReturnsBuiltDocument) {
    builder.buildHeader("Test Header")
           .buildContent("Test Content");

    auto result = builder.getDocument();

    // Check that we get back the original document
    EXPECT_EQ(result, testDocument);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}