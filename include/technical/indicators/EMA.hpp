#pragma once

#include "../TechnicalIndicators.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

class EMA : public TechnicalIndicators {
public:
    EMA(int period = 20);
    ~EMA() override = default;

    void initialize(std::shared_ptr<core::IDataFeed> dataFeed) override;
    void calculate() override;
    std::vector<double> getValues() const override;
    std::string getName() const override;

    // Set EMA period
    void setPeriod(int period);

    // Get current EMA value
    double getCurrentValue() const;

private:
    int period_;
    double multiplier_;
    std::vector<double> prices_;

    void calculateMultiplier();
    void updatePrices();
};

} // namespace indicators
} // namespace technical
} // namespace tradingbot 