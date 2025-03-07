// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "document_strategy.h"
#include "document_factory.h"
#include "concrete_documents.h"
#include "concrete_tools.h"

using namespace doc_system;

// Test fixture for Strategy Pattern tests
class StrategyTest : public ::testing::Test {
protected:
    DocumentPtr document;
    std::stringstream buffer;
    std::streambuf* oldBuffer;
    
    void SetUp() override {
        document = DocumentFactory::createDocument("test.pdf");
        
        // Redirect cout to our stringstream buffer
        oldBuffer = std::cout.rdbuf(buffer.rdbuf());
    }
    
    void TearDown() override {
        // Restore the original buffer
        std::cout.rdbuf(oldBuffer);
    }
    
    // Helper to clear the buffer
    void clearBuffer() {
        buffer.str("");
        buffer.clear();
    }
};

// Test creating and applying text formatting strategies
TEST_F(StrategyTest, CreatesAndAppliesFormattingStrategies) {
    // Create and apply corporate formatting strategy
    auto corporateStrategy = TextFormattingStrategy::create("corporate");
    corporateStrategy->format(*document);
    
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Applying corporate formatting") != std::string::npos);
    
    // Clear buffer for next test
    clearBuffer();
    
    // Create and apply academic formatting strategy
    auto academicStrategy = TextFormattingStrategy::create("academic");
    academicStrategy->format(*document);
    
    output = buffer.str();
    EXPECT_TRUE(output.find("Applying academic formatting") != std::string::npos);
}

// Test creating and applying rendering strategies
TEST_F(StrategyTest, CreatesAndAppliesRenderingStrategies) {
    // Create and apply high-quality rendering strategy
    auto highQualityStrategy = ImageRenderingStrategy::create("high-quality");
    highQualityStrategy->render(*document);
    
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Applying high-quality rendering") != std::string::npos);
    
    // Clear buffer for next test
    clearBuffer();
    
    // Create and apply fast rendering strategy
    auto fastStrategy = ImageRenderingStrategy::create("fast");
    fastStrategy->render(*document);
    
    output = buffer.str();
    EXPECT_TRUE(output.find("Applying fast rendering") != std::string::npos);
}

// Test creating and applying layout strategies
TEST_F(StrategyTest, CreatesAndAppliesLayoutStrategies) {
    // Create and apply grid layout strategy
    auto gridStrategy = TableLayoutStrategy::create("grid");
    gridStrategy->layout(*document);
    
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Applying grid layout") != std::string::npos);
    
    // Clear buffer for next test
    clearBuffer();
    
    // Create and apply compact layout strategy
    auto compactStrategy = TableLayoutStrategy::create("compact");
    compactStrategy->layout(*document);
    
    output = buffer.str();
    EXPECT_TRUE(output.find("Applying compact layout") != std::string::npos);
}

// Test handling of unknown strategy names
TEST_F(StrategyTest, ThrowsForUnknownStrategyNames) {
    EXPECT_THROW(TextFormattingStrategy::create("unknown"), std::invalid_argument);
    EXPECT_THROW(ImageRenderingStrategy::create("unknown"), std::invalid_argument);
    EXPECT_THROW(TableLayoutStrategy::create("unknown"), std::invalid_argument);
}

// Test registering custom strategies
TEST_F(StrategyTest, RegistersAndUsesCustomStrategies) {
    // Create a custom formatting strategy
    class CustomFormattingStrategy : public TextFormattingStrategy {
    public:
        void format(Document& document) override {
            std::cout << "Applying custom formatting to " << document.getFilename() << std::endl;
        }
    };
    
    // Register the custom strategy
    StrategyRegistry::getInstance().registerFormattingStrategy<CustomFormattingStrategy>("custom");
    
    // Create and apply the custom strategy
    auto customStrategy = TextFormattingStrategy::create("custom");
    customStrategy->format(*document);
    
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Applying custom formatting") != std::string::npos);
}

// Test applying strategies through document interface
TEST_F(StrategyTest, AppliesStrategiesThroughDocumentInterface) {
    // Apply formatting through document interface
    document->applyFormatting("corporate");
    
    std::string output = buffer.str();
    EXPECT_TRUE(output.find("Applying corporate formatting") != std::string::npos);
    
    // Clear buffer for next test
    clearBuffer();
    
    // Apply rendering through document interface
    document->applyRendering("high-quality");
    
    output = buffer.str();
    EXPECT_TRUE(output.find("Applying high-quality rendering") != std::string::npos);
    
    // Clear buffer for next test
    clearBuffer();
    
    // Apply layout through document interface
    document->applyLayout("grid");
    
    output = buffer.str();
    EXPECT_TRUE(output.find("Applying grid layout") != std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
