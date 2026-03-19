# CQL Changelog

## Development History

### CQL Reactivation (4-Phase Initiative)

**Phase 4: MCP Server**
- New `cql-mcp` binary implementing Model Context Protocol over stdio
- JSON-RPC 2.0 request/response handling
- Tools: `compile_prompt`, `validate_llm_file`, `list_directives`
- 12 MCP tests (JSON-RPC parsing, tool dispatch, lifecycle)

**Phase 3: Streaming Support**
- `--stream` CLI flag for streaming API responses
- ProgressDisplay spinner for non-streaming mode (background thread)
- OpenAI SSE format parsing (`data: {"choices":[{"delta":{"content":"..."}}]}`)

**Phase 2: OpenAI Provider**
- OpenAIProvider implementation (generate, generate_async, generate_stream)
- `@provider` directive in lexer/parser/AST/compiler
- Provider registered in ProviderFactory alongside Anthropic
- `--provider` CLI flag for `--submit` command
- 6 OpenAI provider tests, 3 provider directive tests

**Phase 1: Parser Error Recovery**
- Panic-mode error recovery: parser records errors and continues to next `@` directive
- ErrorReporter class for accumulating ParseDiagnostics
- Replaced 20-line token validation chain with dispatch table lookup
- All errors reported together with line/column/offending token
- 4 error recovery tests

**Phase 0: README Reframe**
- IaC narrative: `.llm` files as HCL, `cql compile` as `terraform plan`
- Multi-provider quick start documentation
- Updated architecture diagram with ProviderFactory
- Removed resolved limitations (single provider, no streaming, first-error-only)

### Logging System Enhancement (5-Phase Initiative)

**Phase 5: Multi-Logger with Independent Level Control**
- LevelFilteredLogger wrapper for independent level filtering
- `--console-level` and `--file-level` CLI flags
- Console defaults to INFO for clean output
- 6 new tests (245 total passing)

**Phase 4: Mixed Output Cleanup** (PR #46)
- Updated all CLI files to use UserOutputManager
- Covered: cli.cpp, meta_prompt_handler.cpp, template_operations.cpp, documentation_handler.cpp

**Phase 3: Enhanced File Logger** (PR #48)
- Log rotation with `--log-max-size` and `--log-max-files`
- Timestamp formats: ISO 8601 (UTC/local), simple, epoch, none
- Comprehensive rotation and timestamp tests

**Phase 2: User Output Separation** (PR #45)
- UserOutput interface with 5 implementations
- Colored console output, MessageType enum
- 20 unit tests, thread-safe manager, Windows compatibility

**Phase 1: File Logging by Default** (PR #44)
- File-only logging by default with `--log-console` flag
- `--log-file PATH` for custom log locations
- Path validation via `InputValidator::resolve_path_securely()`
- 22 unit tests + integration tests

### Test Reliability Fixes (PR #49)
- Fixed file rotation race conditions (flush before close, remove before rename)
- Fixed test cleanup exceptions (error_code overload of filesystem operations)
- Fixed HybridCompiler API key configuration
- Intelligent httpbin.org failure detection
- Replaced std::cout with Logger system for Phase 2 compliance

### HTTP Client CI Reliability (PR #39)
- Split 3 monolithic conditional tests into 8+ focused methods
- Environment variable control: `CQL_SKIP_EXTERNAL_TESTS=1`
- MockHttpClientTest class with 5 offline tests
- test_utils namespace with RetryTestResult and simulate_retry_scenario()

### AILib Integration (Phase 1)
- Unified provider interface across AI providers
- Anthropic Claude API support with streaming
- CURL-based HTTP client with exponential backoff retry
- SecureString class for API key protection
- Factory pattern for dynamic provider creation
- Comprehensive unit and integration tests
