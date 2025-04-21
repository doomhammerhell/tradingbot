#include "core/SecureConfigLoader.hpp"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <fstream>
#include <algorithm>

namespace tradingbot {
namespace core {

SecureConfigLoader& SecureConfigLoader::getInstance() {
    static SecureConfigLoader instance;
    return instance;
}

void SecureConfigLoader::loadConfig(const std::string& configPath) {
    // First try to load from environment variables
    loadFromEnv();
    
    // If config file path is provided, try to load from file
    if (!configPath.empty()) {
        loadFromFile(configPath);
    }
    
    // Generate encryption key
    encryptionKey_ = generateEncryptionKey();
    
    // Check if running in simulation mode
    isSimulationMode_ = getValue("SIMULATION_MODE", "false") == "true";
    
    spdlog::info("Configuration loaded successfully. Simulation mode: {}", isSimulationMode_);
}

void SecureConfigLoader::loadFromEnv() {
    const char* envVars[] = {
        "API_KEY",
        "API_SECRET",
        "EXCHANGE_URL",
        "SIMULATION_MODE",
        "LOG_LEVEL",
        "DB_CONNECTION"
    };
    
    for (const char* var : envVars) {
        const char* value = std::getenv(var);
        if (value) {
            config_[var] = value;
            spdlog::debug("Loaded environment variable: {}", var);
        }
    }
}

void SecureConfigLoader::loadFromFile(const std::string& configPath) {
    if (!std::filesystem::exists(configPath)) {
        spdlog::warn("Config file not found: {}", configPath);
        return;
    }
    
    try {
        std::ifstream file(configPath);
        nlohmann::json config;
        file >> config;
        
        for (auto& [key, value] : config.items()) {
            if (value.is_string()) {
                config_[key] = value.get<std::string>();
            }
        }
        
        spdlog::info("Loaded configuration from file: {}", configPath);
    } catch (const std::exception& e) {
        spdlog::error("Failed to load config file: {}", e.what());
    }
}

std::string SecureConfigLoader::getValue(const std::string& key, const std::string& defaultValue) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    return defaultValue;
}

bool SecureConfigLoader::isSimulationMode() const {
    return isSimulationMode_;
}

std::string SecureConfigLoader::generateEncryptionKey() const {
    // Generate a key from environment variables and system info
    std::string key;
    
    // Use API_KEY if available
    const char* apiKey = std::getenv("API_KEY");
    if (apiKey) {
        key += apiKey;
    }
    
    // Add some system-specific information
    key += std::to_string(std::hash<std::string>{}(std::getenv("USER")));
    
    // Ensure key has minimum length
    while (key.length() < 32) {
        key += key;
    }
    key = key.substr(0, 32);
    
    return key;
}

std::string SecureConfigLoader::encryptValue(const std::string& input, const std::string& key) const {
    std::string output = input;
    for (size_t i = 0; i < input.length(); ++i) {
        output[i] = input[i] ^ key[i % key.length()];
    }
    return output;
}

std::string SecureConfigLoader::decryptValue(const std::string& encrypted, const std::string& key) const {
    return encryptValue(encrypted, key); // XOR is symmetric
}

} // namespace core
} // namespace tradingbot 