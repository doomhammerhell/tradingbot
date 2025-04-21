#pragma once

#include "strategies/IStrategy.hpp"
#include "core/Logger.hpp"
#include <vector>
#include <deque>
#include <cmath>

namespace tradingbot {
namespace strategies {

class MomentumStrategy : public IStrategy {
public:
    MomentumStrategy(const nlohmann::json& config);
    ~MomentumStrategy() override = default;

    // IStrategy interface implementation
    void initialize() override;
    void update(const MarketData& data) override;
    Signal generateSignal() override;
    void reset() override;
    nlohmann::json getStatus() const override;

private:
    // Configuration parameters
    int shortPeriod_;
    int longPeriod_;
    double momentumThreshold_;
    double positionSize_;
    double stopLoss_;
    double takeProfit_;

    // State variables
    std::deque<double> priceHistory_;
    double currentMomentum_;
    double shortMA_;
    double longMA_;
    bool isPositionOpen_;
    double entryPrice_;

    // Helper methods
    void calculateMomentum();
    double calculateMA(int period) const;
    void updatePosition(const MarketData& data);
    bool shouldClosePosition(double currentPrice) const;
};

} // namespace strategies
} // namespace tradingbot 