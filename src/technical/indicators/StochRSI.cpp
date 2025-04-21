#include "../../../include/technical/indicators/StochRSI.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

StochRSI::StochRSI(int period, int kPeriod, int dPeriod)
    : period_(period)
    , kPeriod_(kPeriod)
    , dPeriod_(dPeriod)
    , rsi_(std::make_unique<RSI>(period)) {}

void StochRSI::initialize(std::shared_ptr<core::IDataFeed> dataFeed) {
    dataFeed_ = dataFeed;
    rsi_->initialize(dataFeed);
}

void StochRSI::calculate() {
    if (!dataFeed_) {
        return;
    }

    // Calculate RSI
    rsi_->calculate();
    const auto& rsiValues = rsi_->getValues();

    // Calculate %K values
    calculateKValues();

    // Calculate %D values
    calculateDValues();
}

std::vector<double> StochRSI::getValues() const {
    return kValues_;
}

std::string StochRSI::getName() const {
    return "StochRSI_" + std::to_string(period_) + "_" + 
           std::to_string(kPeriod_) + "_" + 
           std::to_string(dPeriod_);
}

std::vector<double> StochRSI::getKValues() const {
    return kValues_;
}

std::vector<double> StochRSI::getDValues() const {
    return dValues_;
}

double StochRSI::getCurrentK() const {
    return kValues_.empty() ? 50.0 : kValues_.back();
}

double StochRSI::getCurrentD() const {
    return dValues_.empty() ? 50.0 : dValues_.back();
}

void StochRSI::calculateKValues() {
    const auto& rsiValues = rsi_->getValues();
    kValues_.clear();

    for (size_t i = 0; i <= rsiValues.size() - kPeriod_; ++i) {
        double minRSI = rsiValues[i];
        double maxRSI = rsiValues[i];

        for (size_t j = i; j < i + kPeriod_; ++j) {
            minRSI = std::min(minRSI, rsiValues[j]);
            maxRSI = std::max(maxRSI, rsiValues[j]);
        }

        double currentRSI = rsiValues[i + kPeriod_ - 1];
        double kValue = 100.0 * (currentRSI - minRSI) / (maxRSI - minRSI);
        kValues_.push_back(kValue);
    }
}

void StochRSI::calculateDValues() {
    dValues_.clear();

    for (size_t i = 0; i <= kValues_.size() - dPeriod_; ++i) {
        double sum = 0.0;
        for (size_t j = i; j < i + dPeriod_; ++j) {
            sum += kValues_[j];
        }
        dValues_.push_back(sum / dPeriod_);
    }
}

} // namespace indicators
} // namespace technical
} // namespace tradingbot 