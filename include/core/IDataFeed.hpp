#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace tradingbot {
namespace core {

struct MarketData {
    std::vector<double> prices;
    std::vector<double> volumes;
    std::vector<long> timestamps;
    std::string symbol;
    std::string timeframe;
};

class IDataFeed {
public:
    virtual ~IDataFeed() = default;

    // Initialize data feed with symbol and timeframe
    virtual void initialize(const std::string& symbol, const std::string& timeframe) = 0;

    // Start streaming market data
    virtual void startStreaming() = 0;

    // Stop streaming market data
    virtual void stopStreaming() = 0;

    // Get historical data
    virtual MarketData getHistoricalData(int numBars) const = 0;

    // Subscribe to market data updates
    virtual void subscribe(std::function<void(const MarketData&)> callback) = 0;

    // Get data feed name
    virtual std::string getName() const = 0;
};

} // namespace core
} // namespace tradingbot 