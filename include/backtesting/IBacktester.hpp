#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include "../core/MarketData.hpp"
#include "../core/Order.hpp"
#include "../strategies/IStrategy.hpp"

namespace tradingbot {
namespace backtesting {

struct BacktestConfig {
    std::string symbol;
    std::string timeframe;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    double initialCapital;
    double slippage; // Percentage of price movement
    std::chrono::milliseconds latency; // Order execution latency
    bool useRealisticMarketConditions;
    std::map<std::string, double> strategyParameters;
};

struct PerformanceMetrics {
    double totalReturn;
    double sharpeRatio;
    double sortinoRatio;
    double maxDrawdown;
    double winRate;
    double profitFactor;
    double averageTrade;
    double standardDeviation;
    double valueAtRisk;
    double expectedShortfall;
    std::vector<double> returns;
    std::vector<double> drawdowns;
    std::vector<std::chrono::system_clock::time_point> timestamps;
};

struct Trade {
    core::Order order;
    double entryPrice;
    double exitPrice;
    double pnl;
    std::chrono::system_clock::time_point entryTime;
    std::chrono::system_clock::time_point exitTime;
    std::string reason;
};

class IBacktester {
public:
    virtual ~IBacktester() = default;

    // Configuration
    virtual void initialize(const BacktestConfig& config) = 0;
    virtual void setStrategy(std::shared_ptr<strategies::IStrategy> strategy) = 0;
    virtual void setMarketData(const std::vector<core::MarketData>& data) = 0;

    // Execution
    virtual void run() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;

    // Results
    virtual PerformanceMetrics getPerformanceMetrics() const = 0;
    virtual std::vector<Trade> getTrades() const = 0;
    virtual std::vector<core::MarketData> getMarketData() const = 0;
    virtual std::vector<double> getEquityCurve() const = 0;
    virtual std::vector<double> getDrawdownCurve() const = 0;

    // Optimization
    virtual void optimizeParameters(
        const std::map<std::string, std::pair<double, double>>& parameterRanges,
        size_t populationSize,
        size_t generations,
        const std::string& optimizationMetric) = 0;

    // Analysis
    virtual void analyzeTrades() = 0;
    virtual void generateReport(const std::string& outputPath) = 0;
    virtual void visualizeResults() = 0;

    // Replay
    virtual void startReplay() = 0;
    virtual void stopReplay() = 0;
    virtual void setReplaySpeed(double speed) = 0;
    virtual void jumpToTime(std::chrono::system_clock::time_point time) = 0;

    // Events
    virtual void subscribeToTradeUpdates(
        std::function<void(const Trade&)> callback) = 0;
    virtual void subscribeToPerformanceUpdates(
        std::function<void(const PerformanceMetrics&)> callback) = 0;
    virtual void subscribeToMarketDataUpdates(
        std::function<void(const core::MarketData&)> callback) = 0;
};

} // namespace backtesting
} // namespace tradingbot 