# CQL (Claude Query Language)

Infrastructure-as-code for LLM interactions. Define prompts as `.llm` files with typed directives, compile them into structured prompts, and submit to multiple AI providers.

250+ tests | Zero warnings | C++20

## The Idea

`.llm` files are to LLM prompts what HCL is to infrastructure:
- **`cql compile`** = validate and preview (like `terraform plan`)
- **`cql --submit`** = execute against an API (like `terraform apply`)
- **`@directive` grammar** = enforcement at parse time, not runtime

Directives catch mistakes before they reach the API. A missing `@description` is a parse error, not a wasted API call.

## What It Does

**Input** — a `.llm` file with structured directives:
```
@language "C++"
@description "implement a simple logger class with multiple log levels"

@context "Modern C++20 implementation"
@context "Thread-safe implementation"
@dependency "std::mutex, std::ofstream"

@performance "Low overhead for disabled log levels"
@security "Safe file handling"

@test "Test all log levels"
@test "Test thread safety with concurrent logging"
```

**Output** — a formatted prompt ready for an LLM:
```
Please generate C++ code that:
implement a simple logger class with multiple log levels

Context:
- Modern C++20 implementation
- Thread-safe implementation

Dependencies:
- std::mutex, std::ofstream

Performance Requirements:
- Low overhead for disabled log levels

Security Requirements:
- Safe file handling

Please include tests for the following cases:
- Test all log levels
- Test thread safety with concurrent logging
```

With `--submit`, the compiled prompt goes directly to an AI provider API and the generated code is organized into files.

## Quick Start

```bash
git clone <repository-url> && cd cql
mkdir -p build && cd build && cmake .. && make

# Compile a prompt
./cql ../examples/api_basic_example.llm output.txt

# Submit to Claude API (requires API key in .env)
./cql --submit ../examples/api_basic_example.llm --output-dir ./generated

# Interactive mode
./cql --interactive
```

### Multi-Provider Quick Start

```bash
# Set up API keys in .env
cp .env.example .env
# Edit .env with your keys

# Submit to Anthropic (default)
./cql --submit input.llm --output-dir ./out

# Submit to OpenAI
./cql --submit input.llm --output-dir ./out --provider openai

# Or specify provider in the .llm file itself
# @provider "openai"
```

## Architecture

```
.llm file → Lexer → Parser → AST → Validator → Compiler → Formatted Prompt
                                                                ↓ (--submit)
                                                      ProviderFactory
                                                       ↓           ↓
                                                  Anthropic     OpenAI
                                                       ↓           ↓
                                                     Response Processor
                                                            ↓
                                                     Extracted Files
```

The compiler pipeline is a hand-written lexer/parser (no generator tools). The AST supports 20+ directive types including `@context`, `@language`, `@test`, `@security`, `@performance`, `@dependency`, `@architecture`, `@provider`, and more.

**Error recovery:** The parser uses panic-mode recovery — when it hits an error, it records it and skips to the next `@` directive. All errors are reported together, so you fix everything in one pass instead of playing whack-a-mole.

## Key Components

| Component | What It Does |
|-----------|-------------|
| **CQL Compiler** | Lexer → parser → AST → validator → prompt formatter |
| **AILib** | C++ AI provider library (Anthropic Claude, OpenAI, extensible) |
| **ProviderFactory** | Unified provider creation with fallback chains |
| **HTTP Client** | CURL-based with exponential backoff retry logic |
| **SecureString** | Memory-locked API key storage, zeroed on destruction |
| **Logging System** | File + console with independent levels, rotation, timestamps |
| **Template System** | Reusable prompt templates with variables and inheritance |
| **MCP Server** | Model Context Protocol server for IDE integration |

## Limitations

Being honest about what this is and isn't:

- **~20 directive types** — no conditionals, no loops, no macros in the language itself
- **Generated examples are scaffolding** — the 25 example outputs are structured starting points (headers, class declarations), not complete implementations

The value proposition is structured prompt engineering with type safety and repeatability, not a general-purpose programming language.

## Project Structure

```
cql/
├── src/cql/            # Core compiler (lexer, parser, AST, validator, compiler)
├── src/mcp/            # MCP server implementation
├── include/cql/        # Public headers
├── lib/ailib/          # AI provider library (Anthropic, OpenAI)
│   ├── include/ailib/  # Provider interfaces, HTTP client, auth
│   ├── src/            # Implementation
│   └── tests/          # AILib-specific tests
├── examples/           # 25 sample .llm files
├── docs/               # Technical specifications and guides
└── scripts/            # Build and utility scripts
```

## License

MIT License — see [LICENSE](LICENSE) for details.
