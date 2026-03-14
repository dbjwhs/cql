# CLAUDE.md - CQL (Claude Query Language)

A structured prompt compiler for LLM interactions. Takes `.llm` files with directives (`@context`, `@language`, `@test`, etc.) and compiles them into formatted prompts. Optionally submits to Claude API for code generation.

## Build & Run

```bash
mkdir -p build && cd build && cmake .. && make
./cql_test                                    # Run all tests
./cql input.llm output.txt                    # Compile a prompt
./cql --submit input.llm --output-dir ./out   # Submit to Claude API
./cql --interactive                           # Interactive mode
```

### API Key Setup
```bash
cp .env.example .env
# Add: CQL_API_KEY=your-anthropic-api-key-here
# Key must be 30+ characters. Without it, LOCAL_ONLY mode works fine.
```

### Logging
- **Default**: File logging to `cql.log` (no console clutter)
- `--log-console` — also log to console (defaults to INFO)
- `--debug-level LEVEL` — set file log level (DEBUG|INFO|NORMAL|ERROR|CRITICAL)
- `--console-level LEVEL` / `--file-level LEVEL` — independent level control
- `--log-file PATH` — custom log file (path validated for security)
- `--log-max-size BYTES` / `--log-max-files N` — log rotation
- `--log-timestamp FORMAT` — iso8601, iso8601-local, simple, epoch, none

## Coding Standards

- **C++20 minimum** — concepts, ranges, string_view, constexpr
- **RAII** — strict resource management, smart pointers over raw
- **Naming**: Classes `CamelCase`, functions `snake_case`, constants `UPPER_SNAKE_CASE`, members `m_` prefix, headers `.hpp`
- **Testing**: 85%+ coverage target, GoogleTest framework
- **Security**: All inputs validated, `SecureString` for secrets, `InputValidator::resolve_path_securely()` for paths, `JsonUtils` for JSON

### Prohibited Practices
- Never commit commented-out code — use git history
- No dead code or disabled functionality
- No debugging artifacts in commits
- No bypassing security validations

## Testing

```bash
# Always build before testing
cd build && cmake .. && make && ./cql_test

# Specific suites
./cql_test --gtest_filter="CQLTest.*"
./cql_test --gtest_filter="SecurityTest.*"

# CI mode (skip external HTTP tests)
CQL_SKIP_EXTERNAL_TESTS=1 ./cql_test --gtest_filter="*HttpClient*"
```

Test files: `src/cql/tests.cpp`. All features need comprehensive tests. Security features need dedicated coverage.

## AI Assistant Rules

1. **Build before test** — always, no exceptions
2. **Fix root causes** — never disable failing code or bypass validation
3. **Security first** — validate inputs, protect paths, secure secrets
4. **Follow existing patterns** — study the codebase before changing it
5. **Modern C++20** — use language features appropriately

## Key Patterns

```cpp
// Logging
Logger::getInstance().log(LogLevel::INFO, "message", var);

// Input validation
InputValidator::validate_template_name(name);
auto path = InputValidator::resolve_path_securely(user_path);

// Secure strings
SecureString api_key("key");  // Auto-locked, zeroed on destruction

// JSON
auto req = JsonUtils::create_api_request(model, query, max_tokens, temp);
```

See [CHANGELOG.md](CHANGELOG.md) for development history.
