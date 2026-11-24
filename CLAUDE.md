# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

CQL (Claude Query Language) is a structured prompt compiler written in C++20. It takes `.llm` files made of `@directive` lines (`@language`, `@description`, `@context`, `@test`, `@provider`, ...), compiles them into a formatted prompt, and with `--submit` sends that prompt to Anthropic or OpenAI and writes the generated code out as files.

## Build, test, run

Requires CMake 3.30+ and libcurl. GoogleTest and nlohmann/json are fetched at configure time.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel

cd build
CQL_SKIP_EXTERNAL_TESTS=1 ./cql_test                       # full suite, offline (what CI runs)
./cql_test --gtest_filter="SecurityTest.*"                 # one suite
./cql_test --gtest_filter="CQLTest.BasicCompilation"       # one test
CQL_SKIP_EXTERNAL_TESTS=1 ctest --output-on-failure        # CI's invocation

./cql input.llm output.txt                                 # compile a prompt
./cql --submit input.llm --output-dir ./out                # submit (Anthropic by default)
./cql --submit input.llm --provider openai
./cql --optimize input.llm --mode LOCAL_ONLY               # meta-prompt compiler
./cql --interactive
```

- Always rebuild before running tests.
- Without `CQL_SKIP_EXTERNAL_TESTS=1`, `HttpClientTest`/`MockHttpClientTest` call httpbin.org. `LiveAnthropicIntegrationTest` needs a real key in `ANTHROPIC_API_KEY` or `CQL_API_KEY`.
- Sanitizer build (a separate CI job): `cmake -S . -B build -DENABLE_SANITIZERS=ON`, then run tests with `ASAN_OPTIONS=detect_leaks=1`.
- API keys go in `.env` (copy `.env.example`): `CQL_API_KEY` / `ANTHROPIC_API_KEY` (same value, 30+ characters) and `OPENAI_API_KEY`. Compiling prompts and `--mode LOCAL_ONLY` need no key.
- The pre-commit hook rejects files without a trailing newline. Install it with `./scripts/install_git_hooks.sh`; fix with `python3 scripts/check_eof_newline.py --fix`.
- By default the binary logs to `cql.log` in the working directory, not the console. `--log-console`, `--debug-level`, `--console-level`, `--file-level`, `--log-file` change that.

## Build system facts that bite

- **Source files are listed by hand** in `CMakeLists.txt` (`CQL_CORE_SOURCES`, `CQL_MAIN_SOURCES`, `CQL_TEST_SOURCES`, `MCP_SOURCES`). A new `.cpp` is not built or tested until it is added to the right list.
- **Warnings are errors** (`-Wall -Wextra -Wpedantic -Werror`) on both GCC and AppleClang. CI builds Linux/GCC and macOS/AppleClang.
- **The precompiled header (`include/cql/pch.hpp`) is disabled under GCC.** Code that compiles on macOS only because the PCH pulled in a standard header fails on Linux CI, so every file must include what it uses.
- **Core sources compile twice**: `cql_lib` (for `cql` and `cql-mcp`) and `cql_lib_test` with `CQL_TESTING=1` (for `cql_test`).
- `lib/ailib` sources are compiled straight into those object libraries by the top-level `CMakeLists.txt`; `lib/ailib/CMakeLists.txt` is not part of this build.
- `.gitignore` ignores `lib/` except `lib/ailib/`, and any `build/` directory.

## Architecture

```
.llm file → Lexer → Parser → AST (nodes) → QueryValidator → QueryCompiler (visitor) → prompt text
                                                                    │ --submit
                                                     ApiClient → ailib ProviderFactory → Anthropic | OpenAI
                                                                    ↓
                                                     ResponseProcessor → files in --output-dir
```

**Entry and dispatch.** `main.cpp` only calls `ApplicationController::run`, which parses flags with `CommandLineHandler`, sets up logging, and dispatches to a handler: plain file compilation (`QueryProcessor` in `cql.hpp`), `template_operations.cpp`, `documentation_handler.cpp`, `meta_prompt_handler.cpp` (`--optimize`), `api_client.cpp` (`--submit`), or the interactive REPL in `cli.cpp`.

**Compiler pipeline** (`src/cql/`, headers in `include/cql/`). Hand-written lexer and recursive-descent parser. Each directive is a `QueryNode` subclass in `nodes.hpp`; `QueryCompiler` is a `QueryVisitor` that turns the AST into prompt text. The parser uses panic-mode recovery: on an error it records it and skips to the next `@`, so all errors are reported in one pass.

**Adding or changing a directive touches several places that must agree**: the `TokenType` and keyword table (`lexer.hpp/.cpp`), the node class (`nodes.hpp/.cpp`), the visitor interface (`visitor.hpp`), the parser, the compiler's `visit` method, the valid-directive set in `template_validator.cpp`, and `list_directives` in `src/mcp/mcp_tools.cpp`. Grep an existing one such as `output_format` to find every site. The template validator's list has drifted from the lexer before.

**AILib** (`lib/ailib/`). Provider abstraction used by `--submit` and the meta-prompt compiler: `core/provider.hpp` interface, `providers/factory.hpp` (`ProviderFactory`, fallback chains), Anthropic and OpenAI implementations, a CURL HTTP client with retry/backoff, `SecureString` (`auth/secure_store.hpp`), config loading from env/`.env`, and `JsonUtils`. Provider choice comes from `--provider`, an `@provider` directive, or `CQL_DEFAULT_PROVIDER`.

**Meta-prompt compiler** (`src/cql/meta_prompt/`). `HybridCompiler` optimizes an already-compiled prompt in one of four modes (`LOCAL_ONLY`, `CACHED_LLM`, `FULL_LLM`, `ASYNC_LLM`), combining a local compiler, an LLM-backed `PromptCompiler`, `IntelligentCache`, `ValidationFramework`, `CircuitBreaker`, and `CostController`.

**Templates.** `TemplateManager` stores reusable prompt templates (variables, inheritance, `category/name` paths) under `~/.llm/templates`, not in the repo. `TemplateValidator` checks them.

**MCP server** (`src/mcp/`, binary `cql-mcp`). JSON-RPC server exposing `compile_prompt`, `validate_llm_file`, and `list_directives`. Its sources are also compiled into `cql_test` for `Mcp*Test`.

**Two output channels; keep them separate.**
- *Diagnostics* go to the logger. `Logger::getInstance().log(LogLevel::INFO, "msg", var)` is the historic API; `Logger` and the global `LogLevel` are aliases (in `project_utils.hpp`) for `cql::LoggerBridge` / `cql::HistoricLogLevel`, which forward to the pluggable `LoggerManager`. New code can use the `CQL_LOG_INFO(...)` family from `logger_manager.hpp`. Note that `cql::LogLevel` (in `logger_interface.hpp`) is a different enum from the global `LogLevel` alias.
- *Text meant for the user* goes through `UserOutputManager::info/success/warning/error(...)`, not `std::cout` and not the logger.

**Tests.** GoogleTest, spread across `src/cql/tests.cpp` (core `CQLTest`), `src/cql/test_*.cpp`, `lib/ailib/tests/`, and `src/mcp/tests/`. `MockServer` (`mock_server.hpp`) stands in for the API in integration tests. Some tests write to `./test_output` relative to the working directory.

## Conventions

- C++20, RAII, smart pointers over raw.
- Classes `CamelCase`, functions `snake_case`, constants `UPPER_SNAKE_CASE`, members `m_` prefix, headers `.hpp`.
- Files in `src/cql/` include project headers by relative path (`"../../include/cql/x.hpp"`); third-party headers use angle brackets (`<nlohmann/json.hpp>`) so they resolve through the SYSTEM include path and stay exempt from `-Werror`.
- Return `CQL_NO_ERROR` / `CQL_ERROR` from command handlers. Wrap rethrown errors with `ErrorContextBuilder` (`error_context.hpp`).
- Security-sensitive input goes through existing helpers: `InputValidator::validate_template_name`, `InputValidator::resolve_path_securely` for user-supplied paths, `SecureString` for API keys, `JsonUtils::create_api_request` for request bodies. Do not bypass them. Security changes need tests in `test_security.cpp`.
- No commented-out code, dead code, disabled functionality, or debugging artifacts in commits. Fix a failing test at its root cause; do not disable it.
- New features need tests (85%+ coverage target).

See `CHANGELOG.md` for development history and `docs/` for specifications and user guides.
