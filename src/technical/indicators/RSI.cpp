#include "../../../include/technical/indicators/RSI.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

RSI::RSI(int period) : period_(period) {}

void RSI::initialize(std::shared_ptr<core::IDataFeed> dataFeed) {
    dataFeed_ = dataFeed;
    updatePrices();
    calculateGainsAndLosses();
}

void RSI::calculate() {
    if (prices_.size() < period_ + 1) {
        return;
    }

    for (size_t i = 0; i <= gains_.size() - period_; ++i) {
        std::vector<double> periodGains(gains_.begin() + i, gains_.begin() + i + period_);
        std::vector<double> periodLosses(losses_.begin() + i, losses_.begin() + i + period_);
        values_.push_back(calculateRSI(periodGains, periodLosses));
    }
}

std::vector<double> RSI::getValues() const {
    return values_;
}

std::string RSI::getName() const {
    return "RSI_" + std::to_string(period_);
}

void RSI::setPeriod(int period) {
    period_ = period;
    values_.clear();
    calculate();
}

double RSI::getCurrentValue() const {
    return values_.empty() ? 50.0 : values_.back();
}

void RSI::updatePrices() {
    if (!dataFeed_) {
        return;
    }

    auto marketData = dataFeed_->getHistoricalData(period_ * 2);
    prices_ = marketData.prices;
}

void RSI::calculateGainsAndLosses() {
    gains_.clear();
    losses_.clear();

    for (size_t i = 1; i < prices_.size(); ++i) {
        double change = prices_[i] - prices_[i - 1];
        gains_.push_back(change > 0 ? change : 0.0);
        losses_.push_back(change < 0 ? -change : 0.0);
    }
}

double RSI::calculateRSI(const std::vector<double>& gains, const std::vector<double>& losses) const {
    double avgGain = 0.0;
    double avgLoss = 0.0;

    for (size_t i = 0; i < gains.size(); ++i) {
        avgGain += gains[i];
        avgLoss += losses[i];
    }

    avgGain /= period_;
    avgLoss /= period_;

    if (avgLoss == 0.0) {
        return 100.0;
    }

    double rs = avgGain / avgLoss;
    return 100.0 - (100.0 / (1.0 + rs));
}

} // namespace indicators
} // namespace technical
} // namespace tradingbot 