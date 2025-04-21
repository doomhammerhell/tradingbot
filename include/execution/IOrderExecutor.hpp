#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../core/Order.hpp"

namespace tradingbot {
namespace execution {

class IOrderExecutor {
public:
    virtual ~IOrderExecutor() = default;

    // Initialize the order executor
    virtual void initialize(const std::string& configPath) = 0;

    // Connect to exchange
    virtual void connect() = 0;

    // Disconnect from exchange
    virtual void disconnect() = 0;

    // Place a new order
    virtual core::Order placeOrder(const core::Order& order) = 0;

    // Cancel an existing order
    virtual bool cancelOrder(const std::string& orderId) = 0;

    // Get order status
    virtual core::Order getOrderStatus(const std::string& orderId) = 0;

    // Get all open orders
    virtual std::vector<core::Order> getOpenOrders() = 0;

    // Get account balance
    virtual double getBalance(const std::string& asset) = 0;

    // Get current position
    virtual double getPosition(const std::string& symbol) = 0;

    // Subscribe to order updates
    virtual void subscribeOrderUpdates(
        std::function<void(const core::Order&)> callback) = 0;
};

} // namespace execution
} // namespace tradingbot 