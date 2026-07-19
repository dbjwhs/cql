# CQL Changelog

## Development History

### Hardening & CI Pass

**Build: third-party headers no longer break `-Werror`**
- GoogleTest and nlohmann/json are now treated as SYSTEM dependencies, and GoogleTest's
  own targets are exempted from our warning flags. A strict `-Wall -Wextra -Wpedantic
  -Werror` build no longer fails when a newer compiler adds warnings inside third-party
  code (observed on AppleClang 21: `-Wcharacter-conversion` in `gtest-printers.h` and
  `-Wdeprecated-literal-operator` in nlohmann/json 3.11.2). Our own code keeps the full
  warning bar.
- `compiler.hpp` and `tests.cpp` now include nlohmann/json via `<nlohmann/json.hpp>`
  (resolved through the SYSTEM include path) instead of a relative quoted path that
  bypassed `-isystem`.
- Net effect: a clean `cmake .. && make` is green again on AppleClang 21; the full suite
  (274 run / 262 pass / 12 external-skipped / 1 disabled) builds warning-clean.

**Tests: all network-dependent tests honor `CQL_SKIP_EXTERNAL_TESTS`**
- Eleven `HttpClientTest`/`MockHttpClientTest` cases that call httpbin.org (or expect a
  live external response) previously ignored the `CQL_SKIP_EXTERNAL_TESTS` escape hatch
  and only skipped on an HTTP 503, so "CI mode" still made ~11 live network calls. They
  now skip uniformly when `CQL_SKIP_EXTERNAL_TESTS` is set, matching the five cases that
  already did. `CQL_SKIP_EXTERNAL_TESTS=1 ./cql_test` is now deterministic and offline.

**CI: build + test workflow (GitHub Actions)**
- New `.github/workflows/ci.yml` builds on Linux (GCC) and macOS (AppleClang) with the full
  `-Werror` warning set and runs the suite via `ctest` in offline mode on every push to
  `main`/`hardening/**` and every PR. The project previously had no build/test CI, so the
  suite was a local-only gate; this makes "green build + passing tests" enforceable. (The
  macOS leg matches local verification; the Linux/GCC leg runs for the first time on push.)

**Fix: template directive validation inspects every line and the full directive set**
- `TemplateValidator` directive extraction used a non-multiline `^(@...)` regex, so only the
  first line's directive was ever seen: invalid directives after line 1 went undetected and a
  `@description` below the first line was falsely reported "missing". It now matches a
  directive at the start of any line.
- The validator's valid-directive set was missing nine directives the lexer accepts
  (`@performance`, `@model`, `@format`, `@provider`, `@output_format`, `@max_tokens`,
  `@temperature`, `@pattern`, `@structure`), which the extraction bug had masked. Completed it
  (kept in sync with `Lexer::lex_keyword`) so valid templates are no longer flagged invalid.
- New `src/cql/test_template_validator.cpp` (8 tests) locks in multi-line extraction, the
  essential-`@description` check, invalid-directive detection, the completed directive set,
  and declared/referenced-variable consistency.

**Test: cover TemplateValidatorSchema (previously untested)**
- Add a `TemplateValidatorSchemaTest` suite (5 tests) for the directive-schema registry used
  by the CLI's template tooling: default-schema contents, directive lookup, required
  directives, custom directive registration, and validation-rule round-tripping.

**Test: real coverage for ResponseProcessor**
- Add `src/cql/test_response_processor.cpp` (6 tests). `process_response` code-block
  extraction is exercised through the public API (language detection, explicit `filename:`
  hints, class-name-derived filenames, test-block detection), and `save_generated_file`
  writes into a per-test temp directory, including the no-overwrite `.new` fallback. The only
  prior direct test for this module was a `pass()` stub.

**Test: cover InputValidator size/limit and template-name methods**
- Add `SecurityTest` cases for four previously-untested public methods: `validate_query_length`
  (accepts normal, rejects one past `MAX_QUERY_LENGTH`), `validate_response_size` (accepts
  normal), `validate_template_name` (accepts names and category paths; rejects empty, invalid
  characters, and `..` traversal), and `sanitize_template_variables` (replaces `${...}`).

**Build/CI: optional AddressSanitizer + UndefinedBehaviorSanitizer**
- New `-DENABLE_SANITIZERS=ON` CMake option builds with `-fsanitize=address,undefined`. A CI
  `sanitizers` job builds with it and runs the full suite under ASan/UBSan (with LeakSanitizer
  on Linux) so memory errors and undefined behavior fail the build. Off by default; the normal
  build is unaffected.

**Test: remove pass()-stub "mirage" gtest cases**
- Remove nine `CQLTest` GoogleTest cases (TemplateInheritance, TemplateValidator, APIClient,
  ResponseProcessor, ExamplesCompilation, LexerStandalone, ParserStandalone,
  CompilerStandalone, JSONFormatOutput) that delegated to `test_*()` functions returning
  `TestResult::pass()` and asserted nothing, implying coverage that did not exist.
  TemplateValidator and ResponseProcessor now have real coverage (added above); lexer/parser/
  compiler are covered end-to-end by the `QueryProcessor::compile` tests. ApiClient's real
  HTTP path, JSON output formatting, template inheritance, and example compilation remain as
  honestly-uncovered gaps. The `pass()` stub functions are retained for now because the legacy
  `cql --test` CLI harness still lists them; that harness is a separate mirage tracked for
  follow-up.

**Test: enable the previously-disabled --optimize integration test**
- `MetaPromptCLITest.DISABLED_HandleOptimizeCommandIntegration` was disabled and, when run,
  failed: it asserted on output captured via `UserOutputManager`, but the optimizer prints its
  results to `std::cout`, so the marker was never captured. Rewrite it as a hermetic test that
  writes a self-contained `.llm` to a temp path and asserts the `--optimize` LOCAL_ONLY pipeline
  returns `CQL_NO_ERROR` end-to-end, and enable it (drop the `DISABLED_` prefix). The suite now
  has zero disabled tests.

**Security: HTTP client enforces HTTPS for requests and redirects**
- The ailib CURL client set `CURLOPT_FOLLOWLOCATION` but no protocol restriction, so a
  plaintext `http://` provider `base_url` (from a config file) or a 3xx redirect to another
  scheme/host would send the API-key header in cleartext or to an unexpected host — a
  regression from the legacy `api_client.cpp`, which already restricted to HTTPS. Add
  `CURLOPT_PROTOCOLS`/`CURLOPT_REDIR_PROTOCOLS = CURLPROTO_HTTPS`; non-HTTPS URLs are now
  refused before any connection. New offline test `HttpClientTest.RejectsNonHttpsUrl`.

**Docs: correct SECURITY.md over-claims**
- Remove the `CQL_MAX_REQUEST_SIZE`/`CQL_TIMEOUT_SECONDS`/`CQL_ENABLE_VALIDATION` environment
  variables and the JSON `"security"` configuration block from the guide — none are read by the
  code, so the documented settings had no effect. Correct the "header sanitization" claim (not
  implemented), the response-size-limit claim (constants exist but aren't enforced at the HTTP
  read callback), and the `SecureString` example (the key leaves secure storage on use). The
  guide now describes what the code does, not what it aspires to.

**Build: libcurl 7.85+ deprecation broke the GCC -Werror build (caught by the new CI)**
- `CURLOPT_PROTOCOLS`/`CURLOPT_REDIR_PROTOCOLS` (used by the HTTPS-enforcement fix and,
  latently, by the legacy `api_client.cpp`) were deprecated in libcurl 7.85 in favour of the
  `_STR` string forms. On Ubuntu's newer libcurl this failed `-Werror=deprecated-declarations`
  on the first CI run (AppleClang's headers don't flag it, so macOS and local builds passed).
  Guard with `CURL_AT_LEAST_VERSION(7,85,0)`: use `CURLOPT_PROTOCOLS_STR`/
  `CURLOPT_REDIR_PROTOCOLS_STR` where available, fall back to the enum on older libcurl.
- Give `CompilerFlags::custom_timeout` a default member initializer so designated initializers
  that omit it no longer trip GCC's `-Werror=missing-field-initializers` (Clang does not warn).
- Disable the reused precompiled header under GCC (`CMAKE_DISABLE_PRECOMPILE_HEADERS` when the
  compiler is GNU): GCC on Ubuntu rejects it with a `-fpie` mismatch (`-Werror=invalid-pch`)
  because object libraries build `-fPIC` while the executables that reuse the PCH build `-fPIE`.
  The PCH is a build-speed optimization only and stays enabled for Clang/AppleClang. These were
  latent — the code had never been built under GCC `-Werror` before CI existed.
- Add the standard-library includes four sources relied on the precompiled header to provide:
  `config.cpp` (`<set>`), `logger_adapters.cpp` (`<iostream>`, `<filesystem>`),
  `logger_interface.cpp` (`<thread>`), and `nodes.cpp` (`<algorithm>`). With the PCH disabled on
  GCC these failed to compile. Verified with a local GCC 16 `-Wall -Wextra -Wpedantic -Werror`
  sweep over every source (core, main, MCP, and tests): clean.

**Fix: HTTP client leaked the request header list (found by CI LeakSanitizer)**
- `CurlClient::configure_curl` built the `curl_slist` of request headers, set it on the handle,
  and returned without freeing it; `send()`/`send_stream()`'s `CurlCleanup` freed its own
  (always-null) member instead, so every request leaked the header strings (which include the
  API key) — 64 bytes direct + 128 indirect per call. Hand the list back through a
  `curl_slist*&` out-parameter so the caller's `CurlCleanup` frees it. Caught by the new
  sanitizer CI job's LeakSanitizer (Linux-only; macOS AddressSanitizer has no LSan).

**Fix: CommandLineHandler leaked its copied argv buffers (found by CI LeakSanitizer)**
- `copy_arguments` duplicated each argument with raw `new char[]` and stored the pointers in a
  `unique_ptr<char*[]>` that owns only the pointer array, not the buffers — so the argument
  copies leaked (and `find_and_remove_*` dropped pointers to some without freeing them). Own the
  buffers in a `vector<unique_ptr<char[]>>`; the pointer array just indexes into it.

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
