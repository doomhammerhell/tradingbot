#pragma once

#include "IStrategy.hpp"
#include "../technical/indicators/MACD.hpp"

namespace tradingbot {
namespace strategies {

class MACDStrategy : public IStrategy {
public:
    MACDStrategy();
    ~MACDStrategy() override = default;

    void initialize(const StrategyParameters& params) override;
    void onMarketData(const core::MarketData& data) override;
    void onOrderUpdate(const core::Order& order) override;
    std::vector<core::Order> generateSignals() override;
    std::string getName() const override;
    StrategyParameters getParameters() const override;
    std::map<std::string, double> getPerformanceMetrics() const override;
    void reset() override;

private:
    StrategyParameters params_;
    std::shared_ptr<technical::indicators::MACD> macd_;
    double lastPrice_;
    double currentPosition_;
    double totalPnL_;
    int winningTrades_;
    int losingTrades_;

    void updatePerformanceMetrics(const core::Order& order);
};

} // namespace strategies
} // namespace tradingbot 