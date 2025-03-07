// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "document_factory.h"
#include "concrete_documents.h"

using namespace doc_system;

// Test fixture for factory method tests
class FactoryMethodTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test resources
    }
    
    void TearDown() override {
        // Clean up after each test
    }
};

// Test that factory creates PDF document for PDF extension
TEST_F(FactoryMethodTest, CreatesPdfDocumentForPdfExtension) {
    // Test with uppercase extension
    auto doc1 = DocumentFactory::createDocument("test.PDF");
    EXPECT_EQ(doc1->getType(), DocumentType::PDF);
    EXPECT_EQ(doc1->getFilename(), "test.PDF");
    
    // Test with lowercase extension
    auto doc2 = DocumentFactory::createDocument("report.pdf");
    EXPECT_EQ(doc2->getType(), DocumentType::PDF);
    EXPECT_EQ(doc2->getFilename(), "report.pdf");
}

// Test that factory creates Word document for Word extensions
TEST_F(FactoryMethodTest, CreatesWordDocumentForWordExtensions) {
    // Test .docx extension
    auto doc1 = DocumentFactory::createDocument("test.docx");
    EXPECT_EQ(doc1->getType(), DocumentType::Word);
    EXPECT_EQ(doc1->getFilename(), "test.docx");
    
    // Test .doc extension
    auto doc2 = DocumentFactory::createDocument("old_report.doc");
    EXPECT_EQ(doc2->getType(), DocumentType::Word);
    EXPECT_EQ(doc2->getFilename(), "old_report.doc");
}

// Test that factory creates Text document for txt extension
TEST_F(FactoryMethodTest, CreatesTextDocumentForTxtExtension) {
    auto doc = DocumentFactory::createDocument("notes.txt");
    EXPECT_EQ(doc->getType(), DocumentType::Text);
    EXPECT_EQ(doc->getFilename(), "notes.txt");
}

// Test that factory throws for unsupported extensions
TEST_F(FactoryMethodTest, ThrowsForUnsupportedExtension) {
    EXPECT_THROW(DocumentFactory::createDocument("image.jpg"), std::invalid_argument);
    EXPECT_THROW(DocumentFactory::createDocument("data.csv"), std::invalid_argument);
    EXPECT_THROW(DocumentFactory::createDocument("noextension"), std::invalid_argument);
}

// Test that factory handles complex filenames correctly
TEST_F(FactoryMethodTest, HandlesComplexFilenames) {
    // Test filename with path
    auto doc1 = DocumentFactory::createDocument("/path/to/document.pdf");
    EXPECT_EQ(doc1->getType(), DocumentType::PDF);
    EXPECT_EQ(doc1->getFilename(), "/path/to/document.pdf");
    
    // Test filename with spaces
    auto doc2 = DocumentFactory::createDocument("My Document.docx");
    EXPECT_EQ(doc2->getType(), DocumentType::Word);
    EXPECT_EQ(doc2->getFilename(), "My Document.docx");
    
    // Test filename with multiple dots
    auto doc3 = DocumentFactory::createDocument("version.1.2.final.txt");
    EXPECT_EQ(doc3->getType(), DocumentType::Text);
    EXPECT_EQ(doc3->getFilename(), "version.1.2.final.txt");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
