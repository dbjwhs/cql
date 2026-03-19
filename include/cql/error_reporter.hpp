// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_ERROR_REPORTER_HPP
#define CQL_ERROR_REPORTER_HPP

#include <string>
#include <vector>
#include <cstddef>

namespace cql {

/**
 * @brief Diagnostic information for a single parse error
 */
struct ParseDiagnostic {
    size_t line = 0;
    size_t column = 0;
    std::string error_code;
    std::string message;
    std::string offending_token;
    std::string expected;
};

/**
 * @brief Collects multiple parse errors for batch reporting
 *
 * Used by the parser's panic-mode error recovery to accumulate
 * all errors found during a single parse pass, instead of stopping
 * at the first error.
 */
class ErrorReporter {
public:
    void add_error(ParseDiagnostic diagnostic);

    [[nodiscard]] bool has_errors() const;

    [[nodiscard]] const std::vector<ParseDiagnostic>& get_errors() const;

    [[nodiscard]] std::string format_all() const;

    /**
     * @brief Throws ParserError with all accumulated errors if any exist
     */
    void throw_if_errors() const;

    void clear();

private:
    std::vector<ParseDiagnostic> m_errors;
};

} // namespace cql

#endif // CQL_ERROR_REPORTER_HPP
