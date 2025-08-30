# XML Tags Support in CQL - Technical Specification

## Overview

This specification outlines the implementation of XML tag support in the CQL project for structured communication with Claude's API. XML tags improve prompt clarity, enable better response parsing, and enhance the overall integration with Claude's capabilities.

## Goals

1. Enhance template structure with XML tags
2. Improve response processing for structured XML responses
3. Support conversation context with role-based XML tags
4. Enable structured code generation with XML delimiters
5. Extend template validation to validate XML structure

## Implementation Details

### 1. Template Enhancements

#### 1.1 XML Tag Recognition in Templates

Extend the template system to recognize and preserve XML tags in templates:

```cpp
// Add to template_manager.hpp
class TemplateManager {
public:
    // Existing methods...
    
    // New method to process XML tags in templates
    [[nodiscard]] std::string process_xml_tags(const std::string& template_content) const;
    
private:
    // Existing members...
    
    // Map of predefined XML tags and their descriptions
    static const std::unordered_map<std::string, std::string> m_recognized_tags;
};
```

#### 1.2 Template XML Tag Definitions

Define a set of recommended XML tags for templates:

```cpp
// In template_manager.cpp
const std::unordered_map<std::string, std::string> TemplateManager::m_recognized_tags = {
    {"instructions", "High-level instructions for Claude"},
    {"context", "Background information or context for the query"},
    {"examples", "Example inputs and outputs for few-shot learning"},
    {"example", "A single example within examples"},
    {"input", "Example input within an example"},
    {"output", "Example output within an example"},
    {"query", "The specific query or question to answer"},
    {"format", "Output format instructions"},
    {"thinking", "Step-by-step reasoning process"},
    {"code", "Code snippet with optional language attribute"}
};
```

### 2. Response Processing for XML

#### 2.1 XML-Aware Response Parsing

Enhance the ResponseProcessor to handle XML-structured responses:

```cpp
// Add to response_processor.hpp
class ResponseProcessor {
public:
    // Existing methods...
    
    // New methods for XML parsing
    [[nodiscard]] std::string extract_tag_content(const std::string& response, 
                                                const std::string& tag_name) const;
    
    [[nodiscard]] std::vector<std::string> extract_all_tag_content(
        const std::string& response, const std::string& tag_name) const;
    
    // Extract content with attributes
    [[nodiscard]] std::map<std::string, std::string> extract_tag_attributes(
        const std::string& response, const std::string& tag_name) const;
};
```

#### 2.2 XML Response Structure

Define standard response structure expectations:

```cpp
// Add to response_processor.cpp
std::string ResponseProcessor::process_structured_response(const std::string& raw_response) {
    // Check if response contains recognized XML tags
    if (contains_xml_structure(raw_response)) {
        // Process based on expected structure
        std::string result;
        
        // Extract main content
        std::string main_content = extract_tag_content(raw_response, "response");
        if (!main_content.empty()) {
            result = main_content;
        }
        
        // Handle code separately if present
        std::string code = extract_tag_content(raw_response, "code");
        if (!code.empty()) {
            // Process code based on attributes
            auto attributes = extract_tag_attributes(raw_response, "code");
            std::string language = attributes["language"];
            
            // Format code based on language
            result = format_code(code, language);
        }
        
        return result;
    }
    
    // Fall back to unstructured processing
    return process_unstructured_response(raw_response);
}
```

### 3. Conversation Context with XML

#### 3.1 Conversation Message Structure

Implement the Message struct with XML support:

```cpp
// Add to api_client.hpp
struct Message {
    std::string role;     // "user", "assistant", etc.
    std::string content;  // Message content
    
    // Convert to XML format
    [[nodiscard]] std::string to_xml() const {
        return "<message role=\"" + role + "\">" + content + "</message>";
    }
    
    // Create from XML
    static Message from_xml(const std::string& xml_message);
};

// Add to api_client.cpp
Message Message::from_xml(const std::string& xml_message) {
    Message message;
    
    // Extract role attribute
    std::regex role_pattern("<message role=\"([^\"]+)\"");
    std::smatch role_match;
    if (std::regex_search(xml_message, role_match, role_pattern) && role_match.size() > 1) {
        message.role = role_match[1];
    }
    
    // Extract content between tags
    std::regex content_pattern("<message[^>]*>(.*?)</message>");
    std::smatch content_match;
    if (std::regex_search(xml_message, content_match, content_pattern) && content_match.size() > 1) {
        message.content = content_match[1];
    }
    
    return message;
}
```

#### 3.2 Conversation Context Implementation

Implement conversation support with XML formatting:

```cpp
// Add to api_client.hpp
class ApiClient {
public:
    // Existing methods...
    
    [[nodiscard]] ApiResponse submit_conversation(const std::vector<Message>& messages) const;
    
    // Format conversation as XML
    [[nodiscard]] std::string format_conversation_xml(const std::vector<Message>& messages) const;
};

// Add to api_client.cpp
std::string ApiClient::format_conversation_xml(const std::vector<Message>& messages) const {
    std::string conversation = "<conversation>\n";
    
    for (const auto& message : messages) {
        conversation += "  " + message.to_xml() + "\n";
    }
    
    conversation += "</conversation>";
    return conversation;
}

ApiResponse ApiClient::submit_conversation(const std::vector<Message>& messages) const {
    // Format messages as XML
    std::string conversation_xml = format_conversation_xml(messages);
    
    // Prepare request with conversation context
    // ...
    
    // Submit to API and return response
    // ...
}
```

### 4. Structured Code Generation

#### 4.1 Code Extraction and Formatting

Add support for extracting and formatting code from XML tags:

```cpp
// Add to response_processor.hpp
class ResponseProcessor {
public:
    // Existing methods...
    
    // Extract code with optional language attribute
    [[nodiscard]] std::pair<std::string, std::string> extract_code(const std::string& response) const;
    
    // Format code based on language
    [[nodiscard]] std::string format_code(const std::string& code, const std::string& language) const;
};

// Add to response_processor.cpp
std::pair<std::string, std::string> ResponseProcessor::extract_code(const std::string& response) const {
    std::string code;
    std::string language;
    
    // Extract code tag with language attribute
    std::regex code_pattern("<code\\s+language=\"([^\"]+)\">(.*?)</code>");
    std::smatch code_match;
    
    // Use regex_search with flags for multiline support
    if (std::regex_search(response, code_match, code_pattern, 
                          std::regex_constants::match_default | std::regex_constants::format_no_copy)) {
        if (code_match.size() > 2) {
            language = code_match[1];
            code = code_match[2];
        }
    }
    
    // Try without language attribute if not found
    if (code.empty()) {
        std::regex simple_code_pattern("<code>(.*?)</code>");
        std::smatch simple_match;
        
        if (std::regex_search(response, simple_match, simple_code_pattern,
                             std::regex_constants::match_default | std::regex_constants::format_no_copy)) {
            if (simple_match.size() > 1) {
                code = simple_match[1];
            }
        }
    }
    
    return {code, language};
}
```

### 5. Template Validation Extensions

#### 5.1 XML Validation in Templates

Extend the template validator to validate XML structure:

```cpp
// Add to template_validator.hpp
class TemplateValidator {
public:
    // Existing methods...
    
    // Validate XML structure in templates
    [[nodiscard]] bool validate_xml_structure(const std::string& template_content) const;
    
    // Get XML validation errors
    [[nodiscard]] std::vector<std::string> get_xml_validation_errors() const;
    
private:
    // Existing members...
    
    std::vector<std::string> m_xml_validation_errors;
};

// Add to template_validator.cpp
bool TemplateValidator::validate_xml_structure(const std::string& template_content) const {
    // Reset validation errors
    m_xml_validation_errors.clear();
    
    // Check for balanced XML tags
    std::stack<std::string> tag_stack;
    std::regex tag_pattern("<(/?)([^>\\s]+)[^>]*>");
    
    auto tag_begin = std::sregex_iterator(template_content.begin(), template_content.end(), tag_pattern);
    auto tag_end = std::sregex_iterator();
    
    for (std::sregex_iterator i = tag_begin; i != tag_end; ++i) {
        std::smatch match = *i;
        std::string is_closing = match[1];
        std::string tag_name = match[2];
        
        if (is_closing.empty()) {
            // Opening tag
            tag_stack.push(tag_name);
        } else {
            // Closing tag
            if (tag_stack.empty() || tag_stack.top() != tag_name) {
                m_xml_validation_errors.push_back("Mismatched XML tag: " + tag_name);
                return false;
            }
            tag_stack.pop();
        }
    }
    
    // Check for unclosed tags
    if (!tag_stack.empty()) {
        m_xml_validation_errors.push_back("Unclosed XML tag: " + tag_stack.top());
        return false;
    }
    
    return true;
}
```

## Usage Examples

### Template Example with XML Tags

```
# Query Template with XML Structure

<instructions>
Analyze the provided code and identify potential performance bottlenecks.
Focus on algorithmic complexity, memory usage, and concurrency issues.
</instructions>

<context>
The code is part of a distributed task scheduler that manages worker nodes
and distributes computational tasks across a network.
</context>

<code language="cpp">
{code_to_analyze}
</code>

<query>
What are the main performance bottlenecks in this code?
How can they be improved while maintaining the same functionality?
</query>

<format>
Provide your analysis in the following structure:
1. Identified bottlenecks (list each with explanation)
2. Improvement suggestions (code examples welcome)
3. Expected performance impact
</format>
```

### Conversation Example with XML Tags

```cpp
// Create conversation with XML formatting
std::vector<Message> conversation;

// Add user message
Message user_msg;
user_msg.role = "user";
user_msg.content = "<query>How can I implement retry logic with exponential backoff?</query>";
conversation.push_back(user_msg);

// Add assistant message
Message assistant_msg;
assistant_msg.role = "assistant";
assistant_msg.content = "<response>Exponential backoff is a strategy where...</response>";
conversation.push_back(assistant_msg);

// Add follow-up user message
Message followup_msg;
followup_msg.role = "user";
followup_msg.content = "<query>Can you show me an example in C++?</query>";
conversation.push_back(followup_msg);

// Submit conversation to Claude API
ApiResponse response = api_client.submit_conversation(conversation);

// Extract code from response
auto [code, language] = response_processor.extract_code(response.m_raw_response);
if (!code.empty()) {
    // Implement the extracted code
    std::cout << "Implementing the following code:\n" << code << std::endl;
}
```

### Response Processing Example

```cpp
// Process a response with XML structure
std::string raw_response = R"(
<response>
  <thinking>
    The user is asking about retry logic implementation. This is a common pattern
    in distributed systems to handle transient failures. Key considerations:
    1. Initial delay time
    2. Backoff multiplier
    3. Maximum retries
    4. Jitter for avoiding thundering herd
  </thinking>
  
  <explanation>
    Exponential backoff is a strategy for retrying operations where the delay
    between retries increases exponentially. This helps prevent overwhelming
    the target system during recovery from failure.
  </explanation>
  
  <code language="cpp">
  ApiResponse send_request_with_retry(const std::string& query) {
      int current_retry = 0;
      double retry_delay = 1.0; // Initial delay in seconds
      const int max_retries = 5;
      
      while (true) {
          ApiResponse response = perform_request(query);
          
          if (response.m_success || current_retry >= max_retries) {
              return response;
          }
          
          // Log retry attempt
          Logger::getInstance().log(LogLevel::INFO, 
              "Retrying request (", current_retry + 1, "/", 
              max_retries, ") after ", retry_delay, " seconds");
          
          // Sleep with backoff
          std::this_thread::sleep_for(std::chrono::milliseconds(
              static_cast<int>(retry_delay * 1000)));
          
          // Increase retry counter and delay
          current_retry++;
          retry_delay *= 2; // Exponential backoff
      }
  }
  </code>
</response>
)";

// Extract different components
std::string thinking = response_processor.extract_tag_content(raw_response, "thinking");
std::string explanation = response_processor.extract_tag_content(raw_response, "explanation");
auto [code, language] = response_processor.extract_code(raw_response);

// Process each component appropriately
std::cout << "Explanation: " << explanation << std::endl;
std::cout << "Code (" << language << "): " << code << std::endl;
```

## Implementation Plan

1. **Phase 1: Core XML Support**
   - Implement XML tag recognition in templates
   - Add XML validation to TemplateValidator
   - Create basic XML extraction utilities in ResponseProcessor

2. **Phase 2: Enhanced Features**
   - Implement conversation context with XML formatting
   - Add code extraction and formatting from XML tags
   - Enhance response processing for structured XML responses

3. **Phase 3: Integration & Testing**
   - Update existing templates to use XML structure
   - Add tests for XML processing functionality
   - Update documentation with XML tag guidelines

## Conclusion

Adding XML tag support to the CQL project will significantly enhance the quality and reliability of interactions with Claude's API. This implementation provides better structure for templates, more reliable response parsing, and enhanced support for conversation context. The result will be a more robust and maintainable API integration that aligns with Claude's recommended best practices.
