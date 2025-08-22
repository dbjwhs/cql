// MIT License
// Copyright (c) 2025 dbjwhs

#ifndef CQL_SECURE_STRING_HPP
#define CQL_SECURE_STRING_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstring>

namespace cql {

/**
 * @class SecureString
 * @brief Secure string implementation that zeros memory on destruction
 * 
 * This class provides a more secure way to handle sensitive strings like
 * API keys and passwords by ensuring memory is zeroed when the object
 * is destroyed, reducing the window where sensitive data might remain
 * in memory.
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
    std::string m_data;
    
    /**
     * @brief Securely zero the internal string memory
     */
    void zero_memory() noexcept;
};

/**
 * @brief Create a SecureString from environment variable
 * @param env_var_name Name of environment variable
 * @return SecureString containing the environment variable value, empty if not found
 */
SecureString secure_getenv(const char* env_var_name);

} // namespace cql

#endif // CQL_SECURE_STRING_HPP