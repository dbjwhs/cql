// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <any>
#include <typeindex>
#include <vector>
#include <memory>

namespace doc_system {

// Message structure
struct Message {
    std::string topic;
    std::any payload;
    std::type_index payloadType;
    
    template<typename T>
    Message(std::string t, T&& p) 
        : topic(std::move(t)), 
          payload(std::forward<T>(p)),
          payloadType(typeid(T)) {}
          
    // Helper to get typed payload
    template<typename T>
    T getPayload() const {
        if (payloadType != typeid(T)) {
            throw std::runtime_error("Type mismatch in message payload");
        }
        return std::any_cast<T>(payload);
    }
};

// Message queue for microservice communication
class MessageQueue {
public:
    // Singleton access
    static MessageQueue& getInstance() {
        static MessageQueue instance;
        return instance;
    }
    
    // Non-copyable and non-movable
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;
    MessageQueue(MessageQueue&&) = delete;
    MessageQueue& operator=(MessageQueue&&) = delete;
    
    // Publish a message to a topic
    template<typename T>
    void publish(const std::string& topic, T&& payload) {
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            messageQueue_.push(Message(topic, std::forward<T>(payload)));
        }
        
        queueCondition_.notify_one();
    }
    
    // Subscribe to a topic with a callback
    template<typename T>
    void subscribe(const std::string& topic, std::function<void(T)> callback) {
        std::lock_guard<std::mutex> lock(subscriptionMutex_);
        
        auto wrappedCallback = [callback](const Message& msg) {
            if (msg.payloadType == typeid(T)) {
                callback(msg.getPayload<T>());
            }
        };
        
        subscriptions_[topic].push_back(wrappedCallback);
    }
    
    // Start processing messages in the background
    void startProcessing() {
        if (processingThread_.joinable()) {
            return; // Already running
        }
        
        running_ = true;
        processingThread_ = std::thread([this]() {
            this->processMessages();
        });
    }
    
    // Stop processing messages
    void stopProcessing() {
        running_ = false;
        queueCondition_.notify_all();
        
        if (processingThread_.joinable()) {
            processingThread_.join();
        }
    }
    
    // Wait until the queue is empty
    void waitUntilEmpty() {
        std::unique_lock<std::mutex> lock(queueMutex_);
        emptyCondition_.wait(lock, [this]() {
            return messageQueue_.empty();
        });
    }
    
    // Get the current queue size
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return messageQueue_.size();
    }
    
    ~MessageQueue() {
        stopProcessing();
    }
    
private:
    MessageQueue() : running_(false) {}
    
    // Process messages in the queue
    void processMessages() {
        while (running_) {
            Message currentMessage = waitForMessage();
            
            if (!running_) {
                break;
            }
            
            dispatchMessage(currentMessage);
            
            // Notify if the queue is empty
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                if (messageQueue_.empty()) {
                    emptyCondition_.notify_all();
                }
            }
        }
    }
    
    // Wait for a message to be available
    Message waitForMessage() {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        queueCondition_.wait(lock, [this]() {
            return !messageQueue_.empty() || !running_;
        });
        
        if (!running_) {
            return Message("", 0); // Dummy message
        }
        
        Message message = messageQueue_.front();
        messageQueue_.pop();
        return message;
    }
    
    // Dispatch a message to all subscribers
    void dispatchMessage(const Message& message) {
        std::vector<std::function<void(const Message&)>> callbacks;
        
        {
            std::lock_guard<std::mutex> lock(subscriptionMutex_);
            auto it = subscriptions_.find(message.topic);
            if (it != subscriptions_.end()) {
                callbacks = it->second;
            }
        }
        
        for (const auto& callback : callbacks) {
            callback(message);
        }
    }
    
    mutable std::mutex queueMutex_;
    mutable std::mutex subscriptionMutex_;
    std::condition_variable queueCondition_;
    std::condition_variable emptyCondition_;
    std::queue<Message> messageQueue_;
    std::unordered_map<std::string, std::vector<std::function<void(const Message&)>>> subscriptions_;
    std::thread processingThread_;
    std::atomic<bool> running_;
};

// Microservice base class
class Microservice {
public:
    Microservice(const std::string& name) : name_(name), running_(false) {}
    
    virtual ~Microservice() {
        stop();
    }
    
    // Start the microservice
    void start() {
        if (running_) {
            return;
        }
        
        running_ = true;
        
        // Start message processing
        MessageQueue::getInstance().startProcessing();
        
        // Initialize the service
        initialize();
        
        // Start the service thread
        serviceThread_ = std::thread([this]() {
            this->run();
        });
    }
    
    // Stop the microservice
    void stop() {
        running_ = false;
        
        if (serviceThread_.joinable()) {
            serviceThread_.join();
        }
    }
    
    // Get the service name
    std::string getName() const {
        return name_;
    }
    
protected:
    // Initialize subscriptions and service state
    virtual void initialize() = 0;
    
    // Main service loop
    virtual void run() = 0;
    
    // Helper for subscribing to messages
    template<typename T>
    void subscribe(const std::string& topic, std::function<void(T)> callback) {
        MessageQueue::getInstance().subscribe<T>(topic, std::move(callback));
    }
    
    // Helper for publishing messages
    template<typename T>
    void publish(const std::string& topic, T&& payload) {
        MessageQueue::getInstance().publish(topic, std::forward<T>(payload));
    }
    
    std::string name_;
    std::atomic<bool> running_;
    std::thread serviceThread_;
};

// DocumentProcessingService - a concrete microservice example
class DocumentProcessingService : public Microservice {
public:
    DocumentProcessingService() : Microservice("DocumentProcessor") {}
    
protected:
    void initialize() override {
        // Subscribe to document processing requests
        subscribe<DocumentPtr>("document.process", [this](DocumentPtr doc) {
            processDocument(doc);
        });
    }
    
    void run() override {
        while (running_) {
            // Service heartbeat or other periodic tasks
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
private:
    void processDocument(DocumentPtr document) {
        if (!document) {
            return;
        }
        
        std::cout << "Processing document: " << document->getFilename() << std::endl;
        
        // Apply some processing to the document
        document->applyFormatting("corporate");
        
        // Notify that processing is complete
        publish("document.processed", document);
    }
};

} // namespace doc_system
