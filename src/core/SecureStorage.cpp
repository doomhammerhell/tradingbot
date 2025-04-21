#include "core/SecureStorage.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <random>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace tradingbot {
namespace core {

SecureStorage& SecureStorage::getInstance() {
    static SecureStorage instance;
    return instance;
}

void SecureStorage::initialize(const std::string& storagePath) {
    if (isInitialized_) {
        return;
    }
    
    storagePath_ = storagePath;
    std::filesystem::create_directories(storagePath_);
    
    // Generate initial encryption key
    config_.currentKey = generateKey();
    config_.lastRotation = std::chrono::system_clock::now();
    
    // Load existing storage if available
    if (std::filesystem::exists(storagePath_ + "/storage.json")) {
        loadStorage();
    }
    
    isInitialized_ = true;
    spdlog::info("Secure storage initialized at: {}", storagePath_);
}

void SecureStorage::storeData(const std::string& key, const std::string& value) {
    if (!isInitialized_) {
        throw std::runtime_error("Secure storage not initialized");
    }
    
    if (!isValidKey(key)) {
        throw std::invalid_argument("Invalid key format");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Encrypt the value
    std::string encrypted = encrypt(value);
    
    // Store the entry
    storage_[key] = {
        encrypted,
        std::chrono::system_clock::now(),
        config_.currentKey
    };
    
    // Save to file
    saveStorage();
    
    spdlog::debug("Stored data for key: {}", key);
}

std::string SecureStorage::retrieveData(const std::string& key) const {
    if (!isInitialized_) {
        throw std::runtime_error("Secure storage not initialized");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = storage_.find(key);
    if (it == storage_.end()) {
        throw std::runtime_error("Key not found: " + key);
    }
    
    // Decrypt the value
    return decrypt(it->second.value);
}

void SecureStorage::rotateKeys() {
    if (!isInitialized_) {
        throw std::runtime_error("Secure storage not initialized");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Generate new key
    std::string newKey = generateKey();
    
    // Add current key to history
    config_.previousKeys.push_back(config_.currentKey);
    
    // Trim key history if needed
    if (config_.previousKeys.size() > config_.maxKeyHistory) {
        config_.previousKeys.erase(config_.previousKeys.begin());
    }
    
    // Update current key
    config_.currentKey = newKey;
    config_.lastRotation = std::chrono::system_clock::now();
    
    // Save changes
    saveStorage();
    
    spdlog::info("Encryption keys rotated");
}

void SecureStorage::backup(const std::string& backupPath) {
    if (!isInitialized_) {
        throw std::runtime_error("Secure storage not initialized");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Create backup directory
    std::filesystem::create_directories(backupPath);
    
    // Save current state
    nlohmann::json backup = {
        {"config", {
            {"current_key", config_.currentKey},
            {"previous_keys", config_.previousKeys},
            {"last_rotation", config_.lastRotation.time_since_epoch().count()},
            {"rotation_interval_days", config_.rotationIntervalDays},
            {"max_key_history", config_.maxKeyHistory}
        }},
        {"storage", storage_}
    };
    
    // Save backup file
    std::ofstream file(backupPath + "/backup.json");
    file << backup.dump(4);
    
    spdlog::info("Secure storage backed up to: {}", backupPath);
}

void SecureStorage::restore(const std::string& backupPath) {
    if (!isInitialized_) {
        throw std::runtime_error("Secure storage not initialized");
    }
    
    if (!checkBackupIntegrity(backupPath)) {
        throw std::runtime_error("Backup integrity check failed");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Load backup file
    std::ifstream file(backupPath + "/backup.json");
    nlohmann::json backup;
    file >> backup;
    
    // Restore configuration
    config_.currentKey = backup["config"]["current_key"];
    config_.previousKeys = backup["config"]["previous_keys"].get<std::vector<std::string>>();
    config_.lastRotation = std::chrono::system_clock::time_point(
        std::chrono::system_clock::duration(backup["config"]["last_rotation"]));
    config_.rotationIntervalDays = backup["config"]["rotation_interval_days"];
    config_.maxKeyHistory = backup["config"]["max_key_history"];
    
    // Restore storage
    storage_ = backup["storage"].get<std::unordered_map<std::string, StorageEntry>>();
    
    // Save restored state
    saveStorage();
    
    spdlog::info("Secure storage restored from: {}", backupPath);
}

nlohmann::json SecureStorage::getStatus() const {
    return {
        {"initialized", isInitialized_},
        {"storage_path", storagePath_},
        {"entries_count", storage_.size()},
        {"last_rotation", std::chrono::system_clock::to_time_t(config_.lastRotation)},
        {"next_rotation", std::chrono::system_clock::to_time_t(
            config_.lastRotation + std::chrono::days(config_.rotationIntervalDays))},
        {"key_history_size", config_.previousKeys.size()}
    };
}

bool SecureStorage::validateIntegrity() const {
    if (!isInitialized_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if storage file exists
    if (!std::filesystem::exists(storagePath_ + "/storage.json")) {
        return false;
    }
    
    // Try to load and parse storage file
    try {
        std::ifstream file(storagePath_ + "/storage.json");
        nlohmann::json storage;
        file >> storage;
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Storage integrity check failed: {}", e.what());
        return false;
    }
}

std::string SecureStorage::encrypt(const std::string& data) const {
    // Simple XOR encryption (replace with stronger encryption if needed)
    std::string encrypted = data;
    for (size_t i = 0; i < data.length(); ++i) {
        encrypted[i] = data[i] ^ config_.currentKey[i % config_.currentKey.length()];
    }
    return encrypted;
}

std::string SecureStorage::decrypt(const std::string& encrypted) const {
    return encrypt(encrypted); // XOR is symmetric
}

std::string SecureStorage::generateKey() const {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.length() - 1);
    
    std::string key;
    key.reserve(32);
    for (int i = 0; i < 32; ++i) {
        key += chars[dis(gen)];
    }
    return key;
}

void SecureStorage::saveStorage() const {
    nlohmann::json storage = {
        {"config", {
            {"current_key", config_.currentKey},
            {"previous_keys", config_.previousKeys},
            {"last_rotation", config_.lastRotation.time_since_epoch().count()},
            {"rotation_interval_days", config_.rotationIntervalDays},
            {"max_key_history", config_.maxKeyHistory}
        }},
        {"storage", storage_}
    };
    
    std::ofstream file(storagePath_ + "/storage.json");
    file << storage.dump(4);
}

void SecureStorage::loadStorage() {
    std::ifstream file(storagePath_ + "/storage.json");
    nlohmann::json storage;
    file >> storage;
    
    config_.currentKey = storage["config"]["current_key"];
    config_.previousKeys = storage["config"]["previous_keys"].get<std::vector<std::string>>();
    config_.lastRotation = std::chrono::system_clock::time_point(
        std::chrono::system_clock::duration(storage["config"]["last_rotation"]));
    config_.rotationIntervalDays = storage["config"]["rotation_interval_days"];
    config_.maxKeyHistory = storage["config"]["max_key_history"];
    
    storage_ = storage["storage"].get<std::unordered_map<std::string, StorageEntry>>();
}

bool SecureStorage::isValidKey(const std::string& key) const {
    return !key.empty() && key.length() <= 256 && 
           std::all_of(key.begin(), key.end(), [](char c) {
               return std::isalnum(c) || c == '_' || c == '-';
           });
}

bool SecureStorage::checkBackupIntegrity(const std::string& backupPath) const {
    if (!std::filesystem::exists(backupPath + "/backup.json")) {
        return false;
    }
    
    try {
        std::ifstream file(backupPath + "/backup.json");
        nlohmann::json backup;
        file >> backup;
        
        return backup.contains("config") && backup.contains("storage");
    } catch (const std::exception& e) {
        spdlog::error("Backup integrity check failed: {}", e.what());
        return false;
    }
}

SecureStorage::~SecureStorage() {
    if (isInitialized_) {
        saveStorage();
    }
}

} // namespace core
} // namespace tradingbot 