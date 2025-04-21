#include "../../../include/technical/indicators/EMA.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

EMA::EMA(int period) : period_(period) {
    calculateMultiplier();
}

void EMA::initialize(std::shared_ptr<core::IDataFeed> dataFeed) {
    dataFeed_ = dataFeed;
    updatePrices();
}

void EMA::calculate() {
    if (prices_.size() < period_) {
        return;
    }

    // Calculate initial SMA
    double sum = 0.0;
    for (int i = 0; i < period_; ++i) {
        sum += prices_[i];
    }
    double sma = sum / period_;
    values_.push_back(sma);

    // Calculate EMA
    for (size_t i = period_; i < prices_.size(); ++i) {
        double ema = (prices_[i] - values_.back()) * multiplier_ + values_.back();
        values_.push_back(ema);
    }
}

std::vector<double> EMA::getValues() const {
    return values_;
}

std::string EMA::getName() const {
    return "EMA_" + std::to_string(period_);
}

void EMA::setPeriod(int period) {
    period_ = period;
    calculateMultiplier();
    values_.clear();
    calculate();
}

double EMA::getCurrentValue() const {
    return values_.empty() ? 0.0 : values_.back();
}

void EMA::calculateMultiplier() {
    multiplier_ = 2.0 / (period_ + 1.0);
}

void EMA::updatePrices() {
    if (!dataFeed_) {
        return;
    }

    auto marketData = dataFeed_->getHistoricalData(period_ * 2);
    prices_ = marketData.prices;
}

} // namespace indicators
} // namespace technical
} // namespace tradingbot 