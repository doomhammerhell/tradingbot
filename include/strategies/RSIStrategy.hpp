#pragma once

#include "IStrategy.hpp"
#include <vector>
#include <cmath>

namespace tradingbot {

class RSIStrategy : public IStrategy {
public:
    struct Config {
        int period = 14;           // RSI period
        double overbought = 70.0;  // Overbought threshold
        double oversold = 30.0;    // Oversold threshold
        double take_profit = 0.5;  // Take profit percentage
        double stop_loss = 0.1;    // Stop loss percentage
    };

    RSIStrategy(const Config& config);
    virtual ~RSIStrategy() = default;

    virtual Signal generateSignal(const MarketData& data) override;
    virtual void updateState(const MarketData& data) override;
    virtual void reset() override;

private:
    Config config_;
    std::vector<double> price_changes_;
    double current_rsi_;
    double last_price_;

    double calculateRSI() const;
};

} // namespace tradingbot 