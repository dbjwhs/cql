// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/project_utils.hpp"
#include <iostream>

/**
 * This file contains platform-independent implementations that delegate
 * to platform-specific code.
 */

namespace cql::util {

// Implementation of the clipboard function that uses the namespace function
bool copy_to_clipboard(const std::string& content) {
    return clipboard::copy_to_clipboard(content);
}

} // namespace cql::util