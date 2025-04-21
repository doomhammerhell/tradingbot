#pragma once

#include "../TechnicalIndicators.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

class SMA : public TechnicalIndicators {
public:
    SMA(int period = 20);
    ~SMA() override = default;

    void initialize(std::shared_ptr<core::IDataFeed> dataFeed) override;
    void calculate() override;
    std::vector<double> getValues() const override;
    std::string getName() const override;

    // Set SMA period
    void setPeriod(int period);

    // Get current SMA value
    double getCurrentValue() const;

private:
    int period_;
    std::vector<double> prices_;

    void updatePrices();
};

} // namespace indicators
} // namespace technical
} // namespace tradingbot 