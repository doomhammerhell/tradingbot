#pragma once

#include <memory>
#include <string>
#include <vector>
#include "MarketData.hpp"
#include "Order.hpp"

namespace tradingbot {
namespace core {

class ITradingBot {
public:
    virtual ~ITradingBot() = default;

    // Initialize the bot with configuration
    virtual void initialize(const std::string& configPath) = 0;

    // Start the bot
    virtual void start() = 0;

    // Stop the bot
    virtual void stop() = 0;

    // Get current market data
    virtual MarketData getCurrentMarketData() const = 0;

    // Get current position
    virtual double getCurrentPosition() const = 0;

    // Get current balance
    virtual double getCurrentBalance() const = 0;

    // Get current orders
    virtual std::vector<Order> getCurrentOrders() const = 0;

    // Get bot status
    virtual std::string getStatus() const = 0;
};

} // namespace core
} // namespace tradingbot 