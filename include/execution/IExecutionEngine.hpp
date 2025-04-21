#pragma once

#include <string>
#include <memory>
#include <chrono>

namespace tradingbot {
namespace execution {

enum class OrderType {
    MARKET,
    LIMIT,
    STOP_LOSS,
    TAKE_PROFIT
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED,
    EXPIRED
};

struct Order {
    std::string id;
    std::string symbol;
    OrderType type;
    OrderSide side;
    double quantity;
    double price;
    double stopPrice;
    double takeProfitPrice;
    std::chrono::system_clock::time_point timestamp;
    OrderStatus status;
    double executedQuantity;
    double averagePrice;
};

class IExecutionEngine {
public:
    virtual ~IExecutionEngine() = default;
    
    // Initialize execution engine with configuration
    virtual void initialize(const std::string& config) = 0;
    
    // Execute a new order
    virtual std::string execute(const Order& order) = 0;
    
    // Cancel an existing order
    virtual bool cancelOrder(const std::string& orderId) = 0;
    
    // Get order status
    virtual OrderStatus getOrderStatus(const std::string& orderId) = 0;
    
    // Get execution metrics and statistics
    virtual std::string getMetrics() const = 0;
};

} // namespace execution
} // namespace tradingbot 