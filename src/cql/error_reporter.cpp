// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/error_reporter.hpp"
#include "../../include/cql/parser.hpp"
#include <sstream>

namespace cql {

void ErrorReporter::add_error(ParseDiagnostic diagnostic) {
    m_errors.push_back(std::move(diagnostic));
}

bool ErrorReporter::has_errors() const {
    return !m_errors.empty();
}

const std::vector<ParseDiagnostic>& ErrorReporter::get_errors() const {
    return m_errors;
}

std::string ErrorReporter::format_all() const {
    if (m_errors.empty()) {
        return "";
    }

    std::ostringstream oss;
    oss << m_errors.size() << " error(s) found:\n";

    for (size_t i = 0; i < m_errors.size(); ++i) {
        const auto& err = m_errors[i];
        oss << "  " << (i + 1) << ") [" << err.error_code << "] "
            << err.message
            << " at line " << err.line << ", column " << err.column;

        if (!err.offending_token.empty()) {
            oss << " (got '" << err.offending_token << "')";
        }
        if (!err.expected.empty()) {
            oss << " — expected " << err.expected;
        }
        oss << "\n";
    }

    return oss.str();
}

void ErrorReporter::throw_if_errors() const {
    if (m_errors.empty()) {
        return;
    }

    // Use the first error's location for the ParserError
    const auto& first = m_errors.front();
    throw ParserError(
        format_all(),
        first.line,
        first.column,
        first.error_code
    );
}

void ErrorReporter::clear() {
    m_errors.clear();
}

} // namespace cql
