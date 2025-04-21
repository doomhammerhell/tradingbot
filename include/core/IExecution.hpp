#pragma once

#include <string>
#include <memory>

namespace tradingbot {
namespace core {

enum class OrderType {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT
};

enum class OrderSide {
    BUY,
    SELL
};

struct Order {
    std::string symbol;
    OrderType type;
    OrderSide side;
    double quantity;
    double price;
    double stopPrice;
    std::string orderId;
};

class IExecution {
public:
    virtual ~IExecution() = default;

    // Place a new order
    virtual bool placeOrder(const Order& order) = 0;

    // Cancel an existing order
    virtual bool cancelOrder(const std::string& orderId) = 0;

    // Get order status
    virtual Order getOrderStatus(const std::string& orderId) = 0;

    // Get account balance
    virtual double getBalance() const = 0;

    // Get position for a symbol
    virtual double getPosition(const std::string& symbol) const = 0;

    // Get execution name (e.g., "Mock", "Real")
    virtual std::string getName() const = 0;
};

} // namespace core
} // namespace tradingbot 