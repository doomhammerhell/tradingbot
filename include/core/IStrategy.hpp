#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace tradingbot {
namespace core {

struct MarketData {
    double price;
    double volume;
    double timestamp;
    // Adicione outros campos conforme necessário
};

struct Decision {
    enum class Type {
        BUY,
        SELL,
        HOLD
    };
    
    Type type;
    double price;
    double quantity;
    std::string symbol;
};

class IStrategy {
public:
    virtual ~IStrategy() = default;
    
    // Inicializa a estratégia com configurações específicas
    virtual void initialize(const std::string& config) = 0;
    
    // Analisa os dados de mercado e retorna uma decisão
    virtual Decision analyze(const std::vector<MarketData>& marketData) = 0;
    
    // Atualiza o status de uma ordem executada
    virtual void updateOrderStatus(const std::string& orderId, bool success) = 0;
    
    // Retorna métricas da estratégia
    virtual std::string getMetrics() const = 0;
};

} // namespace core
} // namespace tradingbot 