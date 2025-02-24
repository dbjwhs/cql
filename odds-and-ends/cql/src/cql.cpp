// MIT License
// Copyright (c) 2025 dbjwhs

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <map>
#include <cctype>
#include <stdexcept>
#include <cassert>
#include <sstream>

// forward declarations
class QueryNode;
class CodeRequestNode;
class ContextNode;
class TestNode;

// visitor pattern for different node types
class QueryVisitor {
public:
    virtual ~QueryVisitor() = default;
    virtual void visit(const CodeRequestNode& node) = 0;
    virtual void visit(const ContextNode& node) = 0;
    virtual void visit(const TestNode& node) = 0;
};

// base class for AST nodes
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
        m_result_sections["context"] += node.context() + "\n";
    }

    void visit(const TestNode& node) override {
        // just store the test cases, we'll format them when getting the final query
        m_test_cases.insert(m_test_cases.end(),
                          node.test_cases().begin(),
                          node.test_cases().end());
    }

    [[nodiscard]] std::string get_compiled_query() const {
        std::string result;

        // add a code section if it exists
        auto code_it = m_result_sections.find("code");
        if (code_it != m_result_sections.end()) {
            result += code_it->second;
        }

        // add a context section if it exists
        auto context_it = m_result_sections.find("context");
        if (context_it != m_result_sections.end()) {
            result += context_it->second + "\n";
        }

        // add test cases if we have any
        if (!m_test_cases.empty()) {
            result += "Please include tests for the following cases:\n";
            for (const auto& test_case : m_test_cases) {
                result += "- " + test_case + "\n";
            }
            result += "\n";
        }
        return result;
    }

    // Add this method back
    void print_compiled_query(std::ostream& out = std::cout) const {
        out << "\n=== Compiled Query ===\n\n"
            << get_compiled_query()
            << "===================\n";
    }

private:
    std::map<std::string, std::string> m_result_sections;
    std::vector<std::string> m_test_cases;
};

// token types for our DSL
enum class TokenType {
    LANGUAGE,       // @language
    DESCRIPTION,    // @description
    CONTEXT,        // @context
    TEST,           // @test
    IDENTIFIER,     // any text
    STRING,         // "quoted text"
    NEWLINE,        // \n
    END             // end of input
};

// utility function to convert TokenType to string
std::string token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::LANGUAGE: return "LANGUAGE";
        case TokenType::DESCRIPTION: return "DESCRIPTION";
        case TokenType::CONTEXT: return "CONTEXT";
        case TokenType::TEST: return "TEST";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::STRING: return "STRING";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::END: return "END";
        default: return "UNKNOWN";
    }
}

// token structure
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

// lexical analyzer (Lexer)
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

    void advance() {
        m_current++;
        m_column++;
    }

    void skip_whitespace() {
        while (m_current < m_input.length() &&
               (std::isspace(m_input[m_current]) && m_input[m_current] != '\n')) {
            advance();
        }
    }

    std::optional<Token> lex_keyword() {
        advance(); // Skip @
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
        else throw std::runtime_error("Unknown keyword: @" + keyword);

        return Token(type, keyword, m_line, start_column);
    }

    std::optional<Token> lex_string() {
        if (m_current >= m_input.length() || m_input[m_current] != '"') {
            throw std::runtime_error("Expected opening quote at line " +
                std::to_string(m_line) + ", column " + std::to_string(m_column));
        }

        advance(); // Skip opening quote
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

        advance(); // Skip closing quote
        return Token(TokenType::STRING, value, m_line, start_column);
    }

    std::optional<Token> lex_identifier() {
        std::string value;
        size_t start_column = m_column;

        while (m_current < m_input.length() &&
               !std::isspace(m_input[m_current]) &&
               m_input[m_current] != '@') {
            value += m_input[m_current];
            advance();
        }

        return Token(TokenType::IDENTIFIER, value, m_line, start_column);
    }
};

// Parser for our DSL
class Parser {
public:
    explicit Parser(const std::string_view input) : lexer_(input) {
        advance();
    }

    std::vector<std::unique_ptr<QueryNode>> parse() {
        std::vector<std::unique_ptr<QueryNode>> nodes;

        while (current_token_) {
            if (current_token_->m_type == TokenType::NEWLINE) {
                advance();
                continue;
            }

            if (current_token_->m_type != TokenType::LANGUAGE &&
                current_token_->m_type != TokenType::DESCRIPTION &&
                current_token_->m_type != TokenType::CONTEXT &&
                current_token_->m_type != TokenType::TEST) {
                throw std::runtime_error("Expected keyword at line " +
                    std::to_string(current_token_->m_line));
            }

            switch (current_token_->m_type) {
                case TokenType::LANGUAGE:
                    nodes.push_back(parse_code_request());
                    break;
                case TokenType::CONTEXT:
                    nodes.push_back(parse_context());
                    break;
                case TokenType::TEST:
                    nodes.push_back(parse_test());
                    break;
                default:
                    throw std::runtime_error("Unexpected token type");
            }
        }

        return nodes;
    }

private:
    Lexer lexer_;
    std::optional<Token> current_token_;

    void advance() {
        current_token_ = lexer_.next_token();
    }

    std::string parse_string() {
        // skip whitespace tokens
        while (current_token_ && current_token_->m_type == TokenType::NEWLINE) {
            advance();
        }

        if (!current_token_) {
            throw std::runtime_error("Unexpected end of input while expecting string");
        }

        if (current_token_->m_type != TokenType::STRING) {
            throw std::runtime_error(
                "Expected string at line " + std::to_string(current_token_->m_line) +
                ", column " + std::to_string(current_token_->m_column) +
                " (got " + current_token_->m_value + ")"
            );
        }

        std::string value = current_token_->m_value;
        advance(); // Move to the next token
        return value;
    }

    std::unique_ptr<QueryNode> parse_code_request() {
        advance(); // skip past @language token

        // get the language string
        std::string language = parse_string();

        // skip any newlines
        while (current_token_ && current_token_->m_type == TokenType::NEWLINE) {
            advance();
        }

        // now expect @description
        if (!current_token_ || current_token_->m_type != TokenType::DESCRIPTION) {
            throw std::runtime_error(
                "Expected @description after @language at line " +
                std::to_string(current_token_ ? current_token_->m_line : 0) +
                (current_token_ ? " (got '" + current_token_->m_value + "')" : "")
            );
        }

        advance(); // skip past @description token
        std::string description = parse_string();

        return std::make_unique<CodeRequestNode>(language, description);
    }

    std::unique_ptr<QueryNode> parse_context() {
        advance(); // skip @context
        std::string context = parse_string();
        return std::make_unique<ContextNode>(context);
    }

    std::unique_ptr<QueryNode> parse_test() {
        advance(); // skip @test
        std::vector<std::string> test_cases;

        while (current_token_ && current_token_->m_type == TokenType::STRING) {
            test_cases.push_back(current_token_->m_value);
            advance();
        }

        return std::make_unique<TestNode>(std::move(test_cases));
    }
};

// helper function to check if string contains substring (case-sensitive)
bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

// test suite
void run_tests() {
    // test 1: Basic lexing
    {
        std::string input = "@language \"C++\"\n@description \"implement a stack\"";
        Lexer lexer(input);

        auto token1 = lexer.next_token();
        assert(token1 && token1->m_type == TokenType::LANGUAGE);

        auto token2 = lexer.next_token();
        assert(token2 && token2->m_type == TokenType::STRING && token2->m_value == "C++");

        auto token3 = lexer.next_token();
        assert(token3 && token3->m_type == TokenType::NEWLINE);
    }

    // test 2: Complete query parsing
    {
        std::string input = R"(
            @language "C++"
            @description "implement a thread-safe queue"
            @context "Using Modern C++ features"
            @test "Test empty queue"
            @test "Test concurrent operations"
        )";

        Parser parser(input);
        auto nodes = parser.parse();

        assert(nodes.size() == 4);

        // verify the first node is a CodeRequestNode
        auto* code_request = dynamic_cast<CodeRequestNode*>(nodes[0].get());
        assert(code_request && code_request->language() == "C++");
    }

    // test 3: Error handling
    try {
        std::string invalid_input = "@invalid \"test\"";
        Parser parser(invalid_input);
        parser.parse();
        assert(false && "Should have thrown an exception");
    } catch (const std::runtime_error& e) {
        // expected exception
    }

    std::cout << "All lexer and parser tests passed!\n";
}

void query_examples() {
    std::cout << "\n=== Query Examples ===\n";

    // example 1: Simple function query
    std::cout << "\nExample 1 - Simple Function:\n";
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
        std::cout << "Input DSL:\n" << simple_query << "\n";
        compiler1.print_compiled_query();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // example 2: Class implementation query
    std::cout << "\nExample 2 - Class Implementation:\n";
    std::string class_query =
        "@language \"C++\"\n"
        "@description \"implement a thread-safe queue class with a maximum size\"\n"
        "@context \"Using C++17 features and RAII principles\"\n"
        "@context \"Must be exception-safe\"\n"
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
        std::cout << "Input DSL:\n" << class_query << "\n";
        compiler2.print_compiled_query();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    run_tests();
    query_examples();

    // example query - note the careful spacing and newlines
    std::string query =
        "@language \"C++\"\n"
        "@description \"implement a thread-safe queue with a maximum size\"\n"
        "@context \"Using C++17 features and RAII principles\"\n"
        "@test \"Test concurrent push operations\"\n"
        "@test \"Test concurrent pop operations\"\n"
        "@test \"Test boundary conditions\"\n";

    try {
        Parser parser(query);
        const auto nodes = parser.parse();

        QueryCompiler compiler;
        for (const auto& node : nodes) {
            node->accept(compiler);
        }

        std::cout << "Generated query:\n" << compiler.get_compiled_query() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
