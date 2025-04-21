#include "core/FeatureEngine.hpp"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace tradingbot {
namespace core {

FeatureEngine::FeatureEngine(const nlohmann::json& config)
    : config_(config), isInitialized_(false) {
    reset();
}

void FeatureEngine::initialize() {
    spdlog::info("Initializing FeatureEngine");
    isInitialized_ = true;
}

void FeatureEngine::update(const MarketData& data) {
    updatePriceHistory(data);
    calculateFeatures();
}

void FeatureEngine::calculateFeatures() {
    // Calculate technical indicators
    calculateSMA(config_["sma_period"]);
    calculateEMA(config_["ema_period"]);
    calculateRSI(config_["rsi_period"]);
    calculateMACD(config_["macd_fast_period"], 
                 config_["macd_slow_period"], 
                 config_["macd_signal_period"]);
    calculateBollingerBands(config_["bb_period"], 
                          config_["bb_std_dev"]);
    calculateATR(config_["atr_period"]);
    
    // Generate derived features
    generateFeatures();
    normalizeFeatures();
    calculateDeltas();
}

const std::map<std::string, double>& FeatureEngine::getFeatures() const {
    return features_;
}

void FeatureEngine::exportFeatures(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        spdlog::error("Failed to open file for feature export: {}", filename);
        return;
    }
    
    // Write header
    file << "timestamp,price,volume";
    for (const auto& [name, _] : features_) {
        file << "," << name;
    }
    file << "\n";
    
    // Write data
    for (const auto& data : marketData_) {
        file << std::chrono::system_clock::to_time_t(data.timestamp) << ","
             << data.price << "," << data.volume;
        
        for (const auto& [name, value] : features_) {
            file << "," << value;
        }
        file << "\n";
    }
    
    spdlog::info("Exported features to {}", filename);
}

void FeatureEngine::reset() {
    marketData_.clear();
    features_.clear();
    indicatorHistory_.clear();
    isInitialized_ = false;
}

void FeatureEngine::calculateSMA(int period) {
    if (marketData_.size() < period) {
        return;
    }
    
    std::deque<double> prices;
    for (const auto& data : marketData_) {
        prices.push_back(data.price);
    }
    
    double sma = calculateMean(prices);
    features_["sma_" + std::to_string(period)] = sma;
}

void FeatureEngine::calculateEMA(int period) {
    if (marketData_.size() < period) {
        return;
    }
    
    double multiplier = 2.0 / (period + 1);
    double ema = marketData_[0].price;
    
    for (size_t i = 1; i < marketData_.size(); ++i) {
        ema = (marketData_[i].price - ema) * multiplier + ema;
    }
    
    features_["ema_" + std::to_string(period)] = ema;
}

void FeatureEngine::calculateRSI(int period) {
    if (marketData_.size() < period + 1) {
        return;
    }
    
    std::vector<double> gains;
    std::vector<double> losses;
    
    for (size_t i = 1; i < marketData_.size(); ++i) {
        double change = marketData_[i].price - marketData_[i-1].price;
        if (change >= 0) {
            gains.push_back(change);
            losses.push_back(0);
        } else {
            gains.push_back(0);
            losses.push_back(-change);
        }
    }
    
    double avgGain = calculateMean(std::deque<double>(gains.begin(), gains.end()));
    double avgLoss = calculateMean(std::deque<double>(losses.begin(), losses.end()));
    
    double rs = avgGain / (avgLoss + 1e-10);
    double rsi = 100 - (100 / (1 + rs));
    
    features_["rsi_" + std::to_string(period)] = rsi;
}

void FeatureEngine::calculateMACD(int fastPeriod, int slowPeriod, int signalPeriod) {
    // Implementation of MACD calculation
    // This is a simplified version - you might want to implement a more robust version
    calculateEMA(fastPeriod);
    calculateEMA(slowPeriod);
    
    double macd = features_["ema_" + std::to_string(fastPeriod)] - 
                  features_["ema_" + std::to_string(slowPeriod)];
    
    features_["macd"] = macd;
}

void FeatureEngine::calculateBollingerBands(int period, double stdDev) {
    if (marketData_.size() < period) {
        return;
    }
    
    std::deque<double> prices;
    for (const auto& data : marketData_) {
        prices.push_back(data.price);
    }
    
    double mean = calculateMean(prices);
    double std = calculateStdDev(prices);
    
    features_["bb_middle"] = mean;
    features_["bb_upper"] = mean + (std * stdDev);
    features_["bb_lower"] = mean - (std * stdDev);
}

void FeatureEngine::calculateATR(int period) {
    if (marketData_.size() < period + 1) {
        return;
    }
    
    std::vector<double> trueRanges;
    for (size_t i = 1; i < marketData_.size(); ++i) {
        double high = marketData_[i].price;
        double low = marketData_[i-1].price;
        double tr = std::abs(high - low);
        trueRanges.push_back(tr);
    }
    
    double atr = calculateMean(std::deque<double>(trueRanges.begin(), trueRanges.end()));
    features_["atr_" + std::to_string(period)] = atr;
}

void FeatureEngine::generateFeatures() {
    // Add any additional feature generation logic here
}

void FeatureEngine::normalizeFeatures() {
    // Implement feature normalization if needed
}

void FeatureEngine::calculateDeltas() {
    // Calculate deltas between consecutive values of indicators
}

double FeatureEngine::calculateMean(const std::deque<double>& data) const {
    if (data.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double value : data) {
        sum += value;
    }
    return sum / data.size();
}

double FeatureEngine::calculateStdDev(const std::deque<double>& data) const {
    if (data.size() < 2) {
        return 0.0;
    }
    
    double mean = calculateMean(data);
    double sumSquaredDiff = 0.0;
    
    for (double value : data) {
        double diff = value - mean;
        sumSquaredDiff += diff * diff;
    }
    
    return std::sqrt(sumSquaredDiff / (data.size() - 1));
}

void FeatureEngine::updatePriceHistory(const MarketData& data) {
    marketData_.push_back(data);
    if (marketData_.size() > config_["max_history"]) {
        marketData_.pop_front();
    }
}

} // namespace core
} // namespace tradingbot 