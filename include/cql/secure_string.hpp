// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_SECURE_STRING_HPP
#define CQL_SECURE_STRING_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstring>
#include <stdexcept>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace cql {

/**
 * @class SecureAllocator
 * @brief Custom allocator that locks memory and zeros on deallocation
 * 
 * This allocator provides security for sensitive data by:
 * - Locking allocated memory to prevent swapping to disk
 * - Zeroing memory before deallocation to prevent data leakage
 * - Using secure memory clearing functions that resist compiler optimization
 * 
 * WARNING: This allocator should ONLY be used for sensitive data like
 * API keys, passwords, tokens, etc. It has performance overhead and
 * memory usage limitations due to mlocked memory limits.
 */
template<typename T>
class SecureAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    
    template<typename U>
    struct rebind {
        using other = SecureAllocator<U>;
    };
    
    SecureAllocator() noexcept = default;
    
    template<typename U>
    SecureAllocator(const SecureAllocator<U>&) noexcept {}
    
    /**
     * @brief Allocate locked memory for sensitive data
     * @param n Number of elements to allocate
     * @return Pointer to allocated and locked memory
     * @throws std::bad_alloc if allocation or locking fails
     */
    [[nodiscard]] T* allocate(size_type n);
    
    /**
     * @brief Securely deallocate memory
     * @param ptr Pointer to memory to deallocate
     * @param n Number of elements originally allocated
     */
    void deallocate(T* ptr, size_type n) noexcept;
    
    template<typename U>
    bool operator==(const SecureAllocator<U>&) const noexcept { return true; }
    
    template<typename U>
    bool operator!=(const SecureAllocator<U>&) const noexcept { return false; }

private:
    /**
     * @brief Securely zero memory using platform-specific functions
     * @param ptr Pointer to memory to zero
     * @param size Size in bytes to zero
     */
    static void secure_zero(void* ptr, size_type size) noexcept;
};

// Type alias for secure string using our custom allocator
using secure_basic_string = std::basic_string<char, std::char_traits<char>, SecureAllocator<char>>;

/**
 * @class SecureString
 * @brief Secure string wrapper for sensitive data
 * 
 * This class provides a secure way to handle sensitive strings like API keys 
 * and passwords. It uses a custom allocator that:
 * - Locks memory to prevent swapping to disk
 * - Zeros memory on deallocation to prevent data recovery
 * - Uses secure memory clearing resistant to compiler optimization
 * 
 * WARNING: ONLY use this class for truly sensitive data. It has significant
 * performance overhead and system resource limitations.
 */
class SecureString {
public:
    /**
     * @brief Default constructor
     */
    SecureString() = default;
    
    /**
     * @brief Construct from std::string
     * @param str Source string to copy securely
     */
    explicit SecureString(const std::string& str);
    
    /**
     * @brief Construct from C-style string
     * @param str Source string to copy securely
     */
    explicit SecureString(const char* str);
    
    /**
     * @brief Copy constructor (disabled for security)
     */
    SecureString(const SecureString& other) = delete;
    
    /**
     * @brief Move constructor
     * @param other Source SecureString to move from
     */
    SecureString(SecureString&& other) noexcept;
    
    /**
     * @brief Copy assignment (disabled for security)
     */
    SecureString& operator=(const SecureString& other) = delete;
    
    /**
     * @brief Move assignment
     * @param other Source SecureString to move from
     * @return Reference to this object
     */
    SecureString& operator=(SecureString&& other) noexcept;
    
    /**
     * @brief Destructor - securely zeros memory
     */
    ~SecureString();
    
    /**
     * @brief Get the string data (use with caution)
     * @return Const reference to internal string
     * @warning This exposes the sensitive data - use sparingly
     */
    [[nodiscard]] const std::string& data() const noexcept;
    
    /**
     * @brief Get C-style string pointer (use with extreme caution)
     * @return Const pointer to null-terminated string data
     * @warning This directly exposes sensitive data in memory
     */
    [[nodiscard]] const char* c_str() const noexcept;
    
    /**
     * @brief Check if the secure string is empty
     * @return true if empty, false otherwise
     */
    [[nodiscard]] bool empty() const noexcept;
    
    /**
     * @brief Get the length of the string
     * @return Length of the stored string
     */
    [[nodiscard]] size_t size() const noexcept;
    
    /**
     * @brief Clear the string and zero memory
     */
    void clear() noexcept;
    
    /**
     * @brief Create a masked version for logging (shows only first/last chars)
     * @return Masked string safe for logging
     */
    [[nodiscard]] std::string masked() const;

private:
    secure_basic_string m_data;
    
    /**
     * @brief Clear the secure string (automatic with secure allocator)
     */
    void clear_data() noexcept;
};

/**
 * @brief Create a SecureString from environment variable
 * @param env_var_name Name of environment variable
 * @return SecureString containing the environment variable value, empty if not found
 */
SecureString secure_getenv(const char* env_var_name);

} // namespace cql

#endif // CQL_SECURE_STRING_HPP