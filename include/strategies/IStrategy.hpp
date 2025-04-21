#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>

namespace tradingbot {
namespace strategies {

struct MarketData {
    std::string symbol;
    std::chrono::system_clock::time_point timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

enum class DecisionType {
    BUY,
    SELL,
    HOLD
};

struct Decision {
    DecisionType type;
    double price;
    double quantity;
    std::string reason;
};

class IStrategy {
public:
    virtual ~IStrategy() = default;
    
    // Initialize strategy with configuration
    virtual void initialize(const std::string& config) = 0;
    
    // Analyze market data and make trading decision
    virtual Decision analyze(const std::vector<MarketData>& marketData) = 0;
    
    // Update strategy state after order execution
    virtual void updateOrderStatus(const std::string& orderId, bool success) = 0;
    
    // Get strategy metrics and performance data
    virtual std::string getMetrics() const = 0;
};

} // namespace strategies
} // namespace tradingbot 