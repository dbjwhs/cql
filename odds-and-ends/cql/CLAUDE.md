# CQL Project Guide

## Build Commands
- Build project: `mkdir -p build && cd build && cmake .. && make`
- Run tests: `build/cql --test`
- Run examples: `build/cql --examples`
- Interactive mode: `build/cql --interactive`
- Process a file: `build/cql input.cql output.txt`

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
- Implement thorough unit tests for new features

## TODO Items
1. **Validation System Improvements**:
   - Fix the language directive validation test in `test_validation_requirements()`
   - The parser currently raises syntax errors before validation can run
   - Finalize changes to `include/cql/compiler.hpp`, `include/cql/cql.hpp`, and `src/cql/validator.cpp`
   - Consider separating parsing errors from validation errors more cleanly

2. **Future Enhancements**:
   - Improve error reporting with line/column information
   - Consider adding a `strict` mode to catch more issues
   - Add more comprehensive validation test cases
   - Explore adding a linting capability to suggest improvements
