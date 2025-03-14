# LLM Project Guide

## Build Commands
- Build project: `mkdir -p build && cd build && cmake .. && make`
- Run all tests: `build/cql --test`
- Run specific test: `build/cql --test "Test Name"` (e.g., `build/cql --test "Template Validator"`)
- Run examples: `build/cql --examples`
- Interactive mode: `build/cql --interactive`
- Process a file: `build/cql input.llm output.txt`
- API integration: `build/cql --submit input.llm --output-dir ./output`

## Code Style Guidelines
- Modern C++20 features preferred (concepts, ranges, string_view)
- Use RAII principles for resource management
- Prefer smart pointers to raw pointers
- Classes use CamelCase, methods/variables use snake_case
- Private member variables prefixed with `m_`
- Include `[[nodiscard]]` for functions that return values
- Use `std::string_view` for string parameters not requiring ownership
- Always implement the Visitor pattern for new node types
- Document all classes and methods with Doxygen-style comments
- Use `const` for methods/parameters that don't modify state
- Use Logger::getInstance().log(LogLevel::INFO, "message", var) for logging
- Handle exceptions appropriately with try/catch blocks
- Implement thorough unit tests for new features

## Current Development Focus
- API Integration with Anthropic's Claude API
- Code generation from Claude responses
- Command-line interface enhancements
- Response parsing and file organization

## Testing
- Run `build/cql --test` to execute all test cases
- All new features should have comprehensive tests
- Test files are located in src/cql/tests.cpp
- Use TEST_ASSERT macro for assertions with meaningful messages

## Development Workflow
- Always build the project after making changes to ensure it compiles successfully
- Run tests after significant changes to verify functionality
- Before submitting changes, run the full test suite
