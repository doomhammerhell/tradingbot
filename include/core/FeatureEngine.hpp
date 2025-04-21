#pragma once

#include <vector>
#include <deque>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include "core/Logger.hpp"

namespace tradingbot {
namespace core {

struct MarketData {
    double price;
    double volume;
    std::chrono::system_clock::time_point timestamp;
};

class FeatureEngine {
public:
    FeatureEngine(const nlohmann::json& config);
    ~FeatureEngine() = default;

    // Initialize the feature engine
    void initialize();
    
    // Update with new market data
    void update(const MarketData& data);
    
    // Calculate all features
    void calculateFeatures();
    
    // Get current feature values
    const std::map<std::string, double>& getFeatures() const;
    
    // Export features to CSV
    void exportFeatures(const std::string& filename) const;
    
    // Reset the feature engine
    void reset();

private:
    // Technical indicators
    void calculateSMA(int period);
    void calculateEMA(int period);
    void calculateRSI(int period);
    void calculateMACD(int fastPeriod, int slowPeriod, int signalPeriod);
    void calculateBollingerBands(int period, double stdDev);
    void calculateATR(int period);
    
    // Feature generation
    void generateFeatures();
    void normalizeFeatures();
    void calculateDeltas();
    
    // Helper methods
    double calculateMean(const std::deque<double>& data) const;
    double calculateStdDev(const std::deque<double>& data) const;
    void updatePriceHistory(const MarketData& data);
    
    // Configuration
    nlohmann::json config_;
    
    // Data storage
    std::deque<MarketData> marketData_;
    std::map<std::string, double> features_;
    std::map<std::string, std::deque<double>> indicatorHistory_;
    
    // State
    bool isInitialized_;
};

} // namespace core
} // namespace tradingbot 