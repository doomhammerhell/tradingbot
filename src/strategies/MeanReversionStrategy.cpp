#include "strategies/MeanReversionStrategy.hpp"
#include <spdlog/spdlog.h>

namespace tradingbot {
namespace strategies {

MeanReversionStrategy::MeanReversionStrategy(const nlohmann::json& config) {
    lookbackPeriod_ = config.value("lookback_period", 20);
    zScoreThreshold_ = config.value("z_score_threshold", 2.0);
    positionSize_ = config.value("position_size", 1.0);
    stopLoss_ = config.value("stop_loss", 0.02);
    takeProfit_ = config.value("take_profit", 0.03);
    
    reset();
}

void MeanReversionStrategy::initialize() {
    spdlog::info("Initializing MeanReversionStrategy with lookback period: {}, z-score threshold: {}",
                 lookbackPeriod_, zScoreThreshold_);
}

void MeanReversionStrategy::update(const MarketData& data) {
    priceHistory_.push_back(data.price);
    if (priceHistory_.size() > lookbackPeriod_) {
        priceHistory_.pop_front();
    }
    
    calculateStatistics();
    updatePosition(data);
}

Signal MeanReversionStrategy::generateSignal() {
    if (priceHistory_.size() < lookbackPeriod_) {
        return Signal{SignalType::HOLD, 0.0};
    }
    
    if (isPositionOpen_) {
        return Signal{SignalType::HOLD, positionSize_};
    }
    
    if (currentZScore_ < -zScoreThreshold_) {
        return Signal{SignalType::BUY, positionSize_};
    } else if (currentZScore_ > zScoreThreshold_) {
        return Signal{SignalType::SELL, positionSize_};
    }
    
    return Signal{SignalType::HOLD, 0.0};
}

void MeanReversionStrategy::reset() {
    priceHistory_.clear();
    currentZScore_ = 0.0;
    currentMean_ = 0.0;
    currentStdDev_ = 0.0;
    isPositionOpen_ = false;
    entryPrice_ = 0.0;
}

nlohmann::json MeanReversionStrategy::getStatus() const {
    return {
        {"strategy", "mean_reversion"},
        {"lookback_period", lookbackPeriod_},
        {"z_score_threshold", zScoreThreshold_},
        {"position_size", positionSize_},
        {"current_z_score", currentZScore_},
        {"current_mean", currentMean_},
        {"current_std_dev", currentStdDev_},
        {"is_position_open", isPositionOpen_},
        {"entry_price", entryPrice_}
    };
}

void MeanReversionStrategy::calculateStatistics() {
    if (priceHistory_.size() < 2) {
        return;
    }
    
    // Calculate mean
    double sum = 0.0;
    for (double price : priceHistory_) {
        sum += price;
    }
    currentMean_ = sum / priceHistory_.size();
    
    // Calculate standard deviation
    double sumSquaredDiff = 0.0;
    for (double price : priceHistory_) {
        double diff = price - currentMean_;
        sumSquaredDiff += diff * diff;
    }
    currentStdDev_ = std::sqrt(sumSquaredDiff / (priceHistory_.size() - 1));
    
    // Calculate current z-score
    if (currentStdDev_ > 0) {
        currentZScore_ = (priceHistory_.back() - currentMean_) / currentStdDev_;
    }
}

double MeanReversionStrategy::calculateZScore(double price) const {
    if (currentStdDev_ > 0) {
        return (price - currentMean_) / currentStdDev_;
    }
    return 0.0;
}

void MeanReversionStrategy::updatePosition(const MarketData& data) {
    if (!isPositionOpen_) {
        return;
    }
    
    if (shouldClosePosition(data.price)) {
        isPositionOpen_ = false;
        entryPrice_ = 0.0;
    }
}

bool MeanReversionStrategy::shouldClosePosition(double currentPrice) const {
    if (!isPositionOpen_) {
        return false;
    }
    
    double priceChange = std::abs(currentPrice - entryPrice_) / entryPrice_;
    return priceChange >= stopLoss_ || priceChange >= takeProfit_;
}

} // namespace strategies
} // namespace tradingbot 