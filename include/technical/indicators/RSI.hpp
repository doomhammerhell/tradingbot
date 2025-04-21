#pragma once

#include "../TechnicalIndicators.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

class RSI : public TechnicalIndicators {
public:
    RSI(int period = 14);
    ~RSI() override = default;

    void initialize(std::shared_ptr<core::IDataFeed> dataFeed) override;
    void calculate() override;
    std::vector<double> getValues() const override;
    std::string getName() const override;

    // Set RSI period
    void setPeriod(int period);

    // Get current RSI value
    double getCurrentValue() const;

    // Get overbought threshold
    double getOverboughtThreshold() const { return 70.0; }

    // Get oversold threshold
    double getOversoldThreshold() const { return 30.0; }

private:
    int period_;
    std::vector<double> prices_;
    std::vector<double> gains_;
    std::vector<double> losses_;

    void updatePrices();
    void calculateGainsAndLosses();
    double calculateRSI(const std::vector<double>& gains, const std::vector<double>& losses) const;
};

} // namespace indicators
} // namespace technical
} // namespace tradingbot 