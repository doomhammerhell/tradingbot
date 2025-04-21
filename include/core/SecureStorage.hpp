#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>
#include "core/Logger.hpp"

namespace tradingbot {
namespace core {

class SecureStorage {
public:
    static SecureStorage& getInstance();

    // Initialize secure storage
    void initialize(const std::string& storagePath = "secure_storage");
    
    // Store sensitive data
    void storeData(const std::string& key, const std::string& value);
    
    // Retrieve sensitive data
    std::string retrieveData(const std::string& key) const;
    
    // Rotate encryption keys
    void rotateKeys();
    
    // Backup secure storage
    void backup(const std::string& backupPath);
    
    // Restore from backup
    void restore(const std::string& backupPath);
    
    // Get storage status
    nlohmann::json getStatus() const;
    
    // Validate storage integrity
    bool validateIntegrity() const;
    
    // Encrypt data
    std::string encrypt(const std::string& data) const;
    
    // Decrypt data
    std::string decrypt(const std::string& encrypted) const;

private:
    SecureStorage() = default;
    ~SecureStorage();
    
    // Prevent copying
    SecureStorage(const SecureStorage&) = delete;
    SecureStorage& operator=(const SecureStorage&) = delete;
    
    // Generate new encryption key
    std::string generateKey() const;
    
    // Save storage to file
    void saveStorage() const;
    
    // Load storage from file
    void loadStorage();
    
    // Validate key format
    bool isValidKey(const std::string& key) const;
    
    // Check backup integrity
    bool checkBackupIntegrity(const std::string& backupPath) const;

    struct StorageEntry {
        std::string value;
        std::chrono::system_clock::time_point timestamp;
        std::string keyVersion;
    };

    struct StorageConfig {
        std::string currentKey;
        std::vector<std::string> previousKeys;
        std::chrono::system_clock::time_point lastRotation;
        int rotationIntervalDays{30};
        int maxKeyHistory{5};
    };

    std::string storagePath_;
    StorageConfig config_;
    std::unordered_map<std::string, StorageEntry> storage_;
    mutable std::mutex mutex_;
    std::atomic<bool> isInitialized_{false};
};

} // namespace core
} // namespace tradingbot 