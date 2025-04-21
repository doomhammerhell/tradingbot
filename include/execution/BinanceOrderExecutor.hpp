#pragma once

#include "IOrderExecutor.hpp"
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

namespace tradingbot {
namespace execution {

class BinanceOrderExecutor : public IOrderExecutor {
public:
    BinanceOrderExecutor();
    ~BinanceOrderExecutor() override;

    void initialize(const std::string& configPath) override;
    void connect() override;
    void disconnect() override;
    core::Order placeOrder(const core::Order& order) override;
    bool cancelOrder(const std::string& orderId) override;
    core::Order getOrderStatus(const std::string& orderId) override;
    std::vector<core::Order> getOpenOrders() override;
    double getBalance(const std::string& asset) override;
    double getPosition(const std::string& symbol) override;
    void subscribeOrderUpdates(
        std::function<void(const core::Order&)> callback) override;

private:
    std::string apiKey_;
    std::string apiSecret_;
    std::string baseUrl_;
    std::atomic<bool> isConnected_;
    std::thread websocketThread_;
    std::function<void(const core::Order&)> orderCallback_;
    std::mutex callbackMutex_;
    std::map<std::string, double> balances_;
    std::map<std::string, double> positions_;
    std::mutex dataMutex_;

    void startWebsocket();
    void stopWebsocket();
    void processWebsocketMessage(const std::string& message);
    std::string generateSignature(const std::string& queryString) const;
    void updateBalances();
    void updatePositions();
};

} // namespace execution
} // namespace tradingbot 