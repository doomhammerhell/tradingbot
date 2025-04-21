#pragma once

#include "IExchangeConnector.hpp"
#include <map>
#include <random>
#include <mutex>

namespace tradingbot {
namespace exchange {

class MockBinanceConnector : public IExchangeConnector {
public:
    MockBinanceConnector();
    ~MockBinanceConnector() = default;

    // IExchangeConnector implementation
    void initialize(const std::string& apiKey, const std::string& apiSecret) override;
    Order placeOrder(const Order& order) override;
    bool cancelOrder(const std::string& orderId) override;
    std::vector<Order> getOpenOrders() override;
    Order getOrderStatus(const std::string& orderId) override;
    std::vector<Balance> getBalances() override;
    Balance getBalance(const std::string& asset) override;
    double getCurrentPrice(const std::string& symbol) override;
    std::vector<std::string> getAvailableSymbols() override;
    std::string getExchangeName() const override;
    bool isConnected() const override;

private:
    std::string apiKey_;
    std::string apiSecret_;
    bool connected_;
    std::map<std::string, Balance> balances_;
    std::map<std::string, Order> openOrders_;
    std::map<std::string, double> currentPrices_;
    std::mutex mutex_;
    std::random_device rd_;
    std::mt19937 gen_;

    void initializeMockData();
    std::string generateOrderId();
    void updatePrices();
};

} // namespace exchange
} // namespace tradingbot 