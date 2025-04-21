#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace tradingbot {
namespace core {

struct Order {
    enum class Type {
        MARKET,
        LIMIT,
        STOP_LOSS
    };
    
    enum class Side {
        BUY,
        SELL
    };
    
    std::string id;
    std::string symbol;
    Type type;
    Side side;
    double price;
    double quantity;
    double stopPrice;  // Para ordens STOP_LOSS
};

class IExecutionEngine {
public:
    virtual ~IExecutionEngine() = default;
    
    // Inicializa o engine com configurações específicas
    virtual void initialize(const std::string& config) = 0;
    
    // Executa uma ordem
    virtual bool execute(const Order& order) = 0;
    
    // Cancela uma ordem
    virtual bool cancelOrder(const std::string& orderId) = 0;
    
    // Obtém o status de uma ordem
    virtual std::string getOrderStatus(const std::string& orderId) = 0;
    
    // Retorna métricas do engine
    virtual std::string getMetrics() const = 0;
};

} // namespace core
} // namespace tradingbot 