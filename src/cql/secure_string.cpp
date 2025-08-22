// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/secure_string.hpp"
#include <cstdlib>
#include <algorithm>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace cql {

SecureString::SecureString(const std::string& str) : m_data(str) {
    // Consider locking memory to prevent swapping to disk
    // This is optional and system-dependent
}

SecureString::SecureString(const char* str) : m_data(str ? str : "") {
    // Consider locking memory to prevent swapping to disk
}

SecureString::SecureString(SecureString&& other) noexcept : m_data(std::move(other.m_data)) {
    other.zero_memory();
}

SecureString& SecureString::operator=(SecureString&& other) noexcept {
    if (this != &other) {
        zero_memory();
        m_data = std::move(other.m_data);
        other.zero_memory();
    }
    return *this;
}

SecureString::~SecureString() {
    zero_memory();
}

const std::string& SecureString::data() const noexcept {
    return m_data;
}

bool SecureString::empty() const noexcept {
    return m_data.empty();
}

size_t SecureString::size() const noexcept {
    return m_data.size();
}

void SecureString::clear() noexcept {
    zero_memory();
    m_data.clear();
}

std::string SecureString::masked() const {
    if (m_data.empty()) {
        return "[empty]";
    }
    
    if (m_data.size() <= 6) {
        return "[***]";
    }
    
    // Show first 3 and last 3 characters, mask the middle
    return m_data.substr(0, 3) + "..." + m_data.substr(m_data.size() - 3);
}

void SecureString::zero_memory() noexcept {
    if (!m_data.empty()) {
        // Use volatile to prevent compiler optimization
        volatile char* ptr = const_cast<char*>(m_data.data());
        std::fill_n(ptr, m_data.size(), 0);
        
        #if defined(_WIN32)
            // On Windows, use SecureZeroMemory if available
            SecureZeroMemory(const_cast<char*>(m_data.data()), m_data.size());
        #else
            // On Unix-like systems, use explicit_bzero if available
            #ifdef __GLIBC__
                explicit_bzero(const_cast<char*>(m_data.data()), m_data.size());
            #else
                // Fallback: volatile write to prevent optimization
                for (size_t i = 0; i < m_data.size(); ++i) {
                    const_cast<char*>(m_data.data())[i] = 0;
                }
            #endif
        #endif
    }
}

SecureString secure_getenv(const char* env_var_name) {
    if (!env_var_name) {
        return SecureString{};
    }
    
    const char* env_value = std::getenv(env_var_name);
    if (!env_value) {
        return SecureString{};
    }
    
    return SecureString{env_value};
}

} // namespace cql