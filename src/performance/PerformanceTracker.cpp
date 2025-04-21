#include "performance/PerformanceTracker.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace tradingbot {
namespace performance {

PerformanceTracker::PerformanceTracker()
    : peakBalance_(0.0)
    , currentBalance_(0.0)
    , maxDrawdown_(0.0) {
}

void PerformanceTracker::addTrade(const Trade& trade) {
    trades_.push_back(trade);
    currentBalance_ += trade.pnl;
    symbolPnL_[trade.symbol] += trade.pnl;
    updateDrawdown(currentBalance_);
}

void PerformanceTracker::updatePosition(const std::string& symbol, double quantity, double currentPrice) {
    currentPositions_[symbol] = quantity * currentPrice;
}

PerformanceMetrics PerformanceTracker::getMetrics() const {
    PerformanceMetrics metrics;
    calculateMetrics(metrics);
    return metrics;
}

std::string PerformanceTracker::generateReport() const {
    PerformanceMetrics metrics = getMetrics();
    std::stringstream ss;
    
    ss << "\n=== Performance Report ===\n\n";
    ss << "Trading Statistics:\n";
    ss << "  Total Trades: " << metrics.totalTrades << "\n";
    ss << "  Winning Trades: " << metrics.winningTrades << "\n";
    ss << "  Losing Trades: " << metrics.losingTrades << "\n";
    ss << "  Win Ratio: " << std::fixed << std::setprecision(2) << (metrics.winRatio * 100) << "%\n";
    ss << "  Total PnL: " << std::fixed << std::setprecision(2) << metrics.totalPnL << "\n";
    ss << "  Max Drawdown: " << std::fixed << std::setprecision(2) << (metrics.maxDrawdown * 100) << "%\n\n";
    
    ss << "Risk Metrics:\n";
    ss << "  Sharpe Ratio: " << std::fixed << std::setprecision(2) << metrics.sharpeRatio << "\n";
    ss << "  Sortino Ratio: " << std::fixed << std::setprecision(2) << metrics.sortinoRatio << "\n";
    ss << "  Profit Factor: " << std::fixed << std::setprecision(2) << metrics.profitFactor << "\n\n";
    
    ss << "Trade Statistics:\n";
    ss << "  Average Trade: " << std::fixed << std::setprecision(2) << metrics.averageTrade << "\n";
    ss << "  Average Win: " << std::fixed << std::setprecision(2) << metrics.averageWin << "\n";
    ss << "  Average Loss: " << std::fixed << std::setprecision(2) << metrics.averageLoss << "\n";
    ss << "  Largest Win: " << std::fixed << std::setprecision(2) << metrics.largestWin << "\n";
    ss << "  Largest Loss: " << std::fixed << std::setprecision(2) << metrics.largestLoss << "\n";
    ss << "  Average Duration: " << metrics.averageTradeDuration.count() << " seconds\n\n";
    
    ss << "Symbol Performance:\n";
    for (const auto& pair : metrics.symbolMetrics) {
        ss << "  " << pair.first << ": " << std::fixed << std::setprecision(2) << pair.second << "\n";
    }
    
    return ss.str();
}

void PerformanceTracker::reset() {
    trades_.clear();
    currentPositions_.clear();
    symbolPnL_.clear();
    peakBalance_ = 0.0;
    currentBalance_ = 0.0;
    maxDrawdown_ = 0.0;
}

void PerformanceTracker::calculateMetrics(PerformanceMetrics& metrics) const {
    metrics.totalTrades = trades_.size();
    metrics.winningTrades = std::count_if(trades_.begin(), trades_.end(), [](const Trade& t) { return t.isWin; });
    metrics.losingTrades = metrics.totalTrades - metrics.winningTrades;
    metrics.winRatio = metrics.totalTrades > 0 ? static_cast<double>(metrics.winningTrades) / metrics.totalTrades : 0.0;
    metrics.totalPnL = currentBalance_;
    metrics.maxDrawdown = maxDrawdown_;
    metrics.sharpeRatio = calculateSharpeRatio();
    metrics.sortinoRatio = calculateSortinoRatio();
    metrics.profitFactor = calculateProfitFactor();
    
    // Calculate trade statistics
    if (!trades_.empty()) {
        metrics.averageTrade = std::accumulate(trades_.begin(), trades_.end(), 0.0,
            [](double sum, const Trade& t) { return sum + t.pnl; }) / trades_.size();
        
        auto winningTrades = std::vector<Trade>();
        auto losingTrades = std::vector<Trade>();
        std::copy_if(trades_.begin(), trades_.end(), std::back_inserter(winningTrades),
            [](const Trade& t) { return t.isWin; });
        std::copy_if(trades_.begin(), trades_.end(), std::back_inserter(losingTrades),
            [](const Trade& t) { return !t.isWin; });
        
        metrics.averageWin = winningTrades.empty() ? 0.0 :
            std::accumulate(winningTrades.begin(), winningTrades.end(), 0.0,
                [](double sum, const Trade& t) { return sum + t.pnl; }) / winningTrades.size();
        
        metrics.averageLoss = losingTrades.empty() ? 0.0 :
            std::accumulate(losingTrades.begin(), losingTrades.end(), 0.0,
                [](double sum, const Trade& t) { return sum + t.pnl; }) / losingTrades.size();
        
        metrics.largestWin = winningTrades.empty() ? 0.0 :
            std::max_element(winningTrades.begin(), winningTrades.end(),
                [](const Trade& a, const Trade& b) { return a.pnl < b.pnl; })->pnl;
        
        metrics.largestLoss = losingTrades.empty() ? 0.0 :
            std::min_element(losingTrades.begin(), losingTrades.end(),
                [](const Trade& a, const Trade& b) { return a.pnl < b.pnl; })->pnl;
        
        // Calculate average trade duration
        auto totalDuration = std::accumulate(trades_.begin(), trades_.end(), std::chrono::seconds(0),
            [](std::chrono::seconds sum, const Trade& t) {
                return sum + std::chrono::duration_cast<std::chrono::seconds>(t.exitTime - t.entryTime);
            });
        metrics.averageTradeDuration = totalDuration / trades_.size();
    }
    
    // Copy symbol metrics
    metrics.symbolMetrics = symbolPnL_;
}

void PerformanceTracker::updateDrawdown(double newBalance) {
    if (newBalance > peakBalance_) {
        peakBalance_ = newBalance;
    }
    double drawdown = (peakBalance_ - newBalance) / peakBalance_;
    if (drawdown > maxDrawdown_) {
        maxDrawdown_ = drawdown;
    }
}

double PerformanceTracker::calculateSharpeRatio() const {
    if (trades_.empty()) return 0.0;
    
    // Calculate average return
    double avgReturn = std::accumulate(trades_.begin(), trades_.end(), 0.0,
        [](double sum, const Trade& t) { return sum + t.pnl; }) / trades_.size();
    
    // Calculate standard deviation
    double variance = std::accumulate(trades_.begin(), trades_.end(), 0.0,
        [avgReturn](double sum, const Trade& t) {
            double diff = t.pnl - avgReturn;
            return sum + (diff * diff);
        }) / trades_.size();
    
    double stdDev = std::sqrt(variance);
    return stdDev > 0.0 ? avgReturn / stdDev : 0.0;
}

double PerformanceTracker::calculateSortinoRatio() const {
    if (trades_.empty()) return 0.0;
    
    // Calculate average return
    double avgReturn = std::accumulate(trades_.begin(), trades_.end(), 0.0,
        [](double sum, const Trade& t) { return sum + t.pnl; }) / trades_.size();
    
    // Calculate downside deviation
    double downsideVariance = std::accumulate(trades_.begin(), trades_.end(), 0.0,
        [](double sum, const Trade& t) {
            return sum + (t.pnl < 0.0 ? t.pnl * t.pnl : 0.0);
        }) / trades_.size();
    
    double downsideDeviation = std::sqrt(downsideVariance);
    return downsideDeviation > 0.0 ? avgReturn / downsideDeviation : 0.0;
}

double PerformanceTracker::calculateProfitFactor() const {
    double grossProfit = 0.0;
    double grossLoss = 0.0;
    
    for (const auto& trade : trades_) {
        if (trade.pnl > 0.0) {
            grossProfit += trade.pnl;
        } else {
            grossLoss += std::abs(trade.pnl);
        }
    }
    
    return grossLoss > 0.0 ? grossProfit / grossLoss : 0.0;
}

} // namespace performance
} // namespace tradingbot 