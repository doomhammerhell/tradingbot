#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace tradingbot {
namespace exchange {

struct Order {
    enum class Type { MARKET, LIMIT };
    enum class Side { BUY, SELL };
    
    std::string symbol;
    Type type;
    Side side;
    double quantity;
    double price;  // For limit orders
    std::string orderId;
    std::chrono::system_clock::time_point timestamp;
};

struct Balance {
    std::string asset;
    double free;
    double locked;
    double total;
};

class IExchangeConnector {
public:
    virtual ~IExchangeConnector() = default;

    // Initialize with API credentials
    virtual void initialize(const std::string& apiKey, const std::string& apiSecret) = 0;

    // Order management
    virtual Order placeOrder(const Order& order) = 0;
    virtual bool cancelOrder(const std::string& orderId) = 0;
    virtual std::vector<Order> getOpenOrders() = 0;
    virtual Order getOrderStatus(const std::string& orderId) = 0;

    // Account information
    virtual std::vector<Balance> getBalances() = 0;
    virtual Balance getBalance(const std::string& asset) = 0;

    // Market data
    virtual double getCurrentPrice(const std::string& symbol) = 0;
    virtual std::vector<std::string> getAvailableSymbols() = 0;

    // Exchange information
    virtual std::string getExchangeName() const = 0;
    virtual bool isConnected() const = 0;
};

} // namespace exchange
} // namespace tradingbot 