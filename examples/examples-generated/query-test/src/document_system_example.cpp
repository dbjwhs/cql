// MIT License
// Copyright (c) 2025 dbjwhs

#include "document_types.h"
#include "document_factory.h"
#include "document_manager.h"
#include "document_builder.h"
#include "document_observer.h"
#include "document_strategy.h"
#include "abstract_factory.h"
#include "concrete_documents.h"
#include "concrete_tools.h"
#include "message_queue.h"

#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>

using namespace doc_system;

int main() {
    try {
        // Start the message queue
        auto& messageQueue = MessageQueue::getInstance();
        messageQueue.startProcessing();

        // Start the document processing service
        DocumentProcessingService processingService;
        processingService.start();

        // Example usage based on the provided example
        std::cout << "===== Document Processing System Example =====" << std::endl;
        
        // Create a document using factory method
        auto document = DocumentFactory::createDocument("report.pdf");
        std::cout << "Created document: " << document->getFilename() << std::endl;
        
        // Use builder pattern to construct the document
        DocumentBuilder builder;
        builder.reset(document);
        builder.buildHeader("Quarterly Report")
               .buildContent("This is the content of the report.")
               .buildFooter("Confidential")
               .buildMetadata({
                   {"author", "Jane Doe"},
                   {"created", "2025-02-28"}
               });
        
        auto doc = builder.getDocument();
        
        // Register with singleton document manager
        DocumentManager::getInstance().registerDocument(doc);
        std::cout << "Registered document with DocumentManager. Total documents: "
                  << DocumentManager::getInstance().getDocumentCount() << std::endl;
        
        // Create compatible tools using abstract factory
        auto [viewer, editor, converter] = ToolFactory::createToolsFor(doc->getType());
        std::cout << "Created compatible tools for document type" << std::endl;
        
        // Apply strategy pattern
        doc->applyFormatting("corporate");
        
        // Observer pattern
        doc->attachObserver(std::make_shared<AutoSaveObserver>());
        doc->attachObserver(std::make_shared<ValidationObserver>());
        doc->attachObserver(std::make_shared<LoggingObserver>());
        std::cout << "Attached observers to document" << std::endl;
        
        // Update content to trigger observers
        std::cout << "\nUpdating document content..." << std::endl;
        doc->updateContent("Updated content for the quarterly report.");
        
        // Use tools
        std::cout << "\nUsing document tools..." << std::endl;
        viewer->view(doc);
        editor->edit(doc);
        
        // Using message queue for processing
        std::cout << "\nSending document for processing via message queue..." << std::endl;
        messageQueue.publish("document.process", doc);
        
        // Wait for processing to complete
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Clean up
        std::cout << "\nShutting down services..." << std::endl;
        processingService.stop();
        messageQueue.stopProcessing();
        
        std::cout << "===== Example Completed Successfully =====" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
