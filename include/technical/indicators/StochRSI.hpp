#pragma once

#include "../TechnicalIndicators.hpp"
#include "RSI.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

class StochRSI : public TechnicalIndicators {
public:
    StochRSI(int period = 14, int kPeriod = 3, int dPeriod = 3);
    ~StochRSI() override = default;

    void initialize(std::shared_ptr<core::IDataFeed> dataFeed) override;
    void calculate() override;
    std::vector<double> getValues() const override;
    std::string getName() const override;

    // Get %K values
    std::vector<double> getKValues() const;

    // Get %D values
    std::vector<double> getDValues() const;

    // Get current %K value
    double getCurrentK() const;

    // Get current %D value
    double getCurrentD() const;

    // Get overbought threshold
    double getOverboughtThreshold() const { return 80.0; }

    // Get oversold threshold
    double getOversoldThreshold() const { return 20.0; }

private:
    int period_;
    int kPeriod_;
    int dPeriod_;
    std::vector<double> kValues_;
    std::vector<double> dValues_;
    std::unique_ptr<RSI> rsi_;

    void calculateKValues();
    void calculateDValues();
};

} // namespace indicators
} // namespace technical
} // namespace tradingbot 