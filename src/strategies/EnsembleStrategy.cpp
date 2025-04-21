#include "strategies/EnsembleStrategy.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace tradingbot {
namespace strategies {

EnsembleStrategy::EnsembleStrategy(const nlohmann::json& config) {
    positionSize_ = config.value("position_size", 1.0);
    stopLoss_ = config.value("stop_loss", 0.02);
    takeProfit_ = config.value("take_profit", 0.03);
    consensusThreshold_ = config.value("consensus_threshold", 0.6);
    performanceWindow_ = config.value("performance_window", 100);
    minWinRate_ = config.value("min_win_rate", 0.4);
    maxDrawdownThreshold_ = config.value("max_drawdown_threshold", 0.2);
    minSharpeRatio_ = config.value("min_sharpe_ratio", 0.5);
    
    reset();
}

void EnsembleStrategy::initialize() {
    spdlog::info("Initializing EnsembleStrategy with {} strategies", strategies_.size());
    for (const auto& strategy : strategies_) {
        strategy.strategy->initialize();
        strategyPerformance_[strategy.name] = StrategyPerformance{
            std::deque<double>(),
            0.0,
            0.0,
            0.0,
            0.0,
            0,
            0,
            std::chrono::system_clock::now()
        };
    }
}

void EnsembleStrategy::update(const MarketData& data) {
    for (auto& strategy : strategies_) {
        strategy.strategy->update(data);
        lastSignals_[strategy.name] = strategy.strategy->generateSignal();
    }
    
    // Update weights periodically
    auto now = std::chrono::system_clock::now();
    if (std::chrono::duration_cast<std::chrono::hours>(now - lastWeightUpdate_).count() >= 24) {
        updateStrategyWeights();
        lastWeightUpdate_ = now;
    }
    
    updatePosition(data);
}

Signal EnsembleStrategy::generateSignal() {
    if (strategies_.empty()) {
        return Signal{SignalType::HOLD, 0.0};
    }
    
    if (isPositionOpen_) {
        return Signal{SignalType::HOLD, positionSize_};
    }
    
    return combineSignals();
}

void EnsembleStrategy::reset() {
    for (auto& strategy : strategies_) {
        strategy.strategy->reset();
    }
    lastSignals_.clear();
    strategyPerformance_.clear();
    isPositionOpen_ = false;
    entryPrice_ = 0.0;
    lastWeightUpdate_ = std::chrono::system_clock::now();
}

nlohmann::json EnsembleStrategy::getStatus() const {
    nlohmann::json status = {
        {"strategy", "ensemble"},
        {"position_size", positionSize_},
        {"num_strategies", strategies_.size()},
        {"is_position_open", isPositionOpen_},
        {"entry_price", entryPrice_}
    };
    
    nlohmann::json strategyStatus;
    for (const auto& strategy : strategies_) {
        const auto& perf = strategyPerformance_.at(strategy.name);
        strategyStatus[strategy.name] = {
            {"weight", strategy.weight},
            {"win_rate", perf.winRate},
            {"average_return", perf.averageReturn},
            {"max_drawdown", perf.maxDrawdown},
            {"sharpe_ratio", perf.sharpeRatio},
            {"total_trades", perf.totalTrades},
            {"winning_trades", perf.winningTrades},
            {"status", strategy.strategy->getStatus()}
        };
    }
    status["strategies"] = strategyStatus;
    
    return status;
}

void EnsembleStrategy::addStrategy(std::shared_ptr<IStrategy> strategy, 
                                 double weight, 
                                 const std::string& name) {
    strategies_.push_back({strategy, weight, name});
    spdlog::info("Added strategy {} with weight {}", name, weight);
}

void EnsembleStrategy::removeStrategy(const std::string& name) {
    strategies_.erase(
        std::remove_if(strategies_.begin(), strategies_.end(),
            [&name](const StrategyWeight& sw) { return sw.name == name; }),
        strategies_.end()
    );
    spdlog::info("Removed strategy {}", name);
}

void EnsembleStrategy::updateWeights(const std::map<std::string, double>& newWeights) {
    for (auto& strategy : strategies_) {
        if (newWeights.count(strategy.name)) {
            strategy.weight = newWeights.at(strategy.name);
        }
    }
    spdlog::info("Updated strategy weights");
}

void EnsembleStrategy::updatePerformance(const std::string& strategyName, double returnValue, bool isWin) {
    auto& perf = strategyPerformance_[strategyName];
    perf.recentReturns.push_back(returnValue);
    if (perf.recentReturns.size() > performanceWindow_) {
        perf.recentReturns.pop_front();
    }
    
    perf.totalTrades++;
    if (isWin) {
        perf.winningTrades++;
    }
    
    calculatePerformanceMetrics(strategyName);
}

void EnsembleStrategy::calculatePerformanceMetrics(const std::string& strategyName) {
    auto& perf = strategyPerformance_[strategyName];
    
    if (perf.recentReturns.empty()) {
        return;
    }
    
    // Calculate win rate
    perf.winRate = static_cast<double>(perf.winningTrades) / perf.totalTrades;
    
    // Calculate average return
    perf.averageReturn = std::accumulate(perf.recentReturns.begin(), 
                                       perf.recentReturns.end(), 
                                       0.0) / perf.recentReturns.size();
    
    // Calculate Sharpe ratio
    perf.sharpeRatio = calculateSharpeRatio(perf.recentReturns);
    
    // Calculate max drawdown
    perf.maxDrawdown = calculateMaxDrawdown(perf.recentReturns);
    
    perf.lastUpdate = std::chrono::system_clock::now();
}

double EnsembleStrategy::calculateSharpeRatio(const std::deque<double>& returns) const {
    if (returns.empty()) {
        return 0.0;
    }
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double variance = 0.0;
    for (double r : returns) {
        variance += std::pow(r - mean, 2);
    }
    variance /= returns.size();
    double stdDev = std::sqrt(variance);
    
    // Assuming risk-free rate of 0 for simplicity
    return stdDev > 0 ? mean / stdDev : 0.0;
}

double EnsembleStrategy::calculateMaxDrawdown(const std::deque<double>& returns) const {
    if (returns.empty()) {
        return 0.0;
    }
    
    double peak = returns[0];
    double maxDrawdown = 0.0;
    
    for (double r : returns) {
        if (r > peak) {
            peak = r;
        }
        double drawdown = (peak - r) / peak;
        maxDrawdown = std::max(maxDrawdown, drawdown);
    }
    
    return maxDrawdown;
}

bool EnsembleStrategy::isStrategyValid(const std::string& strategyName) const {
    const auto& perf = strategyPerformance_.at(strategyName);
    return perf.winRate >= minWinRate_ &&
           perf.maxDrawdown <= maxDrawdownThreshold_ &&
           perf.sharpeRatio >= minSharpeRatio_;
}

void EnsembleStrategy::updateStrategyWeights() {
    double totalScore = 0.0;
    std::map<std::string, double> newWeights;
    
    for (const auto& strategy : strategies_) {
        if (!isStrategyValid(strategy.name)) {
            newWeights[strategy.name] = 0.0;
            continue;
        }
        
        const auto& perf = strategyPerformance_.at(strategy.name);
        double score = perf.winRate * 0.4 + 
                      (1.0 - perf.maxDrawdown) * 0.3 + 
                      perf.sharpeRatio * 0.3;
        
        newWeights[strategy.name] = score;
        totalScore += score;
    }
    
    if (totalScore > 0) {
        for (auto& strategy : strategies_) {
            strategy.weight = newWeights[strategy.name] / totalScore;
        }
    }
}

double EnsembleStrategy::calculateStrategyScore(const Signal& signal, const std::string& strategyName) const {
    const auto& perf = strategyPerformance_.at(strategyName);
    
    if (!isStrategyValid(strategyName)) {
        return 0.0;
    }
    
    // Base score from performance metrics
    double baseScore = perf.winRate * 0.4 + 
                      (1.0 - perf.maxDrawdown) * 0.3 + 
                      perf.sharpeRatio * 0.3;
    
    // Adjust score based on signal type and recent performance
    double signalScore = 1.0;
    if (signal.type == SignalType::BUY) {
        // Favor strategies with positive recent returns
        signalScore *= 1.0 + std::max(0.0, perf.averageReturn);
    } else if (signal.type == SignalType::SELL) {
        // Favor strategies with negative recent returns
        signalScore *= 1.0 + std::max(0.0, -perf.averageReturn);
    }
    
    return baseScore * signalScore;
}

Signal EnsembleStrategy::combineSignals() const {
    double buyScore = 0.0;
    double sellScore = 0.0;
    double totalWeight = 0.0;
    
    for (const auto& strategy : strategies_) {
        if (!isStrategyValid(strategy.name)) {
            continue;
        }
        
        const auto& signal = lastSignals_.at(strategy.name);
        double score = calculateStrategyScore(signal, strategy.name);
        
        if (signal.type == SignalType::BUY) {
            buyScore += score * strategy.weight;
        } else if (signal.type == SignalType::SELL) {
            sellScore += score * strategy.weight;
        }
        
        totalWeight += strategy.weight;
    }
    
    if (totalWeight > 0) {
        buyScore /= totalWeight;
        sellScore /= totalWeight;
        
        if (buyScore > consensusThreshold_) {
            return Signal{SignalType::BUY, positionSize_};
        } else if (sellScore > consensusThreshold_) {
            return Signal{SignalType::SELL, positionSize_};
        }
    }
    
    return Signal{SignalType::HOLD, 0.0};
}

void EnsembleStrategy::updatePosition(const MarketData& data) {
    if (!isPositionOpen_) {
        return;
    }
    
    if (shouldClosePosition(data.price)) {
        isPositionOpen_ = false;
        entryPrice_ = 0.0;
    }
}

bool EnsembleStrategy::shouldClosePosition(double currentPrice) const {
    if (!isPositionOpen_) {
        return false;
    }
    
    double priceChange = std::abs(currentPrice - entryPrice_) / entryPrice_;
    return priceChange >= stopLoss_ || priceChange >= takeProfit_;
}

} // namespace strategies
} // namespace tradingbot 