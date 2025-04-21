#pragma once

#include "ITradingBot.hpp"
#include "../data/IDataProvider.hpp"
#include "../execution/IOrderExecutor.hpp"
#include "../risk/IRiskManager.hpp"
#include "../strategies/IStrategy.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace tradingbot {
namespace core {

class TradingBot : public ITradingBot {
public:
    TradingBot();
    ~TradingBot() override;

    void initialize(const std::string& configPath) override;
    void start() override;
    void stop() override;
    MarketData getCurrentMarketData() const override;
    double getCurrentPosition() const override;
    double getCurrentBalance() const override;
    std::vector<Order> getCurrentOrders() const override;
    std::string getStatus() const override;

private:
    std::shared_ptr<data::IDataProvider> dataProvider_;
    std::shared_ptr<execution::IOrderExecutor> orderExecutor_;
    std::shared_ptr<risk::IRiskManager> riskManager_;
    std::shared_ptr<strategies::IStrategy> strategy_;
    std::atomic<bool> isRunning_;
    std::thread tradingThread_;
    std::mutex dataMutex_;
    MarketData currentMarketData_;
    double currentPosition_;
    double currentBalance_;
    std::vector<Order> currentOrders_;

    void tradingLoop();
    void processMarketData(const MarketData& data);
    void processOrderUpdate(const Order& order);
    void executeStrategy();
};

} // namespace core
} // namespace tradingbot 