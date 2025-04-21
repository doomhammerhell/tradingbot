#pragma once

#include "IStrategy.hpp"
#include <memory>
#include <string>
#include <map>
#include <functional>

namespace tradingbot {
namespace strategies {

class StrategyFactory {
public:
    using StrategyCreator = std::function<std::unique_ptr<IStrategy>()>;
    
    static StrategyFactory& getInstance() {
        static StrategyFactory instance;
        return instance;
    }
    
    // Register a new strategy type
    void registerStrategy(const std::string& name, StrategyCreator creator) {
        creators_[name] = creator;
    }
    
    // Create a strategy instance by name
    std::unique_ptr<IStrategy> createStrategy(const std::string& name) {
        auto it = creators_.find(name);
        if (it == creators_.end()) {
            throw std::runtime_error("Unknown strategy type: " + name);
        }
        return it->second();
    }
    
    // Get list of available strategies
    std::vector<std::string> getAvailableStrategies() const {
        std::vector<std::string> names;
        for (const auto& pair : creators_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
private:
    StrategyFactory() = default;
    ~StrategyFactory() = default;
    
    // Delete copy constructor and assignment operator
    StrategyFactory(const StrategyFactory&) = delete;
    StrategyFactory& operator=(const StrategyFactory&) = delete;
    
    std::map<std::string, StrategyCreator> creators_;
};

// Helper macro for registering strategies
#define REGISTER_STRATEGY(StrategyClass, name) \
    namespace { \
        struct StrategyClass##Registrar { \
            StrategyClass##Registrar() { \
                tradingbot::strategies::StrategyFactory::getInstance().registerStrategy( \
                    name, []() { return std::make_unique<StrategyClass>(); } \
                ); \
            } \
        }; \
        static StrategyClass##Registrar registrar; \
    } 