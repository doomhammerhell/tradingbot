#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "core/IExecutionEngine.hpp"

namespace tradingbot {
namespace core {

struct Portfolio {
    double totalBalance;
    double availableBalance;
    std::map<std::string, double> positions;  // symbol -> quantity
    std::map<std::string, double> avgEntryPrices;  // symbol -> price
};

class IRiskManager {
public:
    virtual ~IRiskManager() = default;
    
    // Inicializa o gerenciador de risco com configurações específicas
    virtual void initialize(const std::string& config) = 0;
    
    // Verifica se uma ordem pode ser executada
    virtual bool shouldExecute(const Order& order, const Portfolio& portfolio) = 0;
    
    // Calcula o tamanho da posição com base no risco
    virtual double calculatePositionSize(const Order& order, const Portfolio& portfolio) = 0;
    
    // Atualiza o estado após execução de uma ordem
    virtual void updateAfterExecution(const Order& order, bool success) = 0;
    
    // Retorna métricas do gerenciador de risco
    virtual std::string getMetrics() const = 0;
};

} // namespace core
} // namespace tradingbot 