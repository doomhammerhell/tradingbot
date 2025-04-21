#pragma once

#include "strategies/IStrategy.hpp"
#include "execution/IExecutionEngine.hpp"
#include "risk/IRiskManager.hpp"
#include "data/IDataFeed.hpp"
#include <memory>
#include <string>
#include <chrono>

namespace tradingbot {
namespace core {

enum class RunMode {
    LIVE,
    BACKTEST
};

class TradingEngine {
public:
    TradingEngine(
        std::unique_ptr<strategies::IStrategy> strategy,
        std::unique_ptr<execution::IExecutionEngine> executionEngine,
        std::unique_ptr<risk::IRiskManager> riskManager,
        std::unique_ptr<data::IDataFeed> dataFeed
    );
    
    // Initialize the trading engine with configuration
    void initialize(const std::string& config);
    
    // Start the trading engine
    void start();
    
    // Stop the trading engine
    void stop();
    
    // Get engine metrics and statistics
    std::string getMetrics() const;
    
private:
    // Process market data and make trading decisions
    void processMarketData(const std::string& marketData);
    
    // Execute a trading decision
    void executeDecision(const strategies::Decision& decision);
    
    // Update portfolio state
    void updatePortfolio(const execution::Order& order, bool success);
    
    // Configuration
    RunMode runMode_;
    std::string symbol_;
    std::string timeframe_;
    double initialBalance_;
    bool isRunning_;
    
    // Components
    std::unique_ptr<strategies::IStrategy> strategy_;
    std::unique_ptr<execution::IExecutionEngine> executionEngine_;
    std::unique_ptr<risk::IRiskManager> riskManager_;
    std::unique_ptr<data::IDataFeed> dataFeed_;
    
    // State
    risk::Portfolio portfolio_;
    std::chrono::system_clock::time_point startTime_;
    
    // Metrics
    size_t totalTrades_;
    size_t successfulTrades_;
    size_t failedTrades_;
    double totalPnL_;
};

} // namespace core
} // namespace tradingbot 