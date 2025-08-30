// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "document_strategy.h"
#include "document_factory.h"
#include "concrete_documents.h"
#include "document_manager.h"
#include "concrete_tools.h"

using namespace doc_system;

// Test fixture for Singleton tests
class SingletonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear the document manager before each test
        auto& manager = DocumentManager::getInstance();
        for (const auto& doc : manager.getAllDocuments()) {
            manager.unregisterDocument(doc->getFilename());
        }
    }
};

// Test that multiple getInstance calls return the same instance
TEST_F(SingletonTest, ReturnsSameInstance) {
    DocumentManager& instance1 = DocumentManager::getInstance();
    DocumentManager& instance2 = DocumentManager::getInstance();

    // Check that both references point to the same memory address
    EXPECT_EQ(&instance1, &instance2);
}

// Test document registration and retrieval
TEST_F(SingletonTest, RegistersAndRetrievesDocuments) {
    auto& manager = DocumentManager::getInstance();

    // Create and register a document
    auto doc1 = DocumentFactory::createDocument("test1.pdf");
    manager.registerDocument(doc1);

    // Retrieve the document
    auto retrievedDoc = manager.getDocument("test1.pdf");
    EXPECT_EQ(retrievedDoc, doc1);

    // Register a second document
    auto doc2 = DocumentFactory::createDocument("test2.docx");
    manager.registerDocument(doc2);

    // Check document count
    EXPECT_EQ(manager.getDocumentCount(), 2);

    // Retrieve all documents
    auto allDocs = manager.getAllDocuments();
    EXPECT_EQ(allDocs.size(), 2);

    // Check that both documents are in the collection
    bool foundDoc1 = false;
    bool foundDoc2 = false;

    for (const auto& doc : allDocs) {
        if (doc->getFilename() == "test1.pdf") foundDoc1 = true;
        if (doc->getFilename() == "test2.docx") foundDoc2 = true;
    }

    EXPECT_TRUE(foundDoc1);
    EXPECT_TRUE(foundDoc2);
}

// Test document unregistration
TEST_F(SingletonTest, UnregistersDocuments) {
    auto& manager = DocumentManager::getInstance();

    // Create and register documents
    auto doc1 = DocumentFactory::createDocument("test1.pdf");
    auto doc2 = DocumentFactory::createDocument("test2.docx");

    manager.registerDocument(doc1);
    manager.registerDocument(doc2);

    // Unregister one document
    bool result = manager.unregisterDocument("test1.pdf");
    EXPECT_TRUE(result);

    // Check document count
    EXPECT_EQ(manager.getDocumentCount(), 1);

    // Try to retrieve the unregistered document
    auto retrievedDoc = manager.getDocument("test1.pdf");
    EXPECT_EQ(retrievedDoc, nullptr);

    // Try to unregister a non-existent document
    result = manager.unregisterDocument("nonexistent.pdf");
    EXPECT_FALSE(result);
}

// Test thread safety of the singleton
TEST_F(SingletonTest, ThreadSafety) {
    constexpr int numThreads = 10;
    constexpr int numOperationsPerThread = 100;

    std::atomic<int> successCount(0);
    std::mutex testMutex;

    // Create threads that concurrently access the singleton
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i, &successCount, &testMutex]() {
            try {
                for (int j = 0; j < numOperationsPerThread; ++j) {
                    // Get the singleton instance
                    auto& manager = DocumentManager::getInstance();

                    // Create a document with a unique name
                    std::string filename = "thread_" + std::to_string(i) +
                                           "_doc_" + std::to_string(j) + ".pdf";

                    auto doc = DocumentFactory::createDocument(filename);

                    // Register the document
                    manager.registerDocument(doc);

                    // Retrieve the document
                    auto retrievedDoc = manager.getDocument(filename);

                    // Check if retrieval was successful
                    if (retrievedDoc && retrievedDoc->getFilename() == filename) {
                        successCount++;
                    }

                    // Unregister the document
                    manager.unregisterDocument(filename);
                }
            } catch (const std::exception& e) {
                // Log the error
                std::lock_guard<std::mutex> lock(testMutex);
                std::cerr << "Thread " << i << " error: " << e.what() << std::endl;
            }
        });
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Check that all operations were successful
    EXPECT_EQ(successCount, numThreads * numOperationsPerThread);

    // Check that the document manager is empty after all operations
    auto& manager = DocumentManager::getInstance();
    EXPECT_EQ(manager.getDocumentCount(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
