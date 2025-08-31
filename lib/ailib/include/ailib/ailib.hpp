// MIT License
// Copyright (c) 2025 dbjwhs

/**
 * @file ailib.hpp
 * @brief Single-header convenience include for AILib
 * 
 * AILib is a comprehensive C++ library providing unified interfaces
 * to major AI providers (Anthropic, OpenAI, Google, etc.).
 * 
 * This header includes all core functionality needed for basic usage.
 */

#pragma once

// Core functionality
#include "core/provider.hpp"
#include "core/config.hpp"

// Provider implementations
#include "providers/anthropic.hpp"
#include "providers/factory.hpp"

// HTTP client
#include "http/client.hpp"

// Authentication
#include "auth/secure_store.hpp"

// Utilities
#include "detail/json_utils.hpp"

/**
 * @namespace ailib
 * @brief Main namespace for AILib functionality
 */
namespace ailib {

/**
 * @brief AILib version information
 */
struct version {
    static constexpr int major = 0;
    static constexpr int minor = 1;
    static constexpr int patch = 0;
    static constexpr const char* string = "0.1.0";
};

} // namespace ailib
