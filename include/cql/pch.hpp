// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_PCH_HPP
#define CQL_PCH_HPP

// Precompiled header for CQL project
// Contains the most commonly used standard library headers
// and third-party library headers to speed up compilation

// Standard C++ library headers (most frequently used)
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <optional>
#include <filesystem>
#include <stdexcept>
#include <regex>
#include <string_view>
#include <set>
#include <memory>
#include <functional>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>

// Third-party headers that are commonly used
#include <nlohmann/json.hpp>

// System headers
#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef __APPLE__
    #include <unistd.h>
#endif

#ifdef __linux__
    #include <unistd.h>
#endif

#endif // CQL_PCH_HPP
