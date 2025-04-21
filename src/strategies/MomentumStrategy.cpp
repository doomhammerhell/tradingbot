#include "strategies/MomentumStrategy.hpp"
#include <spdlog/spdlog.h>

namespace tradingbot {
namespace strategies {

MomentumStrategy::MomentumStrategy(const nlohmann::json& config) {
    shortPeriod_ = config.value("short_period", 10);
    longPeriod_ = config.value("long_period", 30);
    momentumThreshold_ = config.value("momentum_threshold", 0.02);
    positionSize_ = config.value("position_size", 1.0);
    stopLoss_ = config.value("stop_loss", 0.02);
    takeProfit_ = config.value("take_profit", 0.03);
    
    reset();
}

void MomentumStrategy::initialize() {
    spdlog::info("Initializing MomentumStrategy with short period: {}, long period: {}, threshold: {}",
                 shortPeriod_, longPeriod_, momentumThreshold_);
}

void MomentumStrategy::update(const MarketData& data) {
    priceHistory_.push_back(data.price);
    if (priceHistory_.size() > longPeriod_) {
        priceHistory_.pop_front();
    }
    
    calculateMomentum();
    updatePosition(data);
}

Signal MomentumStrategy::generateSignal() {
    if (priceHistory_.size() < longPeriod_) {
        return Signal{SignalType::HOLD, 0.0};
    }
    
    if (isPositionOpen_) {
        return Signal{SignalType::HOLD, positionSize_};
    }
    
    if (currentMomentum_ > momentumThreshold_) {
        return Signal{SignalType::BUY, positionSize_};
    } else if (currentMomentum_ < -momentumThreshold_) {
        return Signal{SignalType::SELL, positionSize_};
    }
    
    return Signal{SignalType::HOLD, 0.0};
}

void MomentumStrategy::reset() {
    priceHistory_.clear();
    currentMomentum_ = 0.0;
    shortMA_ = 0.0;
    longMA_ = 0.0;
    isPositionOpen_ = false;
    entryPrice_ = 0.0;
}

nlohmann::json MomentumStrategy::getStatus() const {
    return {
        {"strategy", "momentum"},
        {"short_period", shortPeriod_},
        {"long_period", longPeriod_},
        {"momentum_threshold", momentumThreshold_},
        {"position_size", positionSize_},
        {"current_momentum", currentMomentum_},
        {"short_ma", shortMA_},
        {"long_ma", longMA_},
        {"is_position_open", isPositionOpen_},
        {"entry_price", entryPrice_}
    };
}

void MomentumStrategy::calculateMomentum() {
    if (priceHistory_.size() < longPeriod_) {
        return;
    }
    
    shortMA_ = calculateMA(shortPeriod_);
    longMA_ = calculateMA(longPeriod_);
    
    if (longMA_ != 0) {
        currentMomentum_ = (shortMA_ - longMA_) / longMA_;
    }
}

double MomentumStrategy::calculateMA(int period) const {
    if (priceHistory_.size() < period) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (size_t i = priceHistory_.size() - period; i < priceHistory_.size(); ++i) {
        sum += priceHistory_[i];
    }
    return sum / period;
}

void MomentumStrategy::updatePosition(const MarketData& data) {
    if (!isPositionOpen_) {
        return;
    }
    
    if (shouldClosePosition(data.price)) {
        isPositionOpen_ = false;
        entryPrice_ = 0.0;
    }
}

bool MomentumStrategy::shouldClosePosition(double currentPrice) const {
    if (!isPositionOpen_) {
        return false;
    }
    
    double priceChange = std::abs(currentPrice - entryPrice_) / entryPrice_;
    return priceChange >= stopLoss_ || priceChange >= takeProfit_;
}

} // namespace strategies
} // namespace tradingbot 