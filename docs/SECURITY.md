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
// API keys are stored in a SecureString (mlock'd, zeroed on destruction).
SecureString stored_key{"sk-..."};
std::string masked = stored_key.masked(); // e.g. "sk-...abc"
```

> **Caveat (known limitation).** `Config::get_api_key()` returns a `std::string`, and
> `SecureString::data()` copies the secret into an ordinary (unlocked, non-zeroed) buffer, so
> the key value leaves secure storage as soon as it is used to build a request header. Treat
> the "memory-locked / zeroed" property as covering the *storage object's* lifetime only, not
> the key's whole lifetime in the process. Ending this copy is tracked as hardening work.

#### Network Security
- **HTTPS Enforcement**: The HTTP client restricts both requests and redirects to HTTPS
  (`CURLOPT_PROTOCOLS`/`CURLOPT_REDIR_PROTOCOLS` = `CURLPROTO_HTTPS`); a non-HTTPS URL —
  including a misconfigured provider `base_url` — is refused before any connection is made
- **Timeout Configuration**: User-configurable timeouts prevent hanging connections
- **Retry Logic**: Exponential backoff with jitter prevents API abuse
- **Circuit Breaker**: Automatic failure detection and recovery

#### Request Security
- **Request Validation**: Request bodies are serialized with `nlohmann::json`, which escapes
  content correctly
- **Header handling**: header values are passed to libcurl as-is; **no CRLF sanitization is
  performed today**. This is safe only because header values are not built from untrusted
  input. (An earlier "headers are sanitized to prevent injection" claim did not match the code.)
- **Size limits**: `InputValidator` defines `MAX_QUERY_LENGTH`/`MAX_RESPONSE_SIZE`, but the
  response-size limit is **not yet enforced inside the HTTP read callback** — a large response
  is fully buffered before any check. Enforcing it at the callback is tracked as future work.

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
# API key for the default (Anthropic) provider; must be 30+ characters.
# Without it, LOCAL_ONLY compilation still works.
export CQL_API_KEY="your-anthropic-api-key-here"
```

> **Accuracy note.** Earlier revisions of this guide documented `CQL_MAX_REQUEST_SIZE`,
> `CQL_TIMEOUT_SECONDS`, and `CQL_ENABLE_VALIDATION` environment variables and a JSON
> `"security"` configuration block. **None of these are read by the code today** — setting
> them has no effect. Request/response and input size limits are currently compile-time
> constants in `InputValidator` (`MAX_QUERY_LENGTH`, `MAX_RESPONSE_SIZE`,
> `MAX_DIRECTIVE_LENGTH`, …). Wiring these to runtime configuration is future work.

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
