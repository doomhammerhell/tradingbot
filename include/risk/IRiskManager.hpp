#pragma once

#include <string>
#include <memory>
#include <map>
#include <chrono>
#include "execution/IExecutionEngine.hpp"

namespace tradingbot {
namespace risk {

struct Position {
    std::string symbol;
    double quantity;
    double averagePrice;
    double currentPrice;
    double unrealizedPnL;
    double realizedPnL;
    std::chrono::system_clock::time_point entryTime;
};

struct Portfolio {
    double totalBalance;
    double availableBalance;
    double totalPnL;
    std::map<std::string, Position> positions;
    std::map<std::string, double> assetAllocation;
};

class IRiskManager {
public:
    virtual ~IRiskManager() = default;
    
    // Initialize risk manager with configuration
    virtual void initialize(const std::string& config) = 0;
    
    // Check if an order should be executed based on risk rules
    virtual bool shouldExecute(const execution::Order& order, const Portfolio& portfolio) = 0;
    
    // Calculate appropriate position size based on risk parameters
    virtual double calculatePositionSize(const execution::Order& order, const Portfolio& portfolio) = 0;
    
    // Update risk manager state after order execution
    virtual void updateAfterExecution(const execution::Order& order, bool success) = 0;
    
    // Get risk metrics and statistics
    virtual std::string getMetrics() const = 0;
};

} // namespace risk
} // namespace tradingbot 