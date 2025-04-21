#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace tradingbot {
namespace performance {

struct Trade {
    std::string symbol;
    std::chrono::system_clock::time_point entryTime;
    std::chrono::system_clock::time_point exitTime;
    double entryPrice;
    double exitPrice;
    double quantity;
    double pnl;
    bool isWin;
};

struct PerformanceMetrics {
    int totalTrades;
    int winningTrades;
    int losingTrades;
    double winRatio;
    double totalPnL;
    double maxDrawdown;
    double sharpeRatio;
    double sortinoRatio;
    double profitFactor;
    double averageTrade;
    double averageWin;
    double averageLoss;
    double largestWin;
    double largestLoss;
    std::chrono::seconds averageTradeDuration;
    std::map<std::string, double> symbolMetrics;
};

class PerformanceTracker {
public:
    PerformanceTracker();
    ~PerformanceTracker() = default;

    // Track a new trade
    void addTrade(const Trade& trade);

    // Update current position
    void updatePosition(const std::string& symbol, double quantity, double currentPrice);

    // Calculate and get current metrics
    PerformanceMetrics getMetrics() const;

    // Generate performance report
    std::string generateReport() const;

    // Reset all metrics
    void reset();

private:
    std::vector<Trade> trades_;
    std::map<std::string, double> currentPositions_;
    std::map<std::string, double> symbolPnL_;
    double peakBalance_;
    double currentBalance_;
    double maxDrawdown_;

    void calculateMetrics(PerformanceMetrics& metrics) const;
    void updateDrawdown(double newBalance);
    double calculateSharpeRatio() const;
    double calculateSortinoRatio() const;
    double calculateProfitFactor() const;
};

} // namespace performance
} // namespace tradingbot 