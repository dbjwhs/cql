// MIT License
// Copyright (c) 2025 dbjwhs

#include "security_manager.hpp"
#include <random>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace distributed_scheduler {

SecurityManager::SecurityManager(const std::string& jwt_secret_key)
    : jwt_secret_key_(jwt_secret_key) {
    
    // Initialize default roles and permissions
    add_role("admin");
    add_role("operator");
    add_role("user");
    
    // Admin permissions
    add_permission("admin", "submit_task:*");
    add_permission("admin", "cancel_task:*");
    add_permission("admin", "view_task:*");
    add_permission("admin", "view_stats");
    add_permission("admin", "manage_workers");
    
    // Operator permissions
    add_permission("operator", "submit_task:compute");
    add_permission("operator", "submit_task:io");
    add_permission("operator", "cancel_task:own");
    add_permission("operator", "view_task:own");
    add_permission("operator", "view_stats");
    
    // User permissions
    add_permission("user", "submit_task:compute");
    add_permission("user", "cancel_task:own");
    add_permission("user", "view_task:own");
}

SecurityContext SecurityManager::authenticate(const std::string& username, const std::string& password) {
    std::shared_lock<std::shared_mutex> lock(users_mutex_);
    
    auto it = users_.find(username);
    if (it == users_.end()) {
        throw SecurityException("User not found");
    }
    
    const auto& user_info = it->second;
    
    // Verify password
    std::string hashed_input = hash_password(password, user_info.salt);
    if (hashed_input != user_info.password_hash) {
        throw SecurityException("Invalid password");
    }
    
    // Create security context with token
    SecurityContext context;
    context.user_id = username;
    context.roles = user_info.roles;
    context.auth_token = generate_token(username, user_info.roles);
    context.token_expiry = std::chrono::system_clock::now() + token_validity_;
    
    return context;
}

SecurityContext SecurityManager::validate_token(const std::string& token) {
    // Check if token is revoked
    {
        std::shared_lock<std::shared_mutex> lock(tokens_mutex_);
        if (revoked_tokens_.find(token) != revoked_tokens_.end()) {
            throw SecurityException("Token has been revoked");
        }
    }
    
    try {
        // Verify and decode the JWT using our custom adapter
        auto decoded = jwt::decode<jwt::traits::kazuho_picojson_adapter>(token);
        
        // Verify token signature
        auto verifier = jwt::verify<jwt::traits::kazuho_picojson_adapter>()
            .allow_algorithm(jwt::algorithm::hs256{jwt_secret_key_});
        
        verifier.verify(decoded);
        
        // Check expiration
        const auto exp = decoded.get_payload_claim("exp");
        // Check if exp exists - we'll check its value directly
        try {
            exp.as_integer(); // This will throw if exp is missing or invalid
        } catch (...) {
            throw SecurityException("Token missing or invalid expiration");
        }
        
        const auto exp_time = std::chrono::system_clock::from_time_t(
            exp.as_integer());
        if (std::chrono::system_clock::now() > exp_time) {
            throw SecurityException("Token expired");
        }
        
        // Extract claims
        const auto user_id = decoded.get_payload_claim("sub").as_string();
        
        // Get user roles
        std::vector<std::string> roles;
        const auto roles_claim = decoded.get_payload_claim("roles");
        try {
            auto roles_array = roles_claim.as_array();
            for (const auto& role : roles_array) {
                roles.push_back(jwt::traits::kazuho_picojson_adapter::as_string(role));
            }
        } catch (...) {
            // If roles array isn't found or is invalid, continue with empty roles
        }
        
        // Create security context
        SecurityContext context;
        context.user_id = user_id;
        context.roles = roles;
        context.auth_token = token;
        context.token_expiry = exp_time;
        
        return context;
    } catch (const jwt::error::token_verification_exception& e) {
        throw SecurityException(std::string("Token verification failed: ") + e.what());
    } catch (const std::exception& e) {
        throw SecurityException(std::string("Token validation error: ") + e.what());
    }
}

void SecurityManager::revoke_token(const std::string& token) {
    std::unique_lock<std::shared_mutex> lock(tokens_mutex_);
    revoked_tokens_.insert(token);
}

bool SecurityManager::can_submit_task(const SecurityContext& context, const std::string& task_type) {
    if (!context.is_valid()) {
        return false;
    }
    
    std::shared_lock<std::shared_mutex> lock(roles_mutex_);
    
    // Check for wildcard permission first
    for (const auto& role : context.roles) {
        auto it = role_permissions_.find(role);
        if (it != role_permissions_.end()) {
            const auto& permissions = it->second;
            
            if (permissions.find("submit_task:*") != permissions.end()) {
                return true;
            }
        }
    }
    
    // Check for specific task type permission
    std::string specific_permission = "submit_task:" + task_type;
    for (const auto& role : context.roles) {
        auto it = role_permissions_.find(role);
        if (it != role_permissions_.end()) {
            const auto& permissions = it->second;
            
            if (permissions.find(specific_permission) != permissions.end()) {
                return true;
            }
        }
    }
    
    return false;
}

bool SecurityManager::can_cancel_task(const SecurityContext& context, const std::string& /*task_id*/) {
    if (!context.is_valid()) {
        return false;
    }
    
    std::shared_lock<std::shared_mutex> lock(roles_mutex_);
    
    // Check for wildcard permission first
    for (const auto& role : context.roles) {
        auto it = role_permissions_.find(role);
        if (it != role_permissions_.end()) {
            const auto& permissions = it->second;
            
            if (permissions.find("cancel_task:*") != permissions.end()) {
                return true;
            }
        }
    }
    
    // For ownership-based permissions, we would need task ownership information
    // For this example, we'll assume all tasks with "own" permission are allowed
    // In a real implementation, you would check if the task was created by this user
    for (const auto& role : context.roles) {
        auto it = role_permissions_.find(role);
        if (it != role_permissions_.end()) {
            const auto& permissions = it->second;
            
            if (permissions.find("cancel_task:own") != permissions.end()) {
                // Here we would check if the task belongs to the user
                // For simplicity, we'll allow it
                return true;
            }
        }
    }
    
    return false;
}

bool SecurityManager::can_view_task(const SecurityContext& context, const std::string& /*task_id*/) {
    if (!context.is_valid()) {
        return false;
    }
    
    std::shared_lock<std::shared_mutex> lock(roles_mutex_);
    
    // Check for wildcard permission first
    for (const auto& role : context.roles) {
        auto it = role_permissions_.find(role);
        if (it != role_permissions_.end()) {
            const auto& permissions = it->second;
            
            if (permissions.find("view_task:*") != permissions.end()) {
                return true;
            }
        }
    }
    
    // For ownership-based permissions, we would need task ownership information
    // Similar to cancel_task:own
    for (const auto& role : context.roles) {
        auto it = role_permissions_.find(role);
        if (it != role_permissions_.end()) {
            const auto& permissions = it->second;
            
            if (permissions.find("view_task:own") != permissions.end()) {
                // Here we would check if the task belongs to the user
                // For simplicity, we'll allow it
                return true;
            }
        }
    }
    
    return false;
}

bool SecurityManager::can_view_system_stats(const SecurityContext& context) {
    if (!context.is_valid()) {
        return false;
    }
    
    std::shared_lock<std::shared_mutex> lock(roles_mutex_);
    
    for (const auto& role : context.roles) {
        auto it = role_permissions_.find(role);
        if (it != role_permissions_.end()) {
            const auto& permissions = it->second;
            
            if (permissions.find("view_stats") != permissions.end()) {
                return true;
            }
        }
    }
    
    return false;
}

void SecurityManager::add_role(const std::string& role) {
    std::unique_lock<std::shared_mutex> lock(roles_mutex_);
    role_permissions_[role]; // Create an empty set of permissions
}

void SecurityManager::add_permission(const std::string& role, const std::string& permission) {
    std::unique_lock<std::shared_mutex> lock(roles_mutex_);
    role_permissions_[role].insert(permission);
}

void SecurityManager::assign_role_to_user(const std::string& username, const std::string& role) {
    std::unique_lock<std::shared_mutex> lock(users_mutex_);
    
    auto it = users_.find(username);
    if (it != users_.end()) {
        auto& roles = it->second.roles;
        if (std::find(roles.begin(), roles.end(), role) == roles.end()) {
            roles.push_back(role);
        }
    }
}

void SecurityManager::add_user(const std::string& username, const std::string& password, 
                             const std::vector<std::string>& roles) {
    std::unique_lock<std::shared_mutex> lock(users_mutex_);
    
    // Generate salt for password hashing
    std::string salt = generate_salt();
    
    // Create user info
    UserInfo user_info;
    user_info.salt = salt;
    user_info.password_hash = hash_password(password, salt);
    user_info.roles = roles;
    
    // Add user
    users_[username] = std::move(user_info);
}

void SecurityManager::remove_user(const std::string& username) {
    std::unique_lock<std::shared_mutex> lock(users_mutex_);
    users_.erase(username);
}

std::string SecurityManager::hash_password(const std::string& password, const std::string& salt) {
    // In a real implementation, use a strong hashing algorithm like bcrypt or Argon2
    // For this example, we'll use a simple SHA-256 hash through the EVP interface
    // which is not deprecated in OpenSSL 3.0
    
    std::string salted_password = password + salt;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    // Use EVP digest functions instead of deprecated SHA256_* functions
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create message digest context");
    }
    
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize digest");
    }
    
    if (EVP_DigestUpdate(mdctx, salted_password.c_str(), salted_password.length()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update digest");
    }
    
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &digest_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize digest");
    }
    
    EVP_MD_CTX_free(mdctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < digest_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

std::string SecurityManager::generate_salt() {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const size_t salt_length = 16;
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, sizeof(charset) - 2);
    
    std::string salt;
    salt.reserve(salt_length);
    
    for (size_t i = 0; i < salt_length; ++i) {
        salt += charset[distribution(generator)];
    }
    
    return salt;
}

std::string SecurityManager::generate_token(const std::string& username, const std::vector<std::string>& roles) {
    // Create JWT token using our custom adapter
    auto token = jwt::create<jwt::traits::kazuho_picojson_adapter>()
        .set_issuer("distributed_scheduler")
        .set_subject(username)
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + token_validity_);
    
    // Add roles as a claim - convert to picojson array
    std::vector<std::string> role_strings = roles;
    picojson::value json_roles(picojson::array{});
    auto& roles_array = json_roles.get<picojson::array>();
    for (const auto& role : role_strings) {
        roles_array.push_back(picojson::value(role));
    }
    token.set_payload_claim("roles", json_roles);
    
    // Sign token with the secret key
    return token.sign(jwt::algorithm::hs256{jwt_secret_key_});
}

bool SecurityManager::is_token_valid(const std::string& token) {
    std::shared_lock<std::shared_mutex> lock(tokens_mutex_);
    return revoked_tokens_.find(token) == revoked_tokens_.end();
}

// SecureMessaging implementation
SecureMessaging::SecureMessaging(boost::asio::io_context& /*io_context*/,
                               const std::string& /*cert_file*/,
                               const std::string& key_file)
    : ssl_context_(boost::asio::ssl::context::tlsv12) {
    // In a real implementation, this would load the certificates and keys
    // For this example, we'll just store the paths
    private_key_ = key_file;
    
    // Generate a dummy public key for the example
    public_key_ = "EXAMPLE_PUBLIC_KEY_" + key_file;
}

boost::asio::ssl::context& SecureMessaging::get_ssl_context() {
    return ssl_context_;
}

std::vector<uint8_t> SecureMessaging::encrypt_message(const std::vector<uint8_t>& message,
                                                     const std::string& /*recipient_public_key*/) {
    // In a real implementation, this would encrypt the message using the recipient's public key
    // For this example, we'll just return the original message
    return message;
}

std::vector<uint8_t> SecureMessaging::decrypt_message(const std::vector<uint8_t>& encrypted_message) {
    // In a real implementation, this would decrypt the message using our private key
    // For this example, we'll just return the original message
    return encrypted_message;
}

std::vector<uint8_t> SecureMessaging::sign_message(const std::vector<uint8_t>& /*message*/) {
    // In a real implementation, this would sign the message using our private key
    // For this example, we'll just return a dummy signature
    return {0x01, 0x02, 0x03, 0x04};
}

bool SecureMessaging::verify_signature(const std::vector<uint8_t>& /*message*/,
                                      const std::vector<uint8_t>& /*signature*/,
                                      const std::string& /*sender_public_key*/) {
    // In a real implementation, this would verify the signature using the sender's public key
    // For this example, we'll just return true
    return true;
}

} // namespace distributed_scheduler