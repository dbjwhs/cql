// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/project_utils.hpp"
#include <iostream>

/**
 * This file contains stub implementations for clipboard operations
 * on platforms where specific clipboard functionality is not implemented.
 */

// These implementations only get compiled in if the platform-specific
// implementations are not used.
#if !defined(__APPLE__) && !defined(_WIN32)

namespace clipboard {

bool copy_to_clipboard(const std::string& /*text*/) {
    // Default implementation that does nothing
    std::cerr << "Clipboard operations not implemented on this platform." << std::endl;
    return false;
}

std::string get_from_clipboard() {
    // Default implementation that returns an empty string
    std::cerr << "Clipboard operations not implemented on this platform." << std::endl;
    return "";
}

} // namespace clipboard

#endif // !defined(__APPLE__) && !defined(_WIN32)