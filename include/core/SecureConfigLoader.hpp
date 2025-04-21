#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <mutex>
#include <chrono>

namespace tradingbot {
namespace core {

class SecureConfigLoader {
public:
    static SecureConfigLoader& getInstance();

    // Load configuration from environment variables or encrypted file
    void loadConfig(const std::string& configPath = "");
    
    // Get configuration value with fallback
    std::string getValue(const std::string& key, const std::string& defaultValue = "") const;
    
    // Check if running in simulation mode
    bool isSimulationMode() const;
    
    // Encrypt/decrypt sensitive data
    std::string encryptValue(const std::string& value) const;
    std::string decryptValue(const std::string& encrypted) const;
    
    // Validate configuration
    bool validateConfig() const;
    
    // Rotate encryption key
    void rotateKey();
    
    // Check if configuration is valid
    bool isValid() const;
    
    // Get configuration age
    std::chrono::seconds getConfigAge() const;

private:
    SecureConfigLoader() = default;
    ~SecureConfigLoader() = default;
    
    // Prevent copying
    SecureConfigLoader(const SecureConfigLoader&) = delete;
    SecureConfigLoader& operator=(const SecureConfigLoader&) = delete;

    // Load from environment variables
    void loadFromEnv();
    
    // Load from encrypted config file
    void loadFromFile(const std::string& configPath);
    
    // Generate encryption key from environment
    std::string generateEncryptionKey() const;
    
    // Simple XOR encryption (replace with stronger encryption if needed)
    std::string xorEncrypt(const std::string& input, const std::string& key) const;
    std::string xorDecrypt(const std::string& input, const std::string& key) const;
    
    // Validate required fields
    bool validateRequiredFields() const;
    
    // Check file permissions
    bool checkFilePermissions(const std::string& path) const;
    
    // Sanitize configuration values
    void sanitizeConfig();
    
    // Mask sensitive values in logs
    std::string maskSensitiveValue(const std::string& value) const;

    std::unordered_map<std::string, std::string> config_;
    bool isSimulationMode_ = false;
    std::string encryptionKey_;
    mutable std::mutex mutex_;
    std::chrono::system_clock::time_point lastLoadTime_;
    bool isValid_{false};
};

} // namespace core
} // namespace tradingbot 