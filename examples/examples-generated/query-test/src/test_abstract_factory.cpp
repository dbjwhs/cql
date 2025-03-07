// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "abstract_factory.h"
#include "concrete_tools.h"
#include "document_factory.h"
#include <memory>
#include <tuple>

using namespace doc_system;

// Test fixture for Abstract Factory tests
class AbstractFactoryTest : public ::testing::Test {
protected:
    DocumentPtr pdfDocument;
    DocumentPtr wordDocument;
    DocumentPtr textDocument;
    
    void SetUp() override {
        pdfDocument = DocumentFactory::createDocument("test.pdf");
        wordDocument = DocumentFactory::createDocument("test.docx");
        textDocument = DocumentFactory::createDocument("test.txt");
    }
};

// Test creating the correct factory for each document type
TEST_F(AbstractFactoryTest, CreatesCorrectFactory) {
    auto pdfFactory = DocumentToolFactory::createFactory(DocumentType::PDF);
    auto wordFactory = DocumentToolFactory::createFactory(DocumentType::Word);
    auto textFactory = DocumentToolFactory::createFactory(DocumentType::Text);
    
    // Verify factory types using dynamic_cast
    EXPECT_NE(dynamic_cast<PdfToolFactory*>(pdfFactory.get()), nullptr);
    EXPECT_NE(dynamic_cast<WordToolFactory*>(wordFactory.get()), nullptr);
    EXPECT_NE(dynamic_cast<TextToolFactory*>(textFactory.get()), nullptr);
    
    // Verify factory throws for unknown types
    EXPECT_THROW(DocumentToolFactory::createFactory(DocumentType::Unknown), std::invalid_argument);
}

// Test that each factory creates the correct tool types
TEST_F(AbstractFactoryTest, CreatesCorrectToolTypes) {
    // PDF Tools
    auto pdfFactory = DocumentToolFactory::createFactory(DocumentType::PDF);
    auto pdfViewer = pdfFactory->createViewer();
    auto pdfEditor = pdfFactory->createEditor();
    auto pdfConverter = pdfFactory->createConverter();
    
    EXPECT_EQ(pdfViewer->getSupportedType(), DocumentType::PDF);
    EXPECT_EQ(pdfEditor->getSupportedType(), DocumentType::PDF);
    EXPECT_EQ(pdfConverter->getSourceType(), DocumentType::PDF);
    
    // Word Tools
    auto wordFactory = DocumentToolFactory::createFactory(DocumentType::Word);
    auto wordViewer = wordFactory->createViewer();
    auto wordEditor = wordFactory->createEditor();
    auto wordConverter = wordFactory->createConverter();
    
    EXPECT_EQ(wordViewer->getSupportedType(), DocumentType::Word);
    EXPECT_EQ(wordEditor->getSupportedType(), DocumentType::Word);
    EXPECT_EQ(wordConverter->getSourceType(), DocumentType::Word);
    
    // Text Tools
    auto textFactory = DocumentToolFactory::createFactory(DocumentType::Text);
    auto textViewer = textFactory->createViewer();
    auto textEditor = textFactory->createEditor();
    auto textConverter = textFactory->createConverter();
    
    EXPECT_EQ(textViewer->getSupportedType(), DocumentType::Text);
    EXPECT_EQ(textEditor->getSupportedType(), DocumentType::Text);
    EXPECT_EQ(textConverter->getSourceType(), DocumentType::Text);
}

// Test that tools created by the same factory are compatible with each other
TEST_F(AbstractFactoryTest, CreatesCompatibleTools) {
    // PDF Tools
    auto pdfFactory = DocumentToolFactory::createFactory(DocumentType::PDF);
    auto pdfViewer = pdfFactory->createViewer();
    auto pdfEditor = pdfFactory->createEditor();
    
    // They should both support the same document type
    EXPECT_EQ(pdfViewer->getSupportedType(), pdfEditor->getSupportedType());
    
    // Test that tools work with the correct document type
    EXPECT_NO_THROW(pdfViewer->view(pdfDocument));
    EXPECT_NO_THROW(pdfEditor->edit(pdfDocument));
    
    // Test that tools throw for incompatible document types
    EXPECT_THROW(pdfViewer->view(wordDocument), std::invalid_argument);
    EXPECT_THROW(pdfEditor->edit(textDocument), std::invalid_argument);
}

// Test the convenience ToolFactory wrapper
TEST_F(AbstractFactoryTest, ToolFactoryCreatesCompleteSets) {
    // Create tools for PDF document
    auto [pdfViewer, pdfEditor, pdfConverter] = ToolFactory::createToolsFor(DocumentType::PDF);
    
    // Verify tool types
    EXPECT_EQ(pdfViewer->getSupportedType(), DocumentType::PDF);
    EXPECT_EQ(pdfEditor->getSupportedType(), DocumentType::PDF);
    EXPECT_EQ(pdfConverter->getSourceType(), DocumentType::PDF);
    
    // Create tools for Word document
    auto [wordViewer, wordEditor, wordConverter] = ToolFactory::createToolsFor(DocumentType::Word);
    
    // Verify tool types
    EXPECT_EQ(wordViewer->getSupportedType(), DocumentType::Word);
    EXPECT_EQ(wordEditor->getSupportedType(), DocumentType::Word);
    EXPECT_EQ(wordConverter->getSourceType(), DocumentType::Word);
    
    // Create tools for Text document
    auto [textViewer, textEditor, textConverter] = ToolFactory::createToolsFor(DocumentType::Text);
    
    // Verify tool types
    EXPECT_EQ(textViewer->getSupportedType(), DocumentType::Text);
    EXPECT_EQ(textEditor->getSupportedType(), DocumentType::Text);
    EXPECT_EQ(textConverter->getSourceType(), DocumentType::Text);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
