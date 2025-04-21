#include "core/StrategyFactory.hpp"
#include "utils/Logger.hpp"

namespace tradingbot {
namespace core {

std::map<std::string, StrategyFactory::StrategyCreator>& StrategyFactory::getRegistry() {
    static std::map<std::string, StrategyCreator> registry;
    return registry;
}

void StrategyFactory::registerStrategy(const std::string& name, StrategyCreator creator) {
    auto& logger = utils::Logger::getInstance();
    auto& registry = getRegistry();
    
    if (registry.find(name) != registry.end()) {
        logger.warn("Strategy '{}' already registered, overwriting", name);
    }
    
    registry[name] = creator;
    logger.info("Registered strategy '{}'", name);
}

std::unique_ptr<IStrategy> StrategyFactory::createStrategy(const std::string& name) {
    auto& logger = utils::Logger::getInstance();
    auto& registry = getRegistry();
    
    auto it = registry.find(name);
    if (it == registry.end()) {
        logger.error("Strategy '{}' not found", name);
        throw std::runtime_error("Strategy not found: " + name);
    }
    
    logger.info("Creating strategy '{}'", name);
    return it->second();
}

std::vector<std::string> StrategyFactory::listStrategies() {
    auto& registry = getRegistry();
    std::vector<std::string> strategies;
    
    for (const auto& pair : registry) {
        strategies.push_back(pair.first);
    }
    
    return strategies;
}

} // namespace core
} // namespace tradingbot 