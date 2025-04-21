#pragma once

#include "IExchangeConnector.hpp"
#include <memory>
#include <string>
#include <map>
#include <functional>
#include <nlohmann/json.hpp>

namespace tradingbot {
namespace exchange {

class ExchangeConnectorFactory {
public:
    static ExchangeConnectorFactory& getInstance() {
        static ExchangeConnectorFactory instance;
        return instance;
    }

    // Register a new exchange connector type
    template<typename T>
    void registerConnector(const std::string& type) {
        creators_[type] = [](const std::string& config) -> std::unique_ptr<IExchangeConnector> {
            auto connector = std::make_unique<T>();
            auto json = nlohmann::json::parse(config);
            connector->initialize(
                json["apiKey"].get<std::string>(),
                json["apiSecret"].get<std::string>()
            );
            return connector;
        };
    }

    // Create an exchange connector instance
    std::unique_ptr<IExchangeConnector> createConnector(const std::string& type, const std::string& config) {
        auto it = creators_.find(type);
        if (it == creators_.end()) {
            throw std::runtime_error("Unknown exchange connector type: " + type);
        }
        return it->second(config);
    }

    // Get available connector types
    std::vector<std::string> getAvailableTypes() const {
        std::vector<std::string> types;
        for (const auto& pair : creators_) {
            types.push_back(pair.first);
        }
        return types;
    }

    // Create connector from configuration JSON
    std::unique_ptr<IExchangeConnector> createFromConfig(const std::string& configJson) {
        try {
            auto json = nlohmann::json::parse(configJson);
            std::string type = json["type"].get<std::string>();
            std::string connectorConfig = json["config"].dump();
            return createConnector(type, connectorConfig);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to create connector from config: " + std::string(e.what()));
        }
    }

private:
    ExchangeConnectorFactory() = default;
    ~ExchangeConnectorFactory() = default;
    ExchangeConnectorFactory(const ExchangeConnectorFactory&) = delete;
    ExchangeConnectorFactory& operator=(const ExchangeConnectorFactory&) = delete;

    std::map<std::string, std::function<std::unique_ptr<IExchangeConnector>(const std::string&)>> creators_;
};

// Helper macro for registering exchange connectors
#define REGISTER_EXCHANGE_CONNECTOR(type, class) \
    namespace { \
        struct Register##class { \
            Register##class() { \
                ExchangeConnectorFactory::getInstance().registerConnector<class>(type); \
            } \
        }; \
        static Register##class register##class; \
    }

} // namespace exchange
} // namespace tradingbot 