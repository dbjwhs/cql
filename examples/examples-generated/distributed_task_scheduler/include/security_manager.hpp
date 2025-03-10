// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "scheduler_core.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <boost/asio/ssl.hpp>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include "jwt_traits_adapter.hpp"
// Use our adapter instead of direct JWT include
#define JWT_DISABLE_PICOJSON
#include "../external/jwt-cpp/include/jwt-cpp/jwt.h"

namespace distributed_scheduler {

/**
 * @brief Handles authentication and authorization for the task scheduler
 */
class SecurityManager {
public:
    SecurityManager(const std::string& jwt_secret_key);
    
    // Authentication and token management
    SecurityContext authenticate(const std::string& username, const std::string& password);
    SecurityContext validate_token(const std::string& token);
    void revoke_token(const std::string& token);
    
    // Authorization for task operations
    bool can_submit_task(const SecurityContext& context, const std::string& task_type);
    bool can_cancel_task(const SecurityContext& context, const std::string& task_id);
    bool can_view_task(const SecurityContext& context, const std::string& task_id);
    bool can_view_system_stats(const SecurityContext& context);
    
    // Role-based access control
    void add_role(const std::string& role);
    void add_permission(const std::string& role, const std::string& permission);
    void assign_role_to_user(const std::string& username, const std::string& role);
    
    // User management
    void add_user(const std::string& username, const std::string& password, 
                  const std::vector<std::string>& roles = {});
    void remove_user(const std::string& username);
    
private:
    // Secure password handling
    std::string hash_password(const std::string& password, const std::string& salt);
    std::string generate_salt();
    std::string generate_token(const std::string& username, const std::vector<std::string>& roles);
    
    // Token validation and management
    bool is_token_valid(const std::string& token);
    
    // Internal data structures
    std::shared_mutex users_mutex_;
    struct UserInfo {
        std::string password_hash;
        std::string salt;
        std::vector<std::string> roles;
    };
    std::unordered_map<std::string, UserInfo> users_;
    
    std::shared_mutex roles_mutex_;
    std::unordered_map<std::string, std::unordered_set<std::string>> role_permissions_;
    
    std::shared_mutex tokens_mutex_;
    std::unordered_set<std::string> revoked_tokens_;
    
    // JWT settings
    std::string jwt_secret_key_;
    std::chrono::seconds token_validity_{3600}; // 1 hour by default
};

/**
 * @brief Manages secure communication between scheduler components
 */
class SecureMessaging {
public:
    SecureMessaging(boost::asio::io_context& io_context, 
                   const std::string& cert_file,
                   const std::string& key_file);
    
    // SSL/TLS context setup
    boost::asio::ssl::context& get_ssl_context();
    
    // Message encryption and signing
    std::vector<uint8_t> encrypt_message(const std::vector<uint8_t>& message, 
                                         const std::string& recipient_public_key);
    
    std::vector<uint8_t> decrypt_message(const std::vector<uint8_t>& encrypted_message);
    
    std::vector<uint8_t> sign_message(const std::vector<uint8_t>& message);
    bool verify_signature(const std::vector<uint8_t>& message, 
                          const std::vector<uint8_t>& signature,
                          const std::string& sender_public_key);
    
private:
    // io_context not used in current implementation but kept for future extensions
    boost::asio::ssl::context ssl_context_;
    
    // Cryptographic keys
    std::string private_key_;
    std::string public_key_;
    std::unordered_map<std::string, std::string> peer_public_keys_;
};

// Forward declaration of MessageQueueConnector (actual definition in message_queue.hpp)
class MessageQueueConnector;

} // namespace distributed_scheduler
