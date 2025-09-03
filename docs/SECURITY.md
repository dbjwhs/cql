# CQL Security Guide

This document outlines the comprehensive security measures implemented in the CQL (Claude Query Language) project and provides guidelines for secure development and deployment.

## Overview

CQL implements enterprise-grade security measures throughout the entire system, from input validation to API communication, following security-first principles and defense-in-depth strategies.

## Security Features

### 🔒 Input Validation & Sanitization

#### File Path Security
- **Path Traversal Prevention**: All file paths are validated to prevent directory traversal attacks
- **Secure Path Resolution**: `InputValidator::resolve_path_securely()` prevents access to system files
- **Whitelist-based Validation**: Only approved characters and patterns are allowed in file paths

```cpp
// Secure file path handling
std::string secure_path = InputValidator::resolve_path_securely(user_input);
```

#### Parameter Validation
- **Compilation Mode Validation**: Strict whitelisting of allowed compilation modes
- **Optimization Goal Validation**: Validated against predefined safe values
- **Domain Parameter Sanitization**: Alphanumeric characters, underscores, and hyphens only
- **Length Limits**: All inputs have defined maximum lengths to prevent buffer overflows

#### Content Validation
- **Query Length Limits**: Prevents memory exhaustion attacks
- **Response Size Validation**: Limits response sizes to prevent DoS attacks
- **Null Byte Injection Prevention**: Detects and blocks null byte attacks
- **Control Character Filtering**: Removes dangerous control characters from input

### 🛡️ API Security

#### Secure Authentication
- **SecureString Class**: API keys are stored in locked memory and automatically zeroed
- **Environment Variable Loading**: Secure loading of credentials from environment
- **Key Masking**: API keys are masked in logs and error messages

```cpp
// Secure API key handling
SecureString api_key = config.get_api_key("anthropic");
std::string masked = api_key.masked(); // Shows only "sk-...abc" format
```

#### Network Security
- **HTTPS Enforcement**: All API communications use HTTPS only
- **Timeout Configuration**: User-configurable timeouts prevent hanging connections
- **Retry Logic**: Exponential backoff with jitter prevents API abuse
- **Circuit Breaker**: Automatic failure detection and recovery

#### Request Security
- **Request Validation**: All API requests are validated before transmission
- **Header Sanitization**: HTTP headers are sanitized to prevent injection
- **Body Size Limits**: Request bodies are limited to prevent resource exhaustion

### 🚫 Injection Attack Prevention

#### Command Injection
- **Array-based Execution**: Shell commands use secure array expansion instead of `eval`
- **Parameter Whitelisting**: Only approved parameters are passed to shell commands
- **Input Sanitization**: All shell inputs are validated and sanitized

```bash
# Secure command execution (from demo scripts)
local cmd_array=(
    "$CQL_EXECUTABLE"
    "--optimize"
    "$input_file"
    "--mode"
    "$mode"
)
"${cmd_array[@]}"  # Safe array expansion
```

#### SQL Injection Prevention
- **Content Validation**: SQL injection patterns are detected and blocked
- **Safe Character Validation**: Only approved characters are allowed in user input
- **Parameterized Queries**: When applicable, parameterized queries are used

#### Shell Injection Prevention
- **Shell Safety Validation**: `InputValidator::is_shell_safe()` blocks dangerous patterns
- **Character Whitelisting**: Dangerous shell metacharacters are blocked
- **Command Pattern Detection**: Common attack patterns are identified and blocked

### 🔐 Memory Security

#### Secure Memory Management
- **RAII Patterns**: Automatic resource cleanup prevents memory leaks
- **Smart Pointers**: Prevents use-after-free and double-free vulnerabilities
- **SecureString**: Sensitive data is stored in locked, automatically-cleared memory

#### Resource Management
- **Automatic Cleanup**: `ResourceCleanup` class provides RAII-style resource management
- **Temporary File Security**: Temporary files are automatically cleaned up
- **Memory Limits**: Input size limits prevent memory exhaustion attacks

### 📝 Logging Security

#### Information Disclosure Prevention
- **Log Sanitization**: All logged data is sanitized to prevent information leakage
- **API Key Masking**: Sensitive data is masked in log outputs
- **Error Message Sanitization**: Error messages don't reveal system internals

```cpp
// Secure logging
std::string safe_input = InputValidator::sanitize_for_logging(user_input);
Logger::getInstance().log(LogLevel::INFO, "Processing: ", safe_input);
```

## Security Configuration

### Environment Variables

```bash
# Required for API access
export ANTHROPIC_API_KEY="your-api-key-here"

# Optional security settings
export CQL_MAX_REQUEST_SIZE="10485760"  # 10MB limit
export CQL_TIMEOUT_SECONDS="120"       # 2 minute timeout
export CQL_ENABLE_VALIDATION="true"    # Enable semantic validation
```

### Configuration File Security

```json
{
  "security": {
    "max_request_size": 10485760,
    "timeout_seconds": 120,
    "enable_path_validation": true,
    "log_level": "INFO"
  }
}
```

## Secure Development Guidelines

### Input Validation Rules

1. **Validate All Inputs**: Every user input must be validated before processing
2. **Use Whitelists**: Prefer whitelisting over blacklisting for input validation
3. **Sanitize for Context**: Sanitize data appropriately for its intended use
4. **Check Lengths**: Always validate input lengths against defined limits

### API Security Best Practices

1. **Use SecureString**: Store all API keys and sensitive data in SecureString objects
2. **Validate Responses**: Always validate API responses before processing
3. **Implement Timeouts**: Set appropriate timeouts for all network operations
4. **Use Circuit Breakers**: Implement circuit breakers for external API calls

### Error Handling Guidelines

1. **Sanitize Error Messages**: Never include sensitive data in error messages
2. **Log Security Events**: Log all security-related events for monitoring
3. **Fail Securely**: Always fail to a secure state when errors occur
4. **Provide Generic Messages**: Give generic error messages to users

## Security Testing

### Automated Security Tests

The project includes comprehensive security tests that validate:

- Input validation effectiveness
- Path traversal prevention
- Injection attack prevention
- Memory security
- API security measures
- Error handling security

### Running Security Tests

```bash
# Run all security tests
./cql_test --gtest_filter="SecurityTest.*"

# Run specific security test categories
./cql_test --gtest_filter="SecurityTest.CommandInjectionPrevention"
./cql_test --gtest_filter="SecurityTest.PathTraversalPrevention"
./cql_test --gtest_filter="SecurityTest.ParameterValidationEnhanced"
```

### Security Test Coverage

- **Command Injection Tests**: Validate shell safety and command execution
- **Path Traversal Tests**: Ensure directory traversal attacks are blocked
- **Input Validation Tests**: Verify all input validation mechanisms
- **Memory Security Tests**: Test for memory leaks and secure memory handling
- **Concurrent Access Tests**: Validate thread safety of security mechanisms

## Vulnerability Reporting

### Reporting Security Issues

If you discover a security vulnerability:

1. **Do NOT** create a public issue
2. Email security concerns directly to the maintainers
3. Provide detailed information about the vulnerability
4. Allow reasonable time for fix before disclosure

### Response Process

1. **Acknowledgment**: Security reports are acknowledged within 24 hours
2. **Assessment**: Vulnerabilities are assessed and prioritized
3. **Mitigation**: Critical issues are addressed immediately
4. **Testing**: All fixes undergo comprehensive security testing
5. **Disclosure**: Coordinated disclosure after fix deployment

## Security Hardening Checklist

### Development Environment

- [ ] Use latest compiler with security warnings enabled
- [ ] Enable address sanitizer during development
- [ ] Run static analysis tools regularly
- [ ] Validate all dependencies for known vulnerabilities

### Deployment Environment

- [ ] Use secure environment variable management
- [ ] Enable comprehensive logging and monitoring
- [ ] Implement network-level security controls
- [ ] Regular security audits and updates

### Code Review Checklist

- [ ] All inputs validated against security policies
- [ ] No hardcoded credentials or sensitive data
- [ ] Proper error handling with secure defaults
- [ ] Memory management follows RAII principles
- [ ] External API calls include proper timeout and validation

## Security Updates

### Keeping Secure

1. **Monitor Dependencies**: Regularly check for dependency updates
2. **Security Patches**: Apply security patches promptly
3. **Update Documentation**: Keep security documentation current
4. **Regular Audits**: Conduct periodic security audits

### Version History

- **v1.0.0**: Initial security implementation
- **v1.1.0**: Enhanced input validation and path security
- **v1.2.0**: Added comprehensive security testing
- **v1.3.0**: Implemented resource cleanup and memory security
- **v1.4.0**: Added configurable timeouts and circuit breakers

## References

- [OWASP Secure Coding Practices](https://owasp.org/www-project-secure-coding-practices-quick-reference-guide/)
- [CWE/SANS Top 25 Most Dangerous Software Errors](https://cwe.mitre.org/top25/)
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)
- [SEI CERT C++ Coding Standard](https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=88046682)

---

**Security is a shared responsibility.** All contributors must understand and follow these security guidelines to maintain the integrity and safety of the CQL system.
