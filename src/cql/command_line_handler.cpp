// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/command_line_handler.hpp"
#include "../../include/cql/cql.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace cql {

CommandLineHandler::CommandLineHandler(int argc, char* argv[]) 
    : m_argc(argc) {
    copy_arguments(argc, argv);
}

void CommandLineHandler::copy_arguments(int argc, char* argv[]) {
    // Allocate new array
    m_argv = std::make_unique<char*[]>(argc);
    
    // Copy arguments and build string vector
    for (int i = 0; i < argc; ++i) {
        size_t len = std::strlen(argv[i]) + 1;
        char* arg_copy = new char[len];
        std::strcpy(arg_copy, argv[i]);
        m_argv[i] = arg_copy;
        m_args.emplace_back(argv[i]);
    }
}

bool CommandLineHandler::has_option(const std::string& option) const {
    return std::find(m_args.begin(), m_args.end(), option) != m_args.end();
}

std::optional<std::string> CommandLineHandler::get_option_value(const std::string& option) const {
    auto it = std::find(m_args.begin(), m_args.end(), option);
    if (it != m_args.end() && ++it != m_args.end()) {
        // Check if the next element is not another option
        if (!it->empty() && (*it)[0] != '-') {
            return *it;
        }
    }
    return std::nullopt;
}

bool CommandLineHandler::find_and_remove_option(const std::string& option, std::string& value) {
    auto new_argv = std::make_unique<char*[]>(m_argc);
    int new_argc = 0;

    bool found = false;
    bool skip_next = false;

    // Copy program name
    new_argv[new_argc++] = m_argv[0];

    // Process remaining arguments
    for (int ndx = 1; ndx < m_argc; ++ndx) {
        if (skip_next) {
            skip_next = false;
            continue;
        }

        if (std::string(m_argv[ndx]) == option && ndx + 1 < m_argc) {
            value = m_argv[ndx + 1];
            found = true;
            skip_next = true;
        } else {
            new_argv[new_argc++] = m_argv[ndx];
        }
    }

    if (found) {
        m_argv = std::move(new_argv);
        m_argc = new_argc;

        // Rebuild args vector
        m_args.clear();
        for (int i = 0; i < m_argc; ++i) {
            m_args.emplace_back(m_argv[i]);
        }
    }

    return found;
}

bool CommandLineHandler::find_and_remove_flag(const std::string& flag) {
    auto new_argv = std::make_unique<char*[]>(m_argc);
    int new_argc = 0;

    bool found = false;

    // Copy program name
    new_argv[new_argc++] = m_argv[0];

    // Process remaining arguments
    for (int ndx = 1; ndx < m_argc; ++ndx) {
        if (std::string(m_argv[ndx]) == flag) {
            found = true;
            // Skip this flag
        } else {
            new_argv[new_argc++] = m_argv[ndx];
        }
    }

    if (found) {
        m_argv = std::move(new_argv);
        m_argc = new_argc;

        // Rebuild args vector
        m_args.clear();
        for (int i = 0; i < m_argc; ++i) {
            m_args.emplace_back(m_argv[i]);
        }
    }

    return found;
}

std::vector<std::string> CommandLineHandler::get_positional_args() const {
    std::vector<std::string> positional;
    
    for (size_t i = 1; i < m_args.size(); ++i) {
        // Skip options and their values
        if (!m_args[i].empty() && m_args[i][0] == '-') {
            // Check if this option has a value
            if (i + 1 < m_args.size() && !m_args[i + 1].empty() && m_args[i + 1][0] != '-') {
                ++i; // Skip the value
            }
        } else {
            positional.push_back(m_args[i]);
        }
    }
    
    return positional;
}

std::string CommandLineHandler::get_program_name() const {
    return m_args.empty() ? "" : m_args[0];
}

void CommandLineHandler::print_help() {
    std::cout << "Claude Query Language (CQL) Compiler v" << CQL_VERSION_STRING << " (" << CQL_BUILD_TIMESTAMP << ")\n"
              << "Usage: cql [OPTIONS] [INPUT_FILE] [OUTPUT_FILE]\n\n"
              << "Options:\n"
              << "  --help, -h              Show this help information\n"
              << "  --version, -v           Show version information\n"
              << "  --interactive, -i       Run in interactive mode\n"
              << "  --clipboard, -c         Copy output to clipboard instead of writing to a file\n"
              << "  --env [PATH]            Load environment variables from .env file (default: .env in current dir)\n"
              << "  --include-header        Include compiler headers and status messages in output\n"
              << "  --debug-level LEVEL     Set log level (INFO|NORMAL|DEBUG|ERROR|CRITICAL, default: NORMAL)\n"
              << "  --log-console           Enable logging to console (default: file only, console at INFO)\n"
              << "  --console-level LEVEL   Set console log level independently (default: INFO)\n"
              << "  --file-level LEVEL      Set file log level independently (default: same as --debug-level)\n"
              << "  --log-file PATH         Set log file path (default: cql.log)\n"
              << "  --log-max-size BYTES    Enable log rotation at specified size (bytes, 0=disabled)\n"
              << "  --log-max-files COUNT   Max rotated files to keep (default: 5)\n"
              << "  --log-timestamp FORMAT  Timestamp format (iso8601|iso8601-local|simple|epoch|none, default: simple)\n"
              << "  --templates, -l         List all available templates\n"
              << "  --template NAME, -T     Use a specific template\n"
              << "  --template NAME --force Use template even with validation errors\n"
              << "  --validate NAME         Validate a specific template\n"
              << "  --validate-all PATH     Validate all templates in the specified path\n"
              << "  --docs NAME             Generate documentation for a template\n"
              << "  --docs-all              Generate documentation for all templates\n"
              << "  --export PATH [format]  Export template documentation to a file\n"
              << "                          (formats: md, html, txt; default: md)\n\n"
              << "API Integration Options:\n"
              << "  --submit                Submit the compiled query to the Claude API\n"
              << "  --model <model_name>    Specify the Claude model to use (default: claude-3-opus)\n"
              << "  --output-dir <directory> Directory to save generated code files\n"
              << "  --overwrite             Overwrite existing files without prompting\n"
              << "  --create-dirs           Create missing directories for output files\n"
              << "  --no-save               Display generated code but don't save to files\n\n"
              << "Meta-Prompt Compilation Options:\n"
              << "  --optimize              Optimize the compiled query using meta-prompt compilation\n"
              << "  --mode <mode>           Compilation mode (LOCAL_ONLY, CACHED_LLM, FULL_LLM, default: CACHED_LLM)\n"
              << "  --goal <goal>           Optimization goal (REDUCE_TOKENS, IMPROVE_ACCURACY, BALANCED, DOMAIN_SPECIFIC)\n"
              << "  --domain <domain>       Domain context for optimization (e.g., 'software', 'research', 'creative')\n"
              << "  --show-metrics          Display compilation metrics (timing, token usage, cost estimation)\n"
              << "  --show-validation       Display semantic validation results and confidence scores\n\n"
              << "If INPUT_FILE is provided, it will be processed as a CQL query.\n"
              << "If OUTPUT_FILE is also provided, the compiled query will be written to it.\n"
              << "If --clipboard option is used, the output will be copied to the clipboard.\n";
}

} // namespace cql
