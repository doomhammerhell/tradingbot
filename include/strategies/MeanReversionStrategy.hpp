#pragma once

#include "strategies/IStrategy.hpp"
#include "core/Logger.hpp"
#include <vector>
#include <deque>
#include <cmath>

namespace tradingbot {
namespace strategies {

class MeanReversionStrategy : public IStrategy {
public:
    MeanReversionStrategy(const nlohmann::json& config);
    ~MeanReversionStrategy() override = default;

    // IStrategy interface implementation
    void initialize() override;
    void update(const MarketData& data) override;
    Signal generateSignal() override;
    void reset() override;
    nlohmann::json getStatus() const override;

private:
    // Configuration parameters
    int lookbackPeriod_;
    double zScoreThreshold_;
    double positionSize_;
    double stopLoss_;
    double takeProfit_;

    // State variables
    std::deque<double> priceHistory_;
    double currentZScore_;
    double currentMean_;
    double currentStdDev_;
    bool isPositionOpen_;
    double entryPrice_;

    // Helper methods
    void calculateStatistics();
    double calculateZScore(double price) const;
    void updatePosition(const MarketData& data);
    bool shouldClosePosition(double currentPrice) const;
};

} // namespace strategies
} // namespace tradingbot 