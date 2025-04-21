#include "../../../include/technical/indicators/SMA.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

SMA::SMA(int period) : period_(period) {}

void SMA::initialize(std::shared_ptr<core::IDataFeed> dataFeed) {
    dataFeed_ = dataFeed;
    updatePrices();
}

void SMA::calculate() {
    if (prices_.size() < period_) {
        return;
    }

    for (size_t i = 0; i <= prices_.size() - period_; ++i) {
        double sum = 0.0;
        for (size_t j = i; j < i + period_; ++j) {
            sum += prices_[j];
        }
        values_.push_back(sum / period_);
    }
}

std::vector<double> SMA::getValues() const {
    return values_;
}

std::string SMA::getName() const {
    return "SMA_" + std::to_string(period_);
}

void SMA::setPeriod(int period) {
    period_ = period;
    values_.clear();
    calculate();
}

double SMA::getCurrentValue() const {
    return values_.empty() ? 0.0 : values_.back();
}

void SMA::updatePrices() {
    if (!dataFeed_) {
        return;
    }

    auto marketData = dataFeed_->getHistoricalData(period_ * 2);
    prices_ = marketData.prices;
}

} // namespace indicators
} // namespace technical
} // namespace tradingbot 