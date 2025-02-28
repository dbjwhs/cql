# CQL Real-World Examples

This document provides real-world examples of using CQL for practical code generation tasks. Each example includes the template, variables, and expected output.

## Table of Contents

1. [Thread-Safe Queue](#thread-safe-queue)
2. [REST API Handler](#rest-api-handler)
3. [Database Model](#database-model)
4. [Config Parser](#config-parser)
5. [Design Pattern Templates](#design-pattern-templates)
6. [Multi-File Generation](#multi-file-generation)

## Thread-Safe Queue

A thread-safe queue implementation with configurable size and type.

### Template

```cql
@copyright "MIT License" "2025 Author"
@language "C++"
@description "implement a thread-safe queue with configurable size and element type"
@variable "element_type" "int"
@variable "max_size" "1000"
@variable "namespace" "concurrency"
@architecture "Producer-consumer pattern with RAII locks"
@constraint "Thread-safe for concurrent access"
@security "Prevent data races and deadlocks"
@complexity "O(1) for push and pop operations"
@dependency "std::mutex, std::condition_variable, std::queue"
@test "Test concurrent push operations"
@test "Test concurrent pop operations"
@test "Test boundary conditions with ${max_size} elements"
@example "Basic Usage" "
    // Create a queue with default size
    ${namespace}::ThreadSafeQueue<${element_type}> queue;
    
    // Push and pop elements
    queue.push(42);
    auto value = queue.try_pop();
    
    if (value) {
        std::cout << *value << std::endl;
    }
"
```

### Instantiation

```
instantiate thread_safe_queue element_type=std::string max_size=100 namespace=app::utils
```

### Output

```cpp
/**
 * MIT License
 * Copyright (c) 2025 Author
 */

#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>
#include <string>

namespace app::utils {

/**
 * @brief A thread-safe queue implementation
 * 
 * This queue is designed for concurrent access with the following characteristics:
 * - Uses Producer-consumer pattern with RAII locks
 * - Thread-safe for concurrent access
 * - Prevents data races and deadlocks
 * - O(1) for push and pop operations
 * 
 * @tparam T The element type stored in the queue
 */
template <typename T = std::string>
class ThreadSafeQueue {
public:
    /**
     * @brief Construct a new ThreadSafeQueue
     * @param max_size Maximum number of elements the queue can hold
     */
    explicit ThreadSafeQueue(size_t max_size = 100)
        : m_max_size(max_size) {}
    
    /**
     * @brief Destroy the ThreadSafeQueue
     */
    ~ThreadSafeQueue() = default;
    
    /**
     * @brief Push an element onto the queue
     * 
     * If the queue is full, this operation will block until space is available.
     * 
     * @param value The value to push
     */
    void push(T value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_not_full.wait(lock, [this] { return m_queue.size() < m_max_size; });
        
        m_queue.push(std::move(value));
        lock.unlock();
        m_not_empty.notify_one();
    }
    
    /**
     * @brief Try to push an element without blocking
     * 
     * @param value The value to push
     * @return true if the element was pushed, false if the queue was full
     */
    bool try_push(T value) {
        std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
        if (!lock || m_queue.size() >= m_max_size) {
            return false;
        }
        
        m_queue.push(std::move(value));
        lock.unlock();
        m_not_empty.notify_one();
        return true;
    }
    
    /**
     * @brief Pop an element from the queue
     * 
     * If the queue is empty, this operation will block until an element is available.
     * 
     * @return The popped element
     */
    T pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_not_empty.wait(lock, [this] { return !m_queue.empty(); });
        
        T value = std::move(m_queue.front());
        m_queue.pop();
        lock.unlock();
        m_not_full.notify_one();
        return value;
    }
    
    /**
     * @brief Try to pop an element without blocking
     * 
     * @return An optional containing the popped element, or empty if the queue was empty
     */
    std::optional<T> try_pop() {
        std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
        if (!lock || m_queue.empty()) {
            return std::nullopt;
        }
        
        T value = std::move(m_queue.front());
        m_queue.pop();
        lock.unlock();
        m_not_full.notify_one();
        return value;
    }
    
    /**
     * @brief Check if the queue is empty
     * 
     * @return true if the queue is empty, false otherwise
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    
    /**
     * @brief Get the current size of the queue
     * 
     * @return The number of elements in the queue
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    
    /**
     * @brief Get the maximum size of the queue
     * 
     * @return The maximum number of elements the queue can hold
     */
    size_t max_size() const {
        return m_max_size;
    }
    
private:
    mutable std::mutex m_mutex;
    std::condition_variable m_not_empty;
    std::condition_variable m_not_full;
    std::queue<T> m_queue;
    const size_t m_max_size;
};

} // namespace app::utils
```

## REST API Handler

A REST API handler template for a web service.

### Template

```cql
@copyright "MIT License" "2025 Author"
@language "C++"
@description "implement a REST API handler for ${resource}"
@variable "resource" "users"
@variable "namespace" "api"
@variable "auth_required" "true"
@dependency "nlohmann/json.hpp, httplib.h"
@constraint "RESTful API design principles"
@security "Input validation, authentication"
@test "Test GET /${resource}"
@test "Test GET /${resource}/{id}"
@test "Test POST /${resource}"
@test "Test PUT /${resource}/{id}"
@test "Test DELETE /${resource}/{id}"
@example "Register Handler" "
    httplib::Server server;
    ${namespace}::${resource}::Handler handler;
    handler.register_routes(server);
    server.listen(\"0.0.0.0\", 8080);
"
```

### Instantiation

```
instantiate rest_api resource=products namespace=shop auth_required=true
```

### Output

```cpp
/**
 * MIT License
 * Copyright (c) 2025 Author
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <httplib.h>

namespace shop {
namespace products {

using json = nlohmann::json;

/**
 * @brief Product model class
 */
class Product {
public:
    int id;
    std::string name;
    std::string description;
    double price;
    int stock;
    
    json to_json() const {
        return {
            {"id", id},
            {"name", name},
            {"description", description},
            {"price", price},
            {"stock", stock}
        };
    }
    
    static Product from_json(const json& j) {
        Product product;
        product.id = j.at("id").get<int>();
        product.name = j.at("name").get<std::string>();
        product.description = j.at("description").get<std::string>();
        product.price = j.at("price").get<double>();
        product.stock = j.at("stock").get<int>();
        return product;
    }
};

/**
 * @brief Products repository
 */
class ProductRepository {
public:
    std::vector<Product> find_all() {
        // Implementation would typically use a database
        return m_products;
    }
    
    std::optional<Product> find_by_id(int id) {
        for (const auto& product : m_products) {
            if (product.id == id) {
                return product;
            }
        }
        return std::nullopt;
    }
    
    Product create(const Product& product) {
        // In a real implementation, this would generate an ID
        Product new_product = product;
        new_product.id = next_id++;
        m_products.push_back(new_product);
        return new_product;
    }
    
    bool update(const Product& product) {
        for (auto& p : m_products) {
            if (p.id == product.id) {
                p = product;
                return true;
            }
        }
        return false;
    }
    
    bool remove(int id) {
        auto it = std::remove_if(m_products.begin(), m_products.end(),
            [id](const Product& p) { return p.id == id; });
        
        if (it == m_products.end()) {
            return false;
        }
        
        m_products.erase(it, m_products.end());
        return true;
    }
    
private:
    std::vector<Product> m_products;
    int next_id = 1;
};

/**
 * @brief Authentication middleware
 */
class AuthMiddleware {
public:
    bool authenticate(const httplib::Request& req) {
        // Get the Authorization header
        auto auth_header = req.get_header_value("Authorization");
        if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
            return false;
        }
        
        // Extract the token
        std::string token = auth_header.substr(7);
        
        // Validate the token (in a real system, this would check against a token store)
        return validate_token(token);
    }
    
private:
    bool validate_token(const std::string& token) {
        // In a real system, this would validate the token
        // For this example, we'll accept any non-empty token
        return !token.empty();
    }
};

/**
 * @brief Handler for product-related REST endpoints
 */
class Handler {
public:
    Handler() = default;
    
    /**
     * @brief Register all routes with the server
     * 
     * @param server The HTTP server to register routes with
     */
    void register_routes(httplib::Server& server) {
        // GET /products
        server.Get("/products", [this](const httplib::Request& req, httplib::Response& res) {
            if (!authenticate(req, res)) return;
            
            get_all(req, res);
        });
        
        // GET /products/{id}
        server.Get(R"(/products/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            if (!authenticate(req, res)) return;
            
            get_by_id(req, res);
        });
        
        // POST /products
        server.Post("/products", [this](const httplib::Request& req, httplib::Response& res) {
            if (!authenticate(req, res)) return;
            
            create(req, res);
        });
        
        // PUT /products/{id}
        server.Put(R"(/products/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            if (!authenticate(req, res)) return;
            
            update(req, res);
        });
        
        // DELETE /products/{id}
        server.Delete(R"(/products/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            if (!authenticate(req, res)) return;
            
            remove(req, res);
        });
    }
    
private:
    ProductRepository m_repository;
    AuthMiddleware m_auth;
    
    bool authenticate(const httplib::Request& req, httplib::Response& res) {
        if (!m_auth.authenticate(req)) {
            res.status = 401;
            res.set_content("{\"error\":\"Unauthorized\"}", "application/json");
            return false;
        }
        return true;
    }
    
    void get_all(const httplib::Request& req, httplib::Response& res) {
        auto products = m_repository.find_all();
        
        json response = json::array();
        for (const auto& product : products) {
            response.push_back(product.to_json());
        }
        
        res.set_content(response.dump(), "application/json");
    }
    
    void get_by_id(const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            auto product = m_repository.find_by_id(id);
            
            if (!product) {
                res.status = 404;
                res.set_content("{\"error\":\"Product not found\"}", "application/json");
                return;
            }
            
            res.set_content(product->to_json().dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid product ID\"}", "application/json");
        }
    }
    
    void create(const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            auto product = Product::from_json(body);
            
            auto created = m_repository.create(product);
            
            res.status = 201;
            res.set_content(created.to_json().dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid product data\"}", "application/json");
        }
    }
    
    void update(const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            auto body = json::parse(req.body);
            
            auto product = Product::from_json(body);
            product.id = id;
            
            bool success = m_repository.update(product);
            
            if (!success) {
                res.status = 404;
                res.set_content("{\"error\":\"Product not found\"}", "application/json");
                return;
            }
            
            res.set_content(product.to_json().dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid product data\"}", "application/json");
        }
    }
    
    void remove(const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            
            bool success = m_repository.remove(id);
            
            if (!success) {
                res.status = 404;
                res.set_content("{\"error\":\"Product not found\"}", "application/json");
                return;
            }
            
            res.status = 204;
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid product ID\"}", "application/json");
        }
    }
};

} // namespace products
} // namespace shop
```

## Database Model

A database model with ORM capabilities.

### Template

```cql
@copyright "MIT License" "2025 Author"
@language "C++"
@description "implement a database model for ${entity} with ORM capabilities"
@variable "entity" "User"
@variable "fields" "id:int64_t,name:std::string,email:std::string,created_at:std::time_t"
@variable "table_name" "users"
@variable "namespace" "db::models"
@dependency "sqlite3.h, <optional>, <vector>, <string>"
@constraint "CRUD operations"
@security "SQL injection prevention"
@test "Test create operations"
@test "Test read operations"
@test "Test update operations"
@test "Test delete operations"
@test "Test find by field operations"
@example "Basic Usage" "
    // Create a new user
    ${namespace}::${entity} user;
    user.name = \"John Doe\";
    user.email = \"john@example.com\";
    user.save();
    
    // Find a user by ID
    auto found = ${namespace}::${entity}::find_by_id(1);
    if (found) {
        std::cout << \"Found: \" << found->name << std::endl;
    }
    
    // Update a user
    found->name = \"Jane Doe\";
    found->save();
    
    // Delete a user
    found->remove();
"
```

### Instantiation

```
instantiate db_model entity=Product fields=id:int64_t,name:std::string,price:double,category:std::string,in_stock:bool table_name=products namespace=shop::db
```

### Output

```cpp
/**
 * MIT License
 * Copyright (c) 2025 Author
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <ctime>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

namespace shop::db {

/**
 * @brief Database connection manager
 */
class Connection {
public:
    static Connection& get_instance() {
        static Connection instance;
        return instance;
    }
    
    sqlite3* get_handle() {
        return m_db;
    }
    
private:
    Connection() {
        int rc = sqlite3_open("database.db", &m_db);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Cannot open database");
        }
        
        // Create tables if they don't exist
        const char* create_products_table = 
            "CREATE TABLE IF NOT EXISTS products ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "price REAL NOT NULL,"
            "category TEXT NOT NULL,"
            "in_stock INTEGER NOT NULL"
            ");";
        
        char* error_msg = nullptr;
        rc = sqlite3_exec(m_db, create_products_table, nullptr, nullptr, &error_msg);
        
        if (rc != SQLITE_OK) {
            std::string error(error_msg);
            sqlite3_free(error_msg);
            throw std::runtime_error("SQL error: " + error);
        }
    }
    
    ~Connection() {
        if (m_db) {
            sqlite3_close(m_db);
        }
    }
    
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    
    sqlite3* m_db = nullptr;
};

/**
 * @brief Product model with ORM capabilities
 */
class Product {
public:
    // Fields
    int64_t id = 0;
    std::string name;
    double price = 0.0;
    std::string category;
    bool in_stock = false;
    
    /**
     * @brief Default constructor
     */
    Product() = default;
    
    /**
     * @brief Find a product by ID
     * 
     * @param id The ID to look for
     * @return An optional containing the product if found
     */
    static std::optional<Product> find_by_id(int64_t id) {
        std::string query = "SELECT id, name, price, category, in_stock FROM products WHERE id = ?;";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(Connection::get_instance().get_handle(), query.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement");
        }
        
        sqlite3_bind_int64(stmt, 1, id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            Product product;
            product.id = sqlite3_column_int64(stmt, 0);
            product.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            product.price = sqlite3_column_double(stmt, 2);
            product.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            product.in_stock = sqlite3_column_int(stmt, 4) != 0;
            
            sqlite3_finalize(stmt);
            return product;
        }
        
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    
    /**
     * @brief Find products by a specific field
     * 
     * @param field The field name
     * @param value The value to search for
     * @return A vector of matching products
     */
    template <typename T>
    static std::vector<Product> find_by_field(const std::string& field, const T& value) {
        std::vector<Product> results;
        
        std::string query = "SELECT id, name, price, category, in_stock FROM products WHERE " + field + " = ?;";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(Connection::get_instance().get_handle(), query.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement");
        }
        
        bind_value(stmt, 1, value);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Product product;
            product.id = sqlite3_column_int64(stmt, 0);
            product.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            product.price = sqlite3_column_double(stmt, 2);
            product.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            product.in_stock = sqlite3_column_int(stmt, 4) != 0;
            
            results.push_back(product);
        }
        
        sqlite3_finalize(stmt);
        return results;
    }
    
    /**
     * @brief Find all products
     * 
     * @return A vector of all products
     */
    static std::vector<Product> find_all() {
        std::vector<Product> results;
        
        std::string query = "SELECT id, name, price, category, in_stock FROM products;";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(Connection::get_instance().get_handle(), query.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement");
        }
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Product product;
            product.id = sqlite3_column_int64(stmt, 0);
            product.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            product.price = sqlite3_column_double(stmt, 2);
            product.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            product.in_stock = sqlite3_column_int(stmt, 4) != 0;
            
            results.push_back(product);
        }
        
        sqlite3_finalize(stmt);
        return results;
    }
    
    /**
     * @brief Save the product
     * 
     * If the product has an ID, it will be updated; otherwise, it will be created.
     * 
     * @return true if successful, false otherwise
     */
    bool save() {
        if (id > 0) {
            return update();
        } else {
            return create();
        }
    }
    
    /**
     * @brief Remove the product
     * 
     * @return true if successful, false otherwise
     */
    bool remove() {
        if (id == 0) {
            return false;
        }
        
        std::string query = "DELETE FROM products WHERE id = ?;";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(Connection::get_instance().get_handle(), query.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_int64(stmt, 1, id);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        return rc == SQLITE_DONE;
    }
    
private:
    bool create() {
        std::string query = 
            "INSERT INTO products (name, price, category, in_stock) "
            "VALUES (?, ?, ?, ?);";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(Connection::get_instance().get_handle(), query.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 2, price);
        sqlite3_bind_text(stmt, 3, category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, in_stock ? 1 : 0);
        
        rc = sqlite3_step(stmt);
        
        if (rc == SQLITE_DONE) {
            id = sqlite3_last_insert_rowid(Connection::get_instance().get_handle());
            sqlite3_finalize(stmt);
            return true;
        }
        
        sqlite3_finalize(stmt);
        return false;
    }
    
    bool update() {
        std::string query = 
            "UPDATE products SET name = ?, price = ?, category = ?, in_stock = ? "
            "WHERE id = ?;";
        
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(Connection::get_instance().get_handle(), query.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 2, price);
        sqlite3_bind_text(stmt, 3, category.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, in_stock ? 1 : 0);
        sqlite3_bind_int64(stmt, 5, id);
        
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        return rc == SQLITE_DONE;
    }
    
    // Helper for binding different types to SQLite statements
    template <typename T>
    static void bind_value(sqlite3_stmt* stmt, int index, const T& value) {
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool>) {
            sqlite3_bind_int(stmt, index, value);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            sqlite3_bind_int64(stmt, index, value);
        } else if constexpr (std::is_same_v<T, double>) {
            sqlite3_bind_double(stmt, index, value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_STATIC);
        } else {
            throw std::runtime_error("Unsupported type for binding");
        }
    }
};

} // namespace shop::db
```

## Config Parser

A configuration file parser.

### Template

```cql
@copyright "MIT License" "2025 Author"
@language "C++"
@description "implement a configuration parser for ${format} files"
@variable "format" "INI"
@variable "namespace" "utils"
@variable "case_sensitive" "false"
@dependency "<unordered_map>, <fstream>, <string>, <optional>"
@constraint "Support sections and comments"
@security "Handle malformed input"
@test "Test parsing valid files"
@test "Test handling missing files"
@test "Test handling syntax errors"
@example "Basic Usage" "
    ${namespace}::${format}Config config;
    bool success = config.load(\"config.ini\");
    
    if (success) {
        std::string value = config.get(\"section\", \"key\", \"default_value\");
        int number = config.get_int(\"section\", \"number\", 0);
        bool flag = config.get_bool(\"section\", \"flag\", false);
    }
"
```

### Instantiation

```
instantiate config_parser format=YAML namespace=config case_sensitive=true
```

### Output

```cpp
/**
 * MIT License
 * Copyright (c) 2025 Author
 */

#pragma once

#include <unordered_map>
#include <fstream>
#include <string>
#include <optional>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>

namespace config {

/**
 * @brief YAML configuration file parser
 * 
 * Parses YAML configuration files with support for:
 * - Sections and nested sections
 * - Comments (lines starting with #)
 * - Key-value pairs
 * - Case-sensitive keys
 */
class YAMLConfig {
public:
    /**
     * @brief Default constructor
     */
    YAMLConfig() = default;
    
    /**
     * @brief Load a configuration from a file
     * 
     * @param filename The path to the configuration file
     * @return true if loaded successfully, false otherwise
     */
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            m_last_error = "Failed to open file: " + filename;
            return false;
        }
        
        return parse(file);
    }
    
    /**
     * @brief Load a configuration from a string
     * 
     * @param content The configuration content
     * @return true if loaded successfully, false otherwise
     */
    bool load_from_string(const std::string& content) {
        std::istringstream stream(content);
        return parse(stream);
    }
    
    /**
     * @brief Get a string value from the configuration
     * 
     * @param section The section name
     * @param key The key name
     * @param default_value The default value to return if not found
     * @return The value or default_value if not found
     */
    std::string get(const std::string& section, const std::string& key, const std::string& default_value = "") const {
        auto it = m_sections.find(section);
        if (it == m_sections.end()) {
            return default_value;
        }
        
        auto key_it = it->second.find(key);
        if (key_it == it->second.end()) {
            return default_value;
        }
        
        return key_it->second;
    }
    
    /**
     * @brief Get an integer value from the configuration
     * 
     * @param section The section name
     * @param key The key name
     * @param default_value The default value to return if not found
     * @return The value or default_value if not found or not convertible
     */
    int get_int(const std::string& section, const std::string& key, int default_value = 0) const {
        std::string value = get(section, key, "");
        if (value.empty()) {
            return default_value;
        }
        
        try {
            return std::stoi(value);
        } catch (const std::exception&) {
            return default_value;
        }
    }
    
    /**
     * @brief Get a double value from the configuration
     * 
     * @param section The section name
     * @param key The key name
     * @param default_value The default value to return if not found
     * @return The value or default_value if not found or not convertible
     */
    double get_double(const std::string& section, const std::string& key, double default_value = 0.0) const {
        std::string value = get(section, key, "");
        if (value.empty()) {
            return default_value;
        }
        
        try {
            return std::stod(value);
        } catch (const std::exception&) {
            return default_value;
        }
    }
    
    /**
     * @brief Get a boolean value from the configuration
     * 
     * @param section The section name
     * @param key The key name
     * @param default_value The default value to return if not found
     * @return The value or default_value if not found or not convertible
     */
    bool get_bool(const std::string& section, const std::string& key, bool default_value = false) const {
        std::string value = get(section, key, "");
        if (value.empty()) {
            return default_value;
        }
        
        // Convert to lowercase for comparison
        std::string lower_value = value;
        std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(),
                      [](unsigned char c){ return std::tolower(c); });
        
        if (lower_value == "true" || lower_value == "yes" || lower_value == "1") {
            return true;
        } else if (lower_value == "false" || lower_value == "no" || lower_value == "0") {
            return false;
        }
        
        return default_value;
    }
    
    /**
     * @brief Get a list of values from the configuration
     * 
     * @param section The section name
     * @param key The key name
     * @return A vector of values or empty vector if not found
     */
    std::vector<std::string> get_list(const std::string& section, const std::string& key) const {
        std::string value = get(section, key, "");
        if (value.empty()) {
            return {};
        }
        
        std::vector<std::string> result;
        std::string item;
        bool in_quotes = false;
        char quote_char = 0;
        
        for (size_t i = 0; i < value.size(); ++i) {
            char c = value[i];
            
            if ((c == '"' || c == '\'') && (i == 0 || value[i-1] != '\\')) {
                if (!in_quotes) {
                    in_quotes = true;
                    quote_char = c;
                } else if (c == quote_char) {
                    in_quotes = false;
                } else {
                    item += c;
                }
            } else if (c == ',' && !in_quotes) {
                result.push_back(trim(item));
                item.clear();
            } else {
                item += c;
            }
        }
        
        if (!item.empty()) {
            result.push_back(trim(item));
        }
        
        return result;
    }
    
    /**
     * @brief Get all sections in the configuration
     * 
     * @return A vector of section names
     */
    std::vector<std::string> get_sections() const {
        std::vector<std::string> sections;
        for (const auto& section : m_sections) {
            sections.push_back(section.first);
        }
        return sections;
    }
    
    /**
     * @brief Get all keys in a section
     * 
     * @param section The section name
     * @return A vector of key names
     */
    std::vector<std::string> get_keys(const std::string& section) const {
        std::vector<std::string> keys;
        
        auto it = m_sections.find(section);
        if (it != m_sections.end()) {
            for (const auto& key : it->second) {
                keys.push_back(key.first);
            }
        }
        
        return keys;
    }
    
    /**
     * @brief Set a value in the configuration
     * 
     * @param section The section name
     * @param key The key name
     * @param value The value to set
     */
    void set(const std::string& section, const std::string& key, const std::string& value) {
        m_sections[section][key] = value;
    }
    
    /**
     * @brief Save the configuration to a file
     * 
     * @param filename The path to the file
     * @return true if saved successfully, false otherwise
     */
    bool save(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        for (const auto& section : m_sections) {
            file << section.first << ":" << std::endl;
            
            for (const auto& key : section.second) {
                file << "  " << key.first << ": " << key.second << std::endl;
            }
            
            file << std::endl;
        }
        
        return true;
    }
    
    /**
     * @brief Get the last error message
     * 
     * @return The last error message
     */
    std::string get_last_error() const {
        return m_last_error;
    }
    
private:
    using Section = std::unordered_map<std::string, std::string>;
    std::unordered_map<std::string, Section> m_sections;
    std::string m_last_error;
    
    /**
     * @brief Parse a configuration from a stream
     * 
     * @param stream The input stream
     * @return true if parsed successfully, false otherwise
     */
    bool parse(std::istream& stream) {
        m_sections.clear();
        m_last_error.clear();
        
        std::string line;
        std::string current_section;
        int indent_level = 0;
        int line_number = 0;
        
        while (std::getline(stream, line)) {
            line_number++;
            line = trim(line);
            
            if (line.empty() || line[0] == '#') {
                continue; // Skip empty lines and comments
            }
            
            // Get indentation level
            size_t indent = 0;
            while (indent < line.size() && (line[indent] == ' ' || line[indent] == '\t')) {
                indent++;
            }
            
            if (indent > 0) {
                line = line.substr(indent);
            }
            
            if (line.empty()) {
                continue;
            }
            
            // Check if this is a section
            if (line.back() == ':') {
                current_section = line.substr(0, line.size() - 1);
                current_section = trim(current_section);
                continue;
            }
            
            // Parse key-value pair
            size_t colon_pos = line.find(':');
            if (colon_pos == std::string::npos) {
                m_last_error = "Invalid line format at line " + std::to_string(line_number) + ": " + line;
                return false;
            }
            
            std::string key = trim(line.substr(0, colon_pos));
            std::string value = trim(line.substr(colon_pos + 1));
            
            if (key.empty()) {
                m_last_error = "Empty key at line " + std::to_string(line_number);
                return false;
            }
            
            // Store the key-value pair in the current section
            m_sections[current_section][key] = value;
        }
        
        return true;
    }
    
    /**
     * @brief Trim whitespace from the beginning and end of a string
     * 
     * @param str The string to trim
     * @return The trimmed string
     */
    static std::string trim(const std::string& str) {
        const auto begin = std::find_if_not(str.begin(), str.end(), [](unsigned char c) {
            return std::isspace(c);
        });
        
        const auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
            return std::isspace(c);
        }).base();
        
        return (begin < end) ? std::string(begin, end) : std::string();
    }
};

} // namespace config
```

## Design Pattern Templates

A collection of templates for common design patterns.

### Template

```cql
@copyright "MIT License" "2025 Author"
@language "C++"
@description "implement the ${pattern} design pattern"
@variable "pattern" "Observer"
@variable "subject" "Subject"
@variable "observer" "Observer"
@variable "namespace" "patterns"
@dependency "<vector>, <algorithm>, <memory>"
@constraint "Standard design pattern implementation"
@security "Thread-safe notifications"
@test "Test attaching observers"
@test "Test detaching observers"
@test "Test notifying observers"
@example "Basic Usage" "
    ${namespace}::Concrete${subject} subject;
    auto observer1 = std::make_shared<${namespace}::Concrete${observer}>(\"Observer 1\");
    auto observer2 = std::make_shared<${namespace}::Concrete${observer}>(\"Observer 2\");
    
    subject.attach(observer1);
    subject.attach(observer2);
    
    subject.set_state(10);
    subject.notify();
"
```

### Instantiation

```
instantiate design_pattern pattern=Strategy subject=Context observer=Strategy namespace=design
```

### Output

```cpp
/**
 * MIT License
 * Copyright (c) 2025 Author
 */

#pragma once

#include <vector>
#include <algorithm>
#include <memory>
#include <string>
#include <iostream>

namespace design {

/**
 * @brief Strategy interface
 * 
 * Declares the interface common to all supported algorithms.
 * Context uses this interface to call the algorithm defined by ConcreteStrategy.
 */
class Strategy {
public:
    virtual ~Strategy() = default;
    
    /**
     * @brief Pure virtual method to execute the algorithm
     * 
     * @param data The data to process with the strategy
     * @return Result of the algorithm execution
     */
    virtual int execute(const std::vector<int>& data) const = 0;
};

/**
 * @brief Concrete Strategy implementing the first algorithm
 */
class ConcreteStrategyA : public Strategy {
public:
    /**
     * @brief Constructor with strategy name
     * 
     * @param name The name of this strategy
     */
    explicit ConcreteStrategyA(const std::string& name) : m_name(name) {}
    
    /**
     * @brief Execute the algorithm - finds the sum of all elements
     * 
     * @param data The data to process
     * @return The sum of all elements
     */
    int execute(const std::vector<int>& data) const override {
        int result = 0;
        for (int value : data) {
            result += value;
        }
        std::cout << m_name << " executed: Sum = " << result << std::endl;
        return result;
    }
    
private:
    std::string m_name;
};

/**
 * @brief Concrete Strategy implementing the second algorithm
 */
class ConcreteStrategyB : public Strategy {
public:
    /**
     * @brief Constructor with strategy name
     * 
     * @param name The name of this strategy
     */
    explicit ConcreteStrategyB(const std::string& name) : m_name(name) {}
    
    /**
     * @brief Execute the algorithm - finds the average of all elements
     * 
     * @param data The data to process
     * @return The average of all elements (or 0 if empty)
     */
    int execute(const std::vector<int>& data) const override {
        if (data.empty()) {
            return 0;
        }
        
        int sum = 0;
        for (int value : data) {
            sum += value;
        }
        int avg = sum / static_cast<int>(data.size());
        std::cout << m_name << " executed: Average = " << avg << std::endl;
        return avg;
    }
    
private:
    std::string m_name;
};

/**
 * @brief Concrete Strategy implementing the third algorithm
 */
class ConcreteStrategyC : public Strategy {
public:
    /**
     * @brief Constructor with strategy name
     * 
     * @param name The name of this strategy
     */
    explicit ConcreteStrategyC(const std::string& name) : m_name(name) {}
    
    /**
     * @brief Execute the algorithm - finds the maximum element
     * 
     * @param data The data to process
     * @return The maximum element (or 0 if empty)
     */
    int execute(const std::vector<int>& data) const override {
        if (data.empty()) {
            return 0;
        }
        
        int max = *std::max_element(data.begin(), data.end());
        std::cout << m_name << " executed: Max = " << max << std::endl;
        return max;
    }
    
private:
    std::string m_name;
};

/**
 * @brief Context class that uses a Strategy
 * 
 * Maintains a reference to a Strategy object and defines an interface
 * that lets the strategy access its data.
 */
class Context {
public:
    /**
     * @brief Default constructor
     */
    Context() = default;
    
    /**
     * @brief Constructor with initial strategy
     * 
     * @param strategy The initial strategy to use
     */
    explicit Context(std::shared_ptr<Strategy> strategy) : m_strategy(std::move(strategy)) {}
    
    /**
     * @brief Set the strategy to use
     * 
     * @param strategy The strategy to use
     */
    void set_strategy(std::shared_ptr<Strategy> strategy) {
        m_strategy = std::move(strategy);
    }
    
    /**
     * @brief Execute the current strategy
     * 
     * @return Result from the strategy execution
     */
    int execute_strategy() const {
        if (!m_strategy) {
            throw std::runtime_error("No strategy set");
        }
        
        return m_strategy->execute(m_data);
    }
    
    /**
     * @brief Add data to be processed by the strategy
     * 
     * @param value The value to add
     */
    void add_data(int value) {
        m_data.push_back(value);
    }
    
    /**
     * @brief Set the data to be processed by the strategy
     * 
     * @param data The data to set
     */
    void set_data(const std::vector<int>& data) {
        m_data = data;
    }
    
    /**
     * @brief Clear all data
     */
    void clear_data() {
        m_data.clear();
    }
    
    /**
     * @brief Get the current data
     * 
     * @return The current data
     */
    const std::vector<int>& get_data() const {
        return m_data;
    }
    
private:
    std::shared_ptr<Strategy> m_strategy;
    std::vector<int> m_data;
};

} // namespace design
```

## Multi-File Generation

A template that generates multiple files for a complete component.

### Template

```cql
@copyright "MIT License" "2025 Author"
@language "C++"
@description "create a complete ${component_type} component with header and implementation"
@variable "component_type" "widget"
@variable "component_name" "Button"
@variable "namespace" "ui"
@dependency "<string>, <vector>, <functional>"
@constraint "SOLID principles"
@security "No raw pointers"
@test "Test component initialization"
@test "Test component usage"
@test "Test component events"
@example "Basic Usage" "
    ${namespace}::${component_name} ${component_type}(\"My${component_name}\");
    ${component_type}.set_enabled(true);
    ${component_type}.on_click([](const ${namespace}::ClickEvent& event) {
        std::cout << \"Clicked!\" << std::endl;
    });
"
```

### Instantiation

```
instantiate multi_file component_type=form component_name=LoginForm namespace=app::ui
```

### Output (Header)

```cpp
// LoginForm.hpp

/**
 * MIT License
 * Copyright (c) 2025 Author
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

namespace app::ui {

/**
 * @brief Event data for form submission
 */
struct SubmitEvent {
    std::string form_id;
    bool valid;
    std::unordered_map<std::string, std::string> field_values;
};

/**
 * @brief Event data for field changes
 */
struct FieldChangeEvent {
    std::string field_id;
    std::string old_value;
    std::string new_value;
    bool valid;
};

/**
 * @brief Form field base class
 */
class FormField {
public:
    using ChangeCallback = std::function<void(const FieldChangeEvent&)>;
    
    explicit FormField(std::string id, std::string label);
    virtual ~FormField() = default;
    
    const std::string& get_id() const;
    const std::string& get_label() const;
    
    virtual std::string get_value() const = 0;
    virtual void set_value(const std::string& value) = 0;
    
    bool is_required() const;
    void set_required(bool required);
    
    bool is_valid() const;
    const std::string& get_error() const;
    
    void add_change_listener(ChangeCallback callback);
    
protected:
    void notify_change(const std::string& old_value, const std::string& new_value);
    virtual bool validate() = 0;
    void set_error(const std::string& error);
    
private:
    std::string m_id;
    std::string m_label;
    bool m_required = false;
    bool m_valid = true;
    std::string m_error;
    std::vector<ChangeCallback> m_change_listeners;
};

/**
 * @brief Text input field
 */
class TextField : public FormField {
public:
    explicit TextField(std::string id, std::string label);
    
    std::string get_value() const override;
    void set_value(const std::string& value) override;
    
    void set_max_length(std::optional<size_t> max_length);
    void set_min_length(std::optional<size_t> min_length);
    void set_pattern(std::optional<std::string> pattern);
    
protected:
    bool validate() override;
    
private:
    std::string m_value;
    std::optional<size_t> m_max_length;
    std::optional<size_t> m_min_length;
    std::optional<std::string> m_pattern;
};

/**
 * @brief Password input field
 */
class PasswordField : public TextField {
public:
    explicit PasswordField(std::string id, std::string label);
    
    void set_min_strength(int min_strength);
    
protected:
    bool validate() override;
    
private:
    int m_min_strength = 0;
    
    int calculate_password_strength(const std::string& password) const;
};

/**
 * @brief Login form component
 */
class LoginForm {
public:
    using SubmitCallback = std::function<void(const SubmitEvent&)>;
    
    explicit LoginForm(std::string id);
    ~LoginForm() = default;
    
    const std::string& get_id() const;
    
    bool is_enabled() const;
    void set_enabled(bool enabled);
    
    void add_field(std::shared_ptr<FormField> field);
    void remove_field(const std::string& field_id);
    std::shared_ptr<FormField> get_field(const std::string& field_id);
    
    void set_submit_label(const std::string& label);
    const std::string& get_submit_label() const;
    
    bool validate() const;
    void submit();
    
    void on_submit(SubmitCallback callback);
    
private:
    std::string m_id;
    bool m_enabled = true;
    std::vector<std::shared_ptr<FormField>> m_fields;
    std::string m_submit_label = "Submit";
    std::vector<SubmitCallback> m_submit_callbacks;
    
    void initialize_default_fields();
    void handle_field_change(const FieldChangeEvent& event);
    std::unordered_map<std::string, std::string> collect_field_values() const;
};

} // namespace app::ui
```

### Output (Implementation)

```cpp
// LoginForm.cpp

/**
 * MIT License
 * Copyright (c) 2025 Author
 */

#include "LoginForm.hpp"
#include <regex>
#include <algorithm>
#include <iostream>

namespace app::ui {

//------------------------------------------------------------------------------
// FormField Implementation
//------------------------------------------------------------------------------

FormField::FormField(std::string id, std::string label)
    : m_id(std::move(id)), m_label(std::move(label)) {}

const std::string& FormField::get_id() const {
    return m_id;
}

const std::string& FormField::get_label() const {
    return m_label;
}

bool FormField::is_required() const {
    return m_required;
}

void FormField::set_required(bool required) {
    m_required = required;
    validate();
}

bool FormField::is_valid() const {
    return m_valid;
}

const std::string& FormField::get_error() const {
    return m_error;
}

void FormField::add_change_listener(ChangeCallback callback) {
    m_change_listeners.push_back(std::move(callback));
}

void FormField::notify_change(const std::string& old_value, const std::string& new_value) {
    // Validate after change
    validate();
    
    // Notify listeners
    FieldChangeEvent event{m_id, old_value, new_value, m_valid};
    for (const auto& listener : m_change_listeners) {
        listener(event);
    }
}

void FormField::set_error(const std::string& error) {
    m_error = error;
    m_valid = error.empty();
}

//------------------------------------------------------------------------------
// TextField Implementation
//------------------------------------------------------------------------------

TextField::TextField(std::string id, std::string label)
    : FormField(std::move(id), std::move(label)) {}

std::string TextField::get_value() const {
    return m_value;
}

void TextField::set_value(const std::string& value) {
    std::string old_value = m_value;
    m_value = value;
    notify_change(old_value, m_value);
}

void TextField::set_max_length(std::optional<size_t> max_length) {
    m_max_length = max_length;
    validate();
}

void TextField::set_min_length(std::optional<size_t> min_length) {
    m_min_length = min_length;
    validate();
}

void TextField::set_pattern(std::optional<std::string> pattern) {
    m_pattern = std::move(pattern);
    validate();
}

bool TextField::validate() {
    // Check if required but empty
    if (is_required() && m_value.empty()) {
        set_error("This field is required");
        return false;
    }
    
    // Check min length
    if (m_min_length && !m_value.empty() && m_value.length() < *m_min_length) {
        set_error("Minimum length is " + std::to_string(*m_min_length));
        return false;
    }
    
    // Check max length
    if (m_max_length && m_value.length() > *m_max_length) {
        set_error("Maximum length is " + std::to_string(*m_max_length));
        return false;
    }
    
    // Check pattern
    if (m_pattern && !m_value.empty()) {
        try {
            std::regex pattern_regex(*m_pattern);
            if (!std::regex_match(m_value, pattern_regex)) {
                set_error("Invalid format");
                return false;
            }
        } catch (const std::regex_error&) {
            // Invalid regex pattern shouldn't fail validation
        }
    }
    
    set_error("");
    return true;
}

//------------------------------------------------------------------------------
// PasswordField Implementation
//------------------------------------------------------------------------------

PasswordField::PasswordField(std::string id, std::string label)
    : TextField(std::move(id), std::move(label)) {}

void PasswordField::set_min_strength(int min_strength) {
    m_min_strength = min_strength;
    validate();
}

bool PasswordField::validate() {
    // First use base class validation
    if (!TextField::validate()) {
        return false;
    }
    
    // Then check password strength
    const std::string& password = get_value();
    if (!password.empty() && calculate_password_strength(password) < m_min_strength) {
        set_error("Password is too weak");
        return false;
    }
    
    return true;
}

int PasswordField::calculate_password_strength(const std::string& password) const {
    int strength = 0;
    
    // Length contribution
    strength += std::min(static_cast<int>(password.length()) / 2, 5);
    
    // Character variety contribution
    bool has_lower = false;
    bool has_upper = false;
    bool has_digit = false;
    bool has_special = false;
    
    for (char c : password) {
        if (std::islower(c)) has_lower = true;
        else if (std::isupper(c)) has_upper = true;
        else if (std::isdigit(c)) has_digit = true;
        else has_special = true;
    }
    
    strength += has_lower ? 1 : 0;
    strength += has_upper ? 1 : 0;
    strength += has_digit ? 1 : 0;
    strength += has_special ? 2 : 0;
    
    return strength;
}

//------------------------------------------------------------------------------
// LoginForm Implementation
//------------------------------------------------------------------------------

LoginForm::LoginForm(std::string id)
    : m_id(std::move(id)) {
    initialize_default_fields();
}

const std::string& LoginForm::get_id() const {
    return m_id;
}

bool LoginForm::is_enabled() const {
    return m_enabled;
}

void LoginForm::set_enabled(bool enabled) {
    m_enabled = enabled;
}

void LoginForm::add_field(std::shared_ptr<FormField> field) {
    if (!field) return;
    
    // Check if field with same ID already exists
    auto it = std::find_if(m_fields.begin(), m_fields.end(),
                          [&field](const auto& existing) {
                              return existing->get_id() == field->get_id();
                          });
    
    if (it != m_fields.end()) {
        // Replace existing field
        *it = field;
    } else {
        // Add new field
        m_fields.push_back(field);
    }
    
    // Add change listener
    field->add_change_listener([this](const FieldChangeEvent& event) {
        handle_field_change(event);
    });
}

void LoginForm::remove_field(const std::string& field_id) {
    auto it = std::remove_if(m_fields.begin(), m_fields.end(),
                            [&field_id](const auto& field) {
                                return field->get_id() == field_id;
                            });
    
    if (it != m_fields.end()) {
        m_fields.erase(it, m_fields.end());
    }
}

std::shared_ptr<FormField> LoginForm::get_field(const std::string& field_id) {
    auto it = std::find_if(m_fields.begin(), m_fields.end(),
                          [&field_id](const auto& field) {
                              return field->get_id() == field_id;
                          });
    
    if (it != m_fields.end()) {
        return *it;
    }
    
    return nullptr;
}

void LoginForm::set_submit_label(const std::string& label) {
    m_submit_label = label;
}

const std::string& LoginForm::get_submit_label() const {
    return m_submit_label;
}

bool LoginForm::validate() const {
    bool valid = true;
    
    for (const auto& field : m_fields) {
        if (!field->is_valid()) {
            valid = false;
        }
    }
    
    return valid;
}

void LoginForm::submit() {
    if (!m_enabled) {
        return;
    }
    
    bool valid = validate();
    auto field_values = collect_field_values();
    
    SubmitEvent event{m_id, valid, field_values};
    
    for (const auto& callback : m_submit_callbacks) {
        callback(event);
    }
}

void LoginForm::on_submit(SubmitCallback callback) {
    m_submit_callbacks.push_back(std::move(callback));
}

void LoginForm::initialize_default_fields() {
    // Create username field
    auto username_field = std::make_shared<TextField>("username", "Username");
    username_field->set_required(true);
    username_field->set_min_length(3);
    add_field(username_field);
    
    // Create password field
    auto password_field = std::make_shared<PasswordField>("password", "Password");
    password_field->set_required(true);
    password_field->set_min_length(8);
    password_field->set_min_strength(5);
    add_field(password_field);
}

void LoginForm::handle_field_change(const FieldChangeEvent& event) {
    // The form can override field change handling here if needed
    std::cout << "Field changed: " << event.field_id 
              << " from '" << event.old_value 
              << "' to '" << event.new_value 
              << "' (Valid: " << (event.valid ? "true" : "false") << ")" 
              << std::endl;
}

std::unordered_map<std::string, std::string> LoginForm::collect_field_values() const {
    std::unordered_map<std::string, std::string> values;
    
    for (const auto& field : m_fields) {
        values[field->get_id()] = field->get_value();
    }
    
    return values;
}

} // namespace app::ui
```
