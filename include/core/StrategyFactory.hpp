#pragma once

#include <memory>
#include <string>
#include <map>
#include <functional>
#include "core/IStrategy.hpp"

namespace tradingbot {
namespace core {

class StrategyFactory {
public:
    using StrategyCreator = std::function<std::unique_ptr<IStrategy>()>;
    
    // Registra uma nova estratégia
    static void registerStrategy(const std::string& name, StrategyCreator creator);
    
    // Cria uma estratégia com base no nome
    static std::unique_ptr<IStrategy> createStrategy(const std::string& name);
    
    // Lista todas as estratégias registradas
    static std::vector<std::string> listStrategies();
    
private:
    static std::map<std::string, StrategyCreator>& getRegistry();
};

// Macro para facilitar o registro de estratégias
#define REGISTER_STRATEGY(StrategyClass, name) \
    namespace { \
        struct StrategyClass##Registrar { \
            StrategyClass##Registrar() { \
                StrategyFactory::registerStrategy(name, []() { \
                    return std::make_unique<StrategyClass>(); \
                }); \
            } \
        }; \
        static StrategyClass##Registrar strategyClass##Registrar; \
    } 