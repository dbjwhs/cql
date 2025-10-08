# Message Formatting Standards

This document defines the standards for user-facing messages and debug logging in CQL.

## Overview

CQL separates **user-facing output** from **debug logging**:

- **UserOutput**: Messages for end users during normal operation
- **Logger**: Messages for developers and diagnostics

## User-Facing Message Standards

### Message Types

Use the appropriate `MessageType` for each message:

- `INFO`: General informational messages
- `SUCCESS`: Success confirmations
- `WARNING`: Warning messages (non-fatal issues)
- `ERROR`: Error messages (user-facing, not debug)
- `PROGRESS`: Progress indicators and status updates

### Punctuation Rules

1. **Complete Sentences**: End with a period
   ```cpp
   UserOutputManager::info("Processing file: example.llm.");
   UserOutputManager::success("Template saved successfully.");
   ```

2. **Fragments and Labels**: No terminal punctuation
   ```cpp
   UserOutputManager::info("Template: ", template_name);
   UserOutputManager::info("Variables:");
   ```

3. **Lists**: No punctuation on individual items
   ```cpp
   UserOutputManager::info("  - ", item);
   ```

4. **Questions**: End with question mark
   ```cpp
   std::string response = UserOutputManager::prompt("Do you want to continue? (y/n): ");
   ```

### List Formatting

Use the `UserOutputManager::list()` helper for consistent list output:

```cpp
// Simple list
std::vector<std::string> templates = {"template1", "template2", "template3"};
UserOutputManager::list(MessageType::INFO, templates);

// With custom prefix
UserOutputManager::list(MessageType::INFO, templates, "  • ");

// With header
UserOutputManager::list(MessageType::INFO, templates, "  - ", "Available templates:");
```

### Interactive Prompts

Use `UserOutputManager::prompt()` for all interactive input:

```cpp
// Simple prompt
std::string name = UserOutputManager::prompt("Enter template name: ");

// Yes/no confirmation
std::string response = UserOutputManager::prompt("Do you want to save? (y/n): ");
if (response == "y" || response == "Y") {
    // proceed
}
```

**Why**: The `prompt()` helper ensures proper synchronization between `std::cout` and `std::cin`, preventing prompt display issues.

## Debug Logging Standards

Use the Logger for debug, diagnostic, and trace messages:

```cpp
Logger::getInstance().log(LogLevel::DEBUG, "Processing template: ", template_name);
Logger::getInstance().log(LogLevel::ERROR, "Failed to parse file: ", error_message);
```

### Log Levels

- `DEBUG`: Detailed diagnostic information
- `INFO`: General informational messages
- `NORMAL`: Standard operational messages
- `ERROR`: Error conditions (recoverable)
- `CRITICAL`: Critical errors (unrecoverable)

### Punctuation in Logs

Debug logs follow the same punctuation rules as user messages:
- Complete sentences end with a period
- Fragments and labels have no terminal punctuation

## Examples

### Good Examples

```cpp
// User-facing success message
UserOutputManager::success("File processed successfully.");

// User-facing info with label
UserOutputManager::info("Templates directory: ", dir_path);

// List output
UserOutputManager::list(MessageType::INFO, errors, "  - ", "Validation errors:");

// Interactive prompt
std::string response = UserOutputManager::prompt("Continue? (y/n): ");

// Debug logging
Logger::getInstance().log(LogLevel::DEBUG, "Parsed ", token_count, " tokens in ", elapsed_ms, "ms");
```

### Bad Examples

```cpp
// ❌ Fragment with period
UserOutputManager::info("Template: ", template_name, ".");

// ❌ Complete sentence without period
UserOutputManager::success("File processed successfully")

// ❌ Direct std::cout for interactive prompt (breaks synchronization)
std::cout << "Enter name: ";
std::string name;
std::getline(std::cin, name);

// ❌ Mixing user output and logging
std::cout << "Processing..." << std::endl;  // Should use UserOutputManager
Logger::getInstance().log(LogLevel::INFO, "Processing file: ", filename);  // Should use UserOutputManager for user-facing messages
```

## Migration Guide

When updating existing code:

1. **Replace interactive prompts**: Convert `std::cout` + `std::getline` to `UserOutputManager::prompt()`
2. **Replace list output**: Use `UserOutputManager::list()` for consistent formatting
3. **Check punctuation**: Verify complete sentences end with a period, fragments don't
4. **Separate concerns**: User-facing → UserOutputManager, Debug → Logger

## Testing

When writing tests:

```cpp
// Capture user output with callback
std::vector<std::string> captured_messages;
UserOutputManager::initialize_with_callback(
    [&](MessageType type, const char* msg) {
        captured_messages.push_back(msg);
    }
);

// Use NullUserOutput to suppress output
UserOutputManager::initialize_null();

// Use temporary output for scoped changes
{
    auto temp_output = std::make_unique<NullUserOutput>();
    TemporaryUserOutput temp(std::move(temp_output));
    // UserOutput suppressed in this scope
}
```

## Summary

- **User output**: `UserOutputManager` for user-facing messages
- **Debug logging**: `Logger` for diagnostics and trace information
- **Interactive prompts**: `UserOutputManager::prompt()` for input
- **Lists**: `UserOutputManager::list()` for consistent formatting
- **Punctuation**: Complete sentences get periods, fragments don't
- **Separation**: Keep user output and debug logging separate

---
**Last Updated**: 2025-10-07
