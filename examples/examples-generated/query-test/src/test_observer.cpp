// MIT License
// Copyright (c) 2025 dbjwhs

#include <gtest/gtest.h>
#include "document_strategy.h"
#include "document_factory.h"
#include "concrete_documents.h"
#include "concrete_tools.h"

using namespace doc_system;

// Custom observer for testing
class TestObserver : public DocumentObserver {
public:
    TestObserver() : notificationCount(0) {}
    
    void onNotify(const Document& document, const std::string& eventType) override {
        notificationCount++;
        lastDocument = document.getFilename();
        lastEventType = eventType;
    }
    
    int notificationCount;
    std::string lastDocument;
    std::string lastEventType;
};

// Test fixture for Observer Pattern tests
class ObserverTest : public ::testing::Test {
protected:
    DocumentPtr document;
    std::shared_ptr<TestObserver> observer1;
    std::shared_ptr<TestObserver> observer2;
    
    void SetUp() override {
        document = DocumentFactory::createDocument("test.pdf");
        observer1 = std::make_shared<TestObserver>();
        observer2 = std::make_shared<TestObserver>();
    }
};

// Test attaching observers
TEST_F(ObserverTest, AttachesObservers) {
    document->attachObserver(observer1);
    document->attachObserver(observer2);
    
    // Trigger a notification
    document->updateContent("New content");
    
    // Check that both observers were notified
    EXPECT_EQ(observer1->notificationCount, 1);
    EXPECT_EQ(observer2->notificationCount, 1);
    
    // Check that the event details were captured correctly
    EXPECT_EQ(observer1->lastDocument, "test.pdf");
    EXPECT_EQ(observer1->lastEventType, events::DocumentChanged);
    
    EXPECT_EQ(observer2->lastDocument, "test.pdf");
    EXPECT_EQ(observer2->lastEventType, events::DocumentChanged);
}

// Test detaching observers
TEST_F(ObserverTest, DetachesObservers) {
    document->attachObserver(observer1);
    document->attachObserver(observer2);
    
    // Detach one observer
    document->detachObserver(observer1);
    
    // Trigger a notification
    document->updateContent("New content");
    
    // Check that only the remaining observer was notified
    EXPECT_EQ(observer1->notificationCount, 0);
    EXPECT_EQ(observer2->notificationCount, 1);
}

// Test multiple notifications
TEST_F(ObserverTest, HandlesMultipleNotifications) {
    document->attachObserver(observer1);
    
    // Trigger multiple notifications
    document->updateContent("First update");
    document->notifyObservers(events::DocumentSaved);
    document->notifyObservers(events::DocumentLoaded);
    
    // Check notification count
    EXPECT_EQ(observer1->notificationCount, 3);
    
    // Check that the last event was captured correctly
    EXPECT_EQ(observer1->lastEventType, events::DocumentLoaded);
}

// Test concrete observer implementations
TEST_F(ObserverTest, ConcreteObserversRespond) {
    // Set up output capture
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    // Create concrete observers
    auto autoSave = std::make_shared<AutoSaveObserver>();
    auto validation = std::make_shared<ValidationObserver>();
    auto logging = std::make_shared<LoggingObserver>();
    
    // Attach observers
    document->attachObserver(autoSave);
    document->attachObserver(validation);
    document->attachObserver(logging);
    
    // Trigger notifications for different events
    document->updateContent("Updated content");
    document->notifyObservers(events::DocumentSaved);
    
    // Restore original buffer
    std::cout.rdbuf(old);
    
    // Check output for expected messages
    std::string output = buffer.str();
    
    // AutoSaveObserver should respond to DocumentChanged
    EXPECT_TRUE(output.find("AutoSaveObserver: Auto-saving document test.pdf") != std::string::npos);
    
    // ValidationObserver should respond to DocumentChanged
    EXPECT_TRUE(output.find("ValidationObserver: Validating document test.pdf") != std::string::npos);
    
    // LoggingObserver should respond to both events
    EXPECT_TRUE(output.find("LoggingObserver: Event 'documentChanged' occurred on document test.pdf") != std::string::npos);
    EXPECT_TRUE(output.find("LoggingObserver: Event 'documentSaved' occurred on document test.pdf") != std::string::npos);
}

// Test attaching the same observer multiple times
TEST_F(ObserverTest, DoesNotDuplicateObservers) {
    // Attach the same observer twice
    document->attachObserver(observer1);
    document->attachObserver(observer1);
    
    // Trigger a notification
    document->updateContent("New content");
    
    // Observer should only be notified once
    EXPECT_EQ(observer1->notificationCount, 1);
}

// Test null observer handling
TEST_F(ObserverTest, HandlesNullObservers) {
    // Try to attach a null observer
    document->attachObserver(nullptr);
    
    // This should not cause any errors
    EXPECT_NO_THROW(document->updateContent("New content"));
    
    // Try to detach a null observer
    EXPECT_NO_THROW(document->detachObserver(nullptr));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
