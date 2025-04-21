#pragma once

#include "../TechnicalIndicators.hpp"
#include "EMA.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

class MACD : public TechnicalIndicators {
public:
    MACD(int fastPeriod = 12, int slowPeriod = 26, int signalPeriod = 9);
    ~MACD() override = default;

    void initialize(std::shared_ptr<core::IDataFeed> dataFeed) override;
    void calculate() override;
    std::vector<double> getValues() const override;
    std::string getName() const override;

    // Get MACD line values
    std::vector<double> getMACDLine() const;

    // Get signal line values
    std::vector<double> getSignalLine() const;

    // Get histogram values
    std::vector<double> getHistogram() const;

    // Get current MACD value
    double getCurrentValue() const;

    // Get current signal value
    double getCurrentSignal() const;

    // Get current histogram value
    double getCurrentHistogram() const;

private:
    int fastPeriod_;
    int slowPeriod_;
    int signalPeriod_;
    std::vector<double> macdLine_;
    std::vector<double> signalLine_;
    std::vector<double> histogram_;
    std::unique_ptr<EMA> fastEMA_;
    std::unique_ptr<EMA> slowEMA_;
    std::unique_ptr<EMA> signalEMA_;

    void calculateMACDLine();
    void calculateSignalLine();
    void calculateHistogram();
};

} // namespace indicators
} // namespace technical
} // namespace tradingbot 