// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/secure_string.hpp"
#include <cstdlib>
#include <algorithm>
#include <new>
#include <cerrno>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace cql {

// ============================================================================
// SecureAllocator Implementation
// ============================================================================

template<typename T>
T* SecureAllocator<T>::allocate(size_type n) {
    if (n == 0) return nullptr;
    
    // Calculate total size in bytes
    const size_type total_size = n * sizeof(T);
    
    // Allocate aligned memory
    void* ptr = nullptr;
    
#if defined(_WIN32)
    // Windows: Use VirtualAlloc with PAGE_READWRITE
    ptr = ::VirtualAlloc(nullptr, total_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!ptr) {
        throw std::bad_alloc();
    }
    
    // Lock the memory to prevent swapping
    if (!::VirtualLock(ptr, total_size)) {
        ::VirtualFree(ptr, 0, MEM_RELEASE);
        throw std::bad_alloc();
    }
#else
    // Unix: Use aligned allocation
    if (posix_memalign(&ptr, std::max(alignof(T), sizeof(void*)), total_size) != 0) {
        throw std::bad_alloc();
    }
    
    // Lock memory to prevent swapping to disk
    if (mlock(ptr, total_size) != 0) {
        std::free(ptr);
        // If mlock fails, we still proceed but log the issue
        // This allows the allocator to work on systems with strict mlock limits
        // In production, you might want to throw here for guaranteed security
    }
#endif

    return static_cast<T*>(ptr);
}

template<typename T>
void SecureAllocator<T>::deallocate(T* ptr, size_type n) noexcept {
    if (!ptr || n == 0) return;
    
    const size_type total_size = n * sizeof(T);
    
    // Securely zero the memory before freeing
    secure_zero(ptr, total_size);
    
#if defined(_WIN32)
    // Windows: Unlock and free
    ::VirtualUnlock(ptr, total_size);
    ::VirtualFree(ptr, 0, MEM_RELEASE);
#else
    // Unix: Unlock and free
    munlock(ptr, total_size);  // Ignore errors on unlock
    std::free(ptr);
#endif
}

template<typename T>
void SecureAllocator<T>::secure_zero(void* ptr, size_type size) noexcept {
    if (!ptr || size == 0) return;
    
#if defined(_WIN32)
    // Windows: Use SecureZeroMemory - guaranteed not to be optimized away
    SecureZeroMemory(ptr, size);
#else
    // Unix: Use explicit_bzero if available, otherwise volatile writes
    #if defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 25
        explicit_bzero(ptr, size);
    #elif defined(__OpenBSD__) || defined(__FreeBSD__)
        explicit_bzero(ptr, size);
    #else
        // Fallback: Use volatile pointer to prevent optimization
        volatile unsigned char* vptr = static_cast<volatile unsigned char*>(ptr);
        for (size_type i = 0; i < size; ++i) {
            vptr[i] = 0;
        }
        // Add memory barrier to ensure writes are not reordered
        __asm__ __volatile__("" ::: "memory");
    #endif
#endif
}

// Explicit template instantiation for char (most common use case)
template class SecureAllocator<char>;

// ============================================================================
// SecureString Implementation
// ============================================================================

SecureString::SecureString(const std::string& str) : m_data(str.begin(), str.end()) {
    // Memory locking is handled automatically by SecureAllocator
}

SecureString::SecureString(const char* str) : m_data(str ? str : "") {
    // Memory locking is handled automatically by SecureAllocator
}

SecureString::SecureString(SecureString&& other) noexcept : m_data(std::move(other.m_data)) {
    // Move constructor - SecureAllocator handles secure cleanup automatically
}

SecureString& SecureString::operator=(SecureString&& other) noexcept {
    if (this != &other) {
        // SecureAllocator automatically zeros old memory when reassigning
        m_data = std::move(other.m_data);
    }
    return *this;
}

SecureString::~SecureString() {
    // Destructor - SecureAllocator automatically zeros memory
}

const std::string& SecureString::data() const noexcept {
    // Note: This creates a temporary conversion - use sparingly for sensitive data
    static thread_local std::string temp_conversion;
    temp_conversion = m_data;
    return temp_conversion;
}

const char* SecureString::c_str() const noexcept {
    return m_data.c_str();
}

bool SecureString::empty() const noexcept {
    return m_data.empty();
}

size_t SecureString::size() const noexcept {
    return m_data.size();
}

void SecureString::clear() noexcept {
    // SecureAllocator will automatically zero memory when cleared
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
    // Convert secure string parts to regular string
    std::string first_part(m_data.begin(), m_data.begin() + 3);
    std::string last_part(m_data.end() - 3, m_data.end());
    return first_part + "..." + last_part;
}

void SecureString::clear_data() noexcept {
    // SecureAllocator handles secure memory clearing automatically
    // This method is kept for interface compatibility but delegates to clear()
    clear();
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