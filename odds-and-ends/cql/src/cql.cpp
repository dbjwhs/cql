// MIT License
// Copyright (c) 2025 dbjwhs

#include <map>
#include <cassert>
#include "../../../headers/project_utils.hpp"

#ifndef CQL_HPP
#define CQL_HPP

// claude query language (cql)
//
// history:
// the claude query language was developed in 2025 as a domain-specific language to formalize
// and standardize how developers craft queries for large language models (llms), specifically
// anthropic's claude. it follows the compiler pattern where a high-level representation (the cql)
// is translated into a more detailed and structured query string.
//
// purpose:
// cql addresses several challenges in prompt engineering:
// 1. consistency - providing a standard structure for technical queries
// 2. efficiency - reducing time spent crafting detailed prompts manually
// 3. quality - ensuring all necessary information is included for optimal code generation
// 4. reusability - allowing query templates to be saved, modified, and reused
//
// common usage patterns:
// 1. specification of technical requirements:
//    @language "c++"
//    @description "implement a thread-safe queue with timeout features"
//
// 2. providing implementation context:
//    @context "designed for a real-time system with strict latency requirements"
//    @context "must be compatible with c++20 features"
//
// 3. test case specifications:
//    @test "empty queue behavior"
//    @test "concurrent push/pop operations"
//    @test "timeout handling for blocked operations"
//
// 4. extending with custom sections:
//    @dependencies "requires boost::asio for async operations"
//    @performance "must handle 10k operations per second with <1ms latency"
//
// 5. copyright and license information:
//    @copyright "mit license" "2025 dbjwhs"

// forward declarations
class QueryNode;
class CodeRequestNode;
class ContextNode;
class TestNode;
class DependencyNode;
class PerformanceNode;
class CopyrightNode;

// visitor pattern for different node types
class QueryVisitor {
public:
    virtual ~QueryVisitor() = default;
    virtual void visit(const CodeRequestNode& node) = 0;
    virtual void visit(const ContextNode& node) = 0;
    virtual void visit(const TestNode& node) = 0;
    virtual void visit(const DependencyNode& node) = 0;
    virtual void visit(const PerformanceNode& node) = 0;
    virtual void visit(const CopyrightNode& node) = 0;
};

// base class for ast nodes using pimpl idiom
class QueryNode {
public:
    virtual ~QueryNode() = default;
    virtual void accept(QueryVisitor& visitor) const = 0;
};

// node for code generation requests
class CodeRequestNode final : public QueryNode {
public:
    explicit CodeRequestNode(std::string language, std::string description)
        : m_language(std::move(language)), m_description(std::move(description)) {}

    void accept(QueryVisitor& visitor) const override {
        visitor.visit(*this);
    }

    [[nodiscard]] const std::string& language() const { return m_language; }
    [[nodiscard]] const std::string& description() const { return m_description; }

private:
    std::string m_language;
    std::string m_description;
};

// node for providing context about the code
class ContextNode final : public QueryNode {
public:
    explicit ContextNode(std::string context)
        : m_context(std::move(context)) {}

    void accept(QueryVisitor& visitor) const override {
        visitor.visit(*this);
    }

    [[nodiscard]] const std::string& context() const { return m_context; }

private:
    std::string m_context;
};

// node for specifying test requirements
class TestNode final : public QueryNode {
public:
    explicit TestNode(std::vector<std::string> test_cases)
        : m_test_cases(std::move(test_cases)) {}

    void accept(QueryVisitor& visitor) const override {
        visitor.visit(*this);
    }

    [[nodiscard]] const std::vector<std::string>& test_cases() const { return m_test_cases; }

private:
    std::vector<std::string> m_test_cases;
};

// node for specifying dependencies
class DependencyNode final : public QueryNode {
public:
    explicit DependencyNode(std::vector<std::string> dependencies)
        : m_dependencies(std::move(dependencies)) {}

    void accept(QueryVisitor& visitor) const override {
        visitor.visit(*this);
    }

    [[nodiscard]] const std::vector<std::string>& dependencies() const { return m_dependencies; }

private:
    std::vector<std::string> m_dependencies;
};

// node for specifying performance requirements
class PerformanceNode final : public QueryNode {
public:
    explicit PerformanceNode(std::string requirement)
        : m_requirement(std::move(requirement)) {}

    void accept(QueryVisitor& visitor) const override {
        visitor.visit(*this);
    }

    [[nodiscard]] const std::string& requirement() const { return m_requirement; }

private:
    std::string m_requirement;
};

// node for specifying copyright and license
class CopyrightNode final : public QueryNode {
public:
    explicit CopyrightNode(std::string license, std::string owner)
        : m_license(std::move(license)), m_owner(std::move(owner)) {}

    void accept(QueryVisitor& visitor) const override {
        visitor.visit(*this);
    }

    [[nodiscard]] const std::string& license() const { return m_license; }
    [[nodiscard]] const std::string& owner() const { return m_owner; }

private:
    std::string m_license;
    std::string m_owner;
};

// query compiler that generates structured prompts
class QueryCompiler final : public QueryVisitor {
public:
    void visit(const CodeRequestNode& node) override {
        m_result_sections["code"] = "Please generate " + node.language() + " code that:\n" +
                                 node.description() + "\n\n";
    }

    void visit(const ContextNode& node) override {
        if (!m_result_sections.contains("context")) {
            m_result_sections["context"] = "Context:\n";
        }
        m_result_sections["context"] += "- " + node.context() + "\n";
    }

    void visit(const TestNode& node) override {
        // store the test cases, we'll format them when getting the final query
        m_test_cases.insert(m_test_cases.end(),
                          node.test_cases().begin(),
                          node.test_cases().end());
    }

    void visit(const DependencyNode& node) override {
        if (!m_result_sections.contains("dependencies")) {
            m_result_sections["dependencies"] = "Dependencies:\n";
        }
        for (const auto& dependency : node.dependencies()) {
            m_result_sections["dependencies"] += "- " + dependency + "\n";
        }
    }

    void visit(const PerformanceNode& node) override {
        if (!m_result_sections.contains("performance")) {
            m_result_sections["performance"] = "Performance Requirements:\n";
        }
        m_result_sections["performance"] += "- " + node.requirement() + "\n";
    }

    void visit(const CopyrightNode& node) override {
        std::string copyright_message = "Please include the following copyright header at the top of all generated files:\n";
        copyright_message += "```\n";
        copyright_message += "// " + node.license() + "\n";
        copyright_message += "// Copyright (c) " + node.owner() + "\n";
        copyright_message += "```\n\n";
        m_result_sections["copyright"] = copyright_message;
    }

    [[nodiscard]] std::string get_compiled_query() const {
        std::string query_string;

        // add a copyright section if it exists
        auto copyright_it = m_result_sections.find("copyright");
        if (copyright_it != m_result_sections.end()) {
            query_string += copyright_it->second;
        }

        // add a code section if it exists
        auto code_it = m_result_sections.find("code");
        if (code_it != m_result_sections.end()) {
            query_string += code_it->second;
        }

        // add a context section if it exists
        auto context_it = m_result_sections.find("context");
        if (context_it != m_result_sections.end()) {
            query_string += context_it->second + "\n";
        }

        // add a dependencies section if it exists
        auto dependencies_it = m_result_sections.find("dependencies");
        if (dependencies_it != m_result_sections.end()) {
            query_string += dependencies_it->second + "\n";
        }

        // add a performance section if it exists
        auto performance_it = m_result_sections.find("performance");
        if (performance_it != m_result_sections.end()) {
            query_string += performance_it->second + "\n";
        }

        // add test cases if we have any
        if (!m_test_cases.empty()) {
            query_string += "Please include tests for the following cases:\n";
            for (const auto& test_case : m_test_cases) {
                query_string += "- " + test_case + "\n";
            }
            query_string += "\n";
        }

        // add a quality assurance section as a standard footer
        query_string += "Quality Assurance Requirements:\n";
        query_string += "- All code must be well-documented with comments\n";
        query_string += "- Follow modern C++ best practices\n";
        query_string += "- Ensure proper error handling\n";
        query_string += "- Optimize for readability and maintainability\n";

        return query_string;
    }

    void print_compiled_query(std::ostream& out = std::cout) const {
        out << "\n=== Compiled Query ===\n\n"
            << get_compiled_query()
            << "===================\n";
    }

private:
    std::map<std::string, std::string> m_result_sections;
    std::vector<std::string> m_test_cases;
};

// token types for our dsl
enum class TokenType {
    LANGUAGE,       // @language
    DESCRIPTION,    // @description
    CONTEXT,        // @context
    TEST,           // @test
    DEPENDENCY,     // @dependency
    PERFORMANCE,    // @performance
    COPYRIGHT,      // @copyright
    IDENTIFIER,     // any text
    STRING,         // "quoted text"
    NEWLINE,        // \n
    END             // end of input
};

// utility function to convert tokentype to string
std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::LANGUAGE: return "LANGUAGE";
        case TokenType::DESCRIPTION: return "DESCRIPTION";
        case TokenType::CONTEXT: return "CONTEXT";
        case TokenType::TEST: return "TEST";
        case TokenType::DEPENDENCY: return "DEPENDENCY";
        case TokenType::PERFORMANCE: return "PERFORMANCE";
        case TokenType::COPYRIGHT: return "COPYRIGHT";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::STRING: return "STRING";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::END: return "END";
        default: return "UNKNOWN";
    }
}

// token structure for lexical analysis
struct Token {
    TokenType m_type;
    std::string m_value;
    size_t m_line;
    size_t m_column;

    Token(TokenType t, std::string v, size_t l, size_t c)
        : m_type(t), m_value(std::move(v)), m_line(l), m_column(c) {}

    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << "Token{type=" << token_type_to_string(m_type)
            << ", value='" << m_value << "'"
            << ", line=" << m_line
            << ", column=" << m_column << "}";
        return oss.str();
    }
};

// lexical analyzer (lexer) for tokenizing input
class Lexer {
public:
    explicit Lexer(std::string_view input)
        : m_input(input), m_current(0), m_line(1), m_column(1) {}

    std::optional<Token> next_token() {
        skip_whitespace();

        if (m_current >= m_input.length()) {
            return std::nullopt;
        }

        if (m_input[m_current] == '@') {
            return lex_keyword();
        }

        if (m_input[m_current] == '"') {
            return lex_string();
        }

        if (m_input[m_current] == '\n') {
            auto token = Token(TokenType::NEWLINE, "\n", m_line, m_column);
            advance();
            m_line++;
            m_column = 1;
            return token;
        }

        return lex_identifier();
    }

private:
    std::string_view m_input;
    size_t m_current;
    size_t m_line;
    size_t m_column;

    // move to the next character in the input
    void advance() {
        m_current++;
        m_column++;
    }

    // skip whitespace characters (except newlines)
    void skip_whitespace() {
        while (m_current < m_input.length() &&
               (std::isspace(m_input[m_current]) && m_input[m_current] != '\n')) {
            advance();
        }
    }

    // parse a keyword token (starting with @)
    std::optional<Token> lex_keyword() {
        advance(); // skip @
        std::string keyword;
        size_t start_column = m_column;

        while (m_current < m_input.length() && std::isalpha(m_input[m_current])) {
            keyword += m_input[m_current];
            advance();
        }

        TokenType type;
        if (keyword == "language") type = TokenType::LANGUAGE;
        else if (keyword == "description") type = TokenType::DESCRIPTION;
        else if (keyword == "context") type = TokenType::CONTEXT;
        else if (keyword == "test") type = TokenType::TEST;
        else if (keyword == "dependency") type = TokenType::DEPENDENCY;
        else if (keyword == "performance") type = TokenType::PERFORMANCE;
        else if (keyword == "copyright") type = TokenType::COPYRIGHT;
        else throw std::runtime_error("Unknown keyword: @" + keyword);

        return Token(type, keyword, m_line, start_column);
    }

    // parse a string token (enclosed in quotes)
    std::optional<Token> lex_string() {
        if (m_current >= m_input.length() || m_input[m_current] != '"') {
            throw std::runtime_error("Expected opening quote at line " +
                std::to_string(m_line) + ", column " + std::to_string(m_column));
        }

        advance(); // skip opening quote
        std::string value;
        const size_t start_column = m_column;

        while (m_current < m_input.length() && m_input[m_current] != '"') {
            if (m_input[m_current] == '\\') {
                advance();
                if (m_current >= m_input.length()) {
                    throw std::runtime_error("Unterminated string escape sequence");
                }
                switch (m_input[m_current]) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case '"': value += '"'; break;
                    case '\\': value += '\\'; break;
                    default: throw std::runtime_error("Invalid escape sequence");
                }
            } else {
                value += m_input[m_current];
            }
            advance();
        }

        if (m_current >= m_input.length()) {
            throw std::runtime_error("Unterminated string");
        }

        advance(); // skip closing quote
        return Token(TokenType::STRING, value, m_line, start_column);
    }

    // parse an identifier token
    std::optional<Token> lex_identifier() {
        std::string value;
        const size_t start_column = m_column;

        while (m_current < m_input.length() &&
               !std::isspace(m_input[m_current]) &&
               m_input[m_current] != '@') {
            value += m_input[m_current];
            advance();
        }

        return Token(TokenType::IDENTIFIER, value, m_line, start_column);
    }
};

// parser for building the abstract syntax tree (ast)
class Parser {
public:
    explicit Parser(const std::string_view input) : m_lexer(input) {
        advance();
    }

    std::vector<std::unique_ptr<QueryNode>> parse() {
        std::vector<std::unique_ptr<QueryNode>> nodes;

        while (m_current_token) {
            if (m_current_token->m_type == TokenType::NEWLINE) {
                advance();
                continue;
            }

            // validate the token is a keyword
            if (m_current_token->m_type != TokenType::LANGUAGE &&
                m_current_token->m_type != TokenType::DESCRIPTION &&
                m_current_token->m_type != TokenType::CONTEXT &&
                m_current_token->m_type != TokenType::TEST &&
                m_current_token->m_type != TokenType::DEPENDENCY &&
                m_current_token->m_type != TokenType::PERFORMANCE &&
                m_current_token->m_type != TokenType::COPYRIGHT) {
                throw std::runtime_error("Expected keyword at line " +
                    std::to_string(m_current_token->m_line));
            }

            // parse the appropriate node type
            switch (m_current_token->m_type) {
                case TokenType::LANGUAGE:
                    nodes.push_back(parse_code_request());
                    break;
                case TokenType::CONTEXT:
                    nodes.push_back(parse_context());
                    break;
                case TokenType::TEST:
                    nodes.push_back(parse_test());
                    break;
                case TokenType::DEPENDENCY:
                    nodes.push_back(parse_dependency());
                    break;
                case TokenType::PERFORMANCE:
                    nodes.push_back(parse_performance());
                    break;
                case TokenType::COPYRIGHT:
                    nodes.push_back(parse_copyright());
                    break;
                default:
                    throw std::runtime_error("Unexpected token type");
            }
        }

        return nodes;
    }

private:
    Lexer m_lexer;
    std::optional<Token> m_current_token;

    void advance() {
        m_current_token = m_lexer.next_token();
    }

    // parse a string token
    std::string parse_string() {
        // skip whitespace tokens
        while (m_current_token && m_current_token->m_type == TokenType::NEWLINE) {
            advance();
        }

        if (!m_current_token) {
            throw std::runtime_error("Unexpected end of input while expecting string");
        }

        if (m_current_token->m_type != TokenType::STRING) {
            throw std::runtime_error(
                "Expected string at line " + std::to_string(m_current_token->m_line) +
                ", column " + std::to_string(m_current_token->m_column) +
                " (got " + token_type_to_string(m_current_token->m_type) +
                " with value '" + m_current_token->m_value + "')"
            );
        }

        std::string value = m_current_token->m_value;
        advance(); // move to the next token
        return value;
    }

    // parse a code request node (@language + @description)
    std::unique_ptr<QueryNode> parse_code_request() {
        advance(); // skip past @language token

        // get the language string
        std::string language = parse_string();

        // skip any newlines
        while (m_current_token && m_current_token->m_type == TokenType::NEWLINE) {
            advance();
        }

        // now expect @description
        if (!m_current_token || m_current_token->m_type != TokenType::DESCRIPTION) {
            throw std::runtime_error(
                "Expected @description after @language at line " +
                std::to_string(m_current_token ? m_current_token->m_line : 0) +
                (m_current_token ? " (got '" + m_current_token->m_value + "')" : "")
            );
        }

        advance(); // skip past @description token
        std::string description = parse_string();

        return std::make_unique<CodeRequestNode>(language, description);
    }

    // parse a context node
    std::unique_ptr<QueryNode> parse_context() {
        advance(); // skip @context
        std::string context = parse_string();
        return std::make_unique<ContextNode>(context);
    }

    // parse a test node
    std::unique_ptr<QueryNode> parse_test() {
        advance(); // skip @test
        std::vector<std::string> test_cases;

        // get the first test case
        test_cases.push_back(parse_string());

        return std::make_unique<TestNode>(std::move(test_cases));
    }

    // parse a dependency node
    std::unique_ptr<QueryNode> parse_dependency() {
        advance(); // skip @dependency
        std::vector<std::string> dependencies;

        // get the dependency
        dependencies.push_back(parse_string());

        return std::make_unique<DependencyNode>(std::move(dependencies));
    }

    // parse a performance node
    std::unique_ptr<QueryNode> parse_performance() {
        advance(); // skip @performance
        std::string requirement = parse_string();
        return std::make_unique<PerformanceNode>(requirement);
    }

    // parse a copyright node
    std::unique_ptr<QueryNode> parse_copyright() {
        advance(); // skip @copyright
        std::string license = parse_string();
        std::string owner = parse_string();
        return std::make_unique<CopyrightNode>(license, owner);
    }
};

// helper function to check if string contains substring (case-sensitive)
bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

// file reading utility
std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// file writing utility
void write_file(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filepath);
    }
    file << content;
}

// test suite for the lexer
void test_lexer() {
    Logger::getInstance().log(LogLevel::INFO, "Running lexer tests...");

    // test basic tokenization
    {
        std::string input = "@language \"C++\"\n@description \"implement a stack\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::LANGUAGE);
        Logger::getInstance().log(LogLevel::DEBUG, "Token 1: ", token1->to_string());

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING && token2->m_value == "C++");
        Logger::getInstance().log(LogLevel::DEBUG, "Token 2: ", token2->to_string());

        auto token3 = lexer.next_token();
        assert(token3 && token3->m_type == TokenType::NEWLINE);
        Logger::getInstance().log(LogLevel::DEBUG, "Token 3: ", token3->to_string());

        auto token4 = lexer.next_token();
        assert(token4 && token4->m_type == TokenType::DESCRIPTION);

        auto token5 = lexer.next_token();
        assert(token5 && token5->m_type == TokenType::STRING && token5->m_value == "implement a stack");

        auto token6 = lexer.next_token();
        assert(!token6);
    }

    // test string escape sequences
    {
        std::string input = "@language \"C++\\n with newline\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::LANGUAGE);

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING);
        assert(token2->m_value == "C++\n with newline");
    }

    // test error handling - unterminated string
    try {
        std::string input = "@language \"unterminated string";
        Lexer lexer(input);
        lexer.next_token(); // @language
        lexer.next_token(); // should throw
        assert(false && "Expected exception for unterminated string");
    } catch (const std::runtime_error& e) {
        Logger::getInstance().log(LogLevel::DEBUG, "Expected exception: ", e.what());
    }

    // test new tokens
    {
        std::string input = "@dependency \"boost::asio\"\n@performance \"latency < 5ms\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::DEPENDENCY);

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING && token2->m_value == "boost::asio");

        auto token3 = lexer.next_token();
        assert(token3 && token3->m_type == TokenType::NEWLINE);

        auto token4 = lexer.next_token();
        assert(token4 && token4->m_type == TokenType::PERFORMANCE);

        auto token5 = lexer.next_token();
        assert(token5 && token5->m_type == TokenType::STRING && token5->m_value == "latency < 5ms");
    }

    // test copyright token
    {
        std::string input = "@copyright \"MIT License\" \"2025 dbjwhs\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::COPYRIGHT);

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING && token2->m_value == "MIT License");

        auto token3 = lexer.next_token();
        assert(token3 && token3->m_type == TokenType::STRING && token3->m_value == "2025 dbjwhs");
    }

    Logger::getInstance().log(LogLevel::INFO, "Lexer tests passed!");
}

// test suite for the parser
void test_parser() {
    Logger::getInstance().log(LogLevel::INFO, "Running parser tests...");

    // test basic parsing
    {
        std::string input = R"(
            @language "C++"
            @description "implement a thread-safe queue"
            @context "Using Modern C++ features"
            @test "Test empty queue"
            @dependency "std::mutex"
            @performance "Handle 1M operations per second"
        )";

        Parser parser(input);
        auto nodes = parser.parse();

        assert(nodes.size() == 5);
        Logger::getInstance().log(LogLevel::DEBUG, "Parsed ", nodes.size(), " nodes");

        // verify node types
        auto* code_request = dynamic_cast<CodeRequestNode*>(nodes[0].get());
        assert(code_request && code_request->language() == "C++");
        assert(code_request->description() == "implement a thread-safe queue");

        auto* context = dynamic_cast<ContextNode*>(nodes[1].get());
        assert(context && context->context() == "Using Modern C++ features");

        auto* test = dynamic_cast<TestNode*>(nodes[2].get());
        assert(test && test->test_cases().size() == 1);
        assert(test->test_cases()[0] == "Test empty queue");

        auto* dependency = dynamic_cast<DependencyNode*>(nodes[3].get());
        assert(dependency && dependency->dependencies().size() == 1);
        assert(dependency->dependencies()[0] == "std::mutex");

        auto* performance = dynamic_cast<PerformanceNode*>(nodes[4].get());
        assert(performance && performance->requirement() == "Handle 1M operations per second");
    }

    // test error handling - missing description
    try {
        std::string input = "@language \"C++\"";
        Parser parser(input);
        parser.parse();
        assert(false && "Expected exception for missing description");
    } catch (const std::runtime_error& e) {
        Logger::getInstance().log(LogLevel::DEBUG, "Expected exception: ", e.what());
    }

    // test error handling - invalid token
    try {
        std::string input = "@invalid \"test\"";
        Parser parser(input);
        parser.parse();
        assert(false && "Expected exception for invalid token");
    } catch (const std::runtime_error& e) {
        Logger::getInstance().log(LogLevel::DEBUG, "Expected exception: ", e.what());
    }

    Logger::getInstance().log(LogLevel::INFO, "Parser tests passed!");
}

// test suite for the compiler
void test_compiler() {
    Logger::getInstance().log(LogLevel::INFO, "Running compiler tests...");

    // test basic compilation
    {
        std::string input = R"(
            @language "C++"
            @description "implement a thread-safe queue"
            @context "Using Modern C++ features"
            @test "Test empty queue"
        )";

        Parser parser(input);
        auto nodes = parser.parse();

        QueryCompiler compiler;
        for (const auto& node : nodes) {
            node->accept(compiler);
        }

        std::string result = compiler.get_compiled_query();

        // verify the compiled query contains expected sections
        assert(contains(result, "Please generate C++ code that:"));
        assert(contains(result, "implement a thread-safe queue"));
        assert(contains(result, "Context:"));
        assert(contains(result, "Using Modern C++ features"));
        assert(contains(result, "Please include tests for the following cases:"));
        assert(contains(result, "Test empty queue"));

        Logger::getInstance().log(LogLevel::DEBUG, "Compiled query: ", result);
    }

    // test extended compilation with new node types
    {
        std::string input = R"(
            @language "C++"
            @description "implement a real-time data processor"
            @context "Embedded system environment"
            @dependency "boost::asio"
            @performance "Process 10k messages/second"
            @test "Test throughput under load"
        )";

        Parser parser(input);
        auto nodes = parser.parse();

        QueryCompiler compiler;
        for (const auto& node : nodes) {
            node->accept(compiler);
        }

        std::string result = compiler.get_compiled_query();

        // verify the compiled query contains expected sections
        assert(contains(result, "Please generate C++ code that:"));
        assert(contains(result, "implement a real-time data processor"));
        assert(contains(result, "Context:"));
        assert(contains(result, "Embedded system environment"));
        assert(contains(result, "Dependencies:"));
        assert(contains(result, "boost::asio"));
        assert(contains(result, "Performance Requirements:"));
        assert(contains(result, "Process 10k messages/second"));

        Logger::getInstance().log(LogLevel::DEBUG, "Compiled query: ", result);
    }

    // test compiler copyright
    {
        std::string input = R"(
            @copyright "MIT License" "2025 dbjwhs"
            @language "C++"
            @description "implement a thread-safe queue"
        )";

        Parser parser(input);
        auto nodes = parser.parse();

        QueryCompiler compiler;
        for (const auto& node : nodes) {
            node->accept(compiler);
        }

        std::string result = compiler.get_compiled_query();

        // verify the compiled query contains expected sections
        assert(contains(result, "Please include the following copyright header"));
        assert(contains(result, "// mit license"));
        assert(contains(result, "// Copyright (c) 2025 dbjwhs"));
        assert(contains(result, "Please generate C++ code that:"));

        Logger::getInstance().log(LogLevel::DEBUG, "Compiled query with copyright: ", result);
    }

    Logger::getInstance().log(LogLevel::INFO, "Compiler tests passed!");
}

// run all tests
void run_tests() {
    Logger::getInstance().log(LogLevel::INFO, "Starting CQL test suite");

    try {
        test_lexer();
        test_parser();
        test_compiler();

        Logger::getInstance().log(LogLevel::INFO, "All tests passed!");
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Test failed: ", e.what());
        throw; // rethrow to signal test failure
    }
}

// showcase example queries
void query_examples() {
    Logger::getInstance().log(LogLevel::INFO, "\n=== Query Examples ===");

    // example 1: simple function query
    Logger::getInstance().log(LogLevel::INFO, "\nExample 1 - Simple Function:");
    std::string simple_query =
        "@language \"C++\"\n"
        "@description \"implement a string reverse function\"\n"
        "@context \"Using string_view for efficiency\"\n"
        "@test \"Empty string\"\n"
        "@test \"Single character\"\n"
        "@test \"Multiple characters\"\n";

    try {
        Parser parser1(simple_query);
        auto nodes = parser1.parse();
        QueryCompiler compiler1;
        for (const auto& node : nodes) {
            node->accept(compiler1);
        }
        Logger::getInstance().log(LogLevel::INFO, "Input DSL:\n", simple_query);

        std::string result = compiler1.get_compiled_query();
        Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error: ", e.what());
    }

    // example 2: class implementation query
    Logger::getInstance().log(LogLevel::INFO, "\nExample 2 - Class Implementation:");
    std::string class_query =
        "@language \"C++\"\n"
        "@description \"implement a thread-safe queue class with a maximum size\"\n"
        "@context \"Using C++20 features and RAII principles\"\n"
        "@context \"Must be exception-safe\"\n"
        "@dependency \"std::mutex, std::condition_variable\"\n"
        "@performance \"Support 100k operations per second\"\n"
        "@test \"Test concurrent push operations\"\n"
        "@test \"Test concurrent pop operations\"\n"
        "@test \"Test boundary conditions (empty/full)\"\n"
        "@test \"Test exception safety guarantees\"\n";

    try {
        Parser parser2(class_query);
        auto nodes = parser2.parse();
        QueryCompiler compiler2;
        for (const auto& node : nodes) {
            node->accept(compiler2);
        }
        Logger::getInstance().log(LogLevel::INFO, "Input DSL:\n", class_query);

        std::string result = compiler2.get_compiled_query();
        Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error: ", e.what());
    }

    // example 3: with copyright and license
    Logger::getInstance().log(LogLevel::INFO, "\nExample 3 - With Copyright and License:");
    std::string copyright_query =
        "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
        "@language \"C++\"\n"
        "@description \"implement a binary search tree\"\n"
        "@context \"Modern C++ implementation\"\n"
        "@test \"Insert elements\"\n"
        "@test \"Delete elements\"\n"
        "@test \"Find elements\"\n";

    try {
        Parser parser3(copyright_query);
        auto nodes = parser3.parse();
        QueryCompiler compiler3;
        for (const auto& node : nodes) {
            node->accept(compiler3);
        }
        Logger::getInstance().log(LogLevel::INFO, "Input DSL:\n", copyright_query);

        std::string result = compiler3.get_compiled_query();
        Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error: ", e.what());
    }
}

// cli interface for interactive use
void run_cli() {
    Logger::getInstance().log(LogLevel::INFO, "CQL Interactive Mode");
    Logger::getInstance().log(LogLevel::INFO, "Type 'exit' to quit, 'help' for command list");

    std::string line;
    std::string current_query;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line == "exit" || line == "quit") {
            break;
        } else if (line == "help") {
            std::cout << "Commands:\n"
                      << "  help       - Show this help\n"
                      << "  exit/quit  - Exit the program\n"
                      << "  clear      - Clear the current query\n"
                      << "  show       - Show the current query\n"
                      << "  compile    - Compile the current query\n"
                      << "  load FILE  - Load query from file\n"
                      << "  save FILE  - Save compiled query to file\n";
        } else if (line == "clear") {
            current_query.clear();
            Logger::getInstance().log(LogLevel::INFO, "Query cleared");
        } else if (line == "show") {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::INFO, "Current query is empty");
            } else {
                Logger::getInstance().log(LogLevel::INFO, "Current query:\n", current_query);
            }
        } else if (line == "compile") {
            if (current_query.empty()) {
                Logger::getInstance().log(LogLevel::ERROR, "Nothing to compile");
                continue;
            }

            try {
                Parser parser(current_query);
                auto nodes = parser.parse();

                QueryCompiler compiler;
                for (const auto& node : nodes) {
                    node->accept(compiler);
                }

                std::string result = compiler.get_compiled_query();
                Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Compilation error: ", e.what());
            }
        } else if (line.substr(0, 5) == "load ") {
            std::string filename = line.substr(5);
            try {
                current_query = read_file(filename);
                Logger::getInstance().log(LogLevel::INFO, "Loaded query from ", filename);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to load file: ", e.what());
            }
        } else if (line.substr(0, 5) == "save ") {
            std::string filename = line.substr(5);
            try {
                if (current_query.empty()) {
                    Logger::getInstance().log(LogLevel::ERROR, "Nothing to save");
                    continue;
                }

                Parser parser(current_query);
                auto nodes = parser.parse();

                QueryCompiler compiler;
                for (const auto& node : nodes) {
                    node->accept(compiler);
                }

                std::string result = compiler.get_compiled_query();
                write_file(filename, result);
                Logger::getInstance().log(LogLevel::INFO, "Saved compiled query to ", filename);
            } catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Failed to save file: ", e.what());
            }
        } else {
            // add line to the current query
            if (!current_query.empty()) {
                current_query += "\n";
            }
            current_query += line;
        }
    }
}

// process a query file
bool process_file(const std::string& input_file, const std::string& output_file) {
    try {
        Logger::getInstance().log(LogLevel::INFO, "Processing file: ", input_file);

        std::string query = read_file(input_file);
        Parser parser(query);
        auto nodes = parser.parse();

        QueryCompiler compiler;
        for (const auto& node : nodes) {
            node->accept(compiler);
        }

        std::string result = compiler.get_compiled_query();

        if (output_file.empty()) {
            Logger::getInstance().log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
        } else {
            write_file(output_file, result);
            Logger::getInstance().log(LogLevel::INFO, "Compiled query written to ", output_file);
        }

        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error processing file: ", e.what());
        return false;
    }
}

#endif // cql_hpp

// main function for the cql compiler
int main(int argc, char* argv[]) {
    // initialize logger
    auto& logger = Logger::getInstance();
    logger.log(LogLevel::INFO, "Claude Query Language (CQL) Compiler v1.0");

    try {
        // parse command line arguments
        if (argc > 1) {
            std::string arg1 = argv[1];

            if (arg1 == "--test" || arg1 == "-t") {
                // run the test suite
                run_tests();
            } else if (arg1 == "--examples" || arg1 == "-e") {
                // show example queries
                query_examples();
            } else if (arg1 == "--interactive" || arg1 == "-i") {
                // run in interactive mode
                run_cli();
            } else if (arg1 == "--copyright") {
                // show an example of copyright usage
                std::string copyright_example =
                    "@copyright \"MIT License\" \"2025 dbjwhs\"\n"
                    "@language \"C++\"\n"
                    "@description \"implement a thread-safe queue\"\n";

                logger.log(LogLevel::INFO, "Copyright Example DSL:\n", copyright_example);

                Parser parser(copyright_example);
                auto nodes = parser.parse();

                QueryCompiler compiler;
                for (const auto& node : nodes) {
                    node->accept(compiler);
                }

                std::string result = compiler.get_compiled_query();
                logger.log(LogLevel::INFO, "\n=== Compiled Query with Copyright ===\n\n", result, "\n===================");
            } else {
                // assume it's an input file
                std::string output_file;
                if (argc > 2) {
                    output_file = argv[2];
                }

                if (!process_file(arg1, output_file)) {
                    return 1;
                }
            }
        } else {
            // no arguments, run comprehensive tests and examples
            logger.log(LogLevel::INFO, "Running in default mode - tests and examples");
            run_tests();
            query_examples();

            // example query - note the careful spacing and newlines
            std::string query =
                "@language \"C++\"\n"
                "@description \"implement a thread-safe queue with a maximum size\"\n"
                "@context \"Using C++20 features and RAII principles\"\n"
                "@test \"Test concurrent push operations\"\n"
                "@test \"Test concurrent pop operations\"\n"
                "@test \"Test boundary conditions\"\n";

            logger.log(LogLevel::INFO, "\nDefault example:");
            logger.log(LogLevel::INFO, "Input query:\n", query);

            Parser parser(query);
            const auto nodes = parser.parse();

            QueryCompiler compiler;
            for (const auto& node : nodes) {
                node->accept(compiler);
            }

            std::string result = compiler.get_compiled_query();
            logger.log(LogLevel::INFO, "\n=== Compiled Query ===\n\n", result, "\n===================");
        }
    } catch (const std::exception& e) {
        logger.log(LogLevel::ERROR, "Fatal error: ", e.what());
        return 1;
    }
    return 0;
}
