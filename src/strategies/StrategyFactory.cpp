#include "strategies/StrategyFactory.hpp"

namespace tradingbot {
namespace strategies {

std::map<std::string, StrategyFactory::StrategyCreator>& StrategyFactory::getRegistry() {
    static std::map<std::string, StrategyCreator> registry;
    return registry;
}

void StrategyFactory::registerStrategy(const std::string& name, StrategyCreator creator) {
    getRegistry()[name] = std::move(creator);
}

std::unique_ptr<core::IStrategy> StrategyFactory::createStrategy(const std::string& name) {
    auto it = getRegistry().find(name);
    if (it == getRegistry().end()) {
        throw std::runtime_error("Strategy not found: " + name);
    }
    return it->second();
}

std::vector<std::string> StrategyFactory::getAvailableStrategies() {
    std::vector<std::string> strategies;
    for (const auto& pair : getRegistry()) {
        strategies.push_back(pair.first);
    }
    return strategies;
}

} // namespace strategies
} // namespace tradingbot 