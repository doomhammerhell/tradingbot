#pragma once

#include "strategies/IStrategy.hpp"
#include "core/Logger.hpp"
#include <vector>
#include <map>
#include <memory>
#include <deque>

namespace tradingbot {
namespace strategies {

/**
 * @brief Structure representing a strategy with its weight in the ensemble.
 * 
 * This structure holds a shared pointer to a strategy implementation,
 * its weight in the ensemble, and a name for identification.
 */
struct StrategyWeight {
    std::shared_ptr<IStrategy> strategy; ///< Pointer to the strategy implementation
    double weight;                       ///< Weight of the strategy in the ensemble
    std::string name;                    ///< Name of the strategy
};

/**
 * @brief Structure holding performance metrics for a strategy.
 * 
 * This structure tracks various performance metrics for a strategy,
 * including recent returns, win rate, and risk-adjusted metrics.
 */
struct StrategyPerformance {
    std::deque<double> recentReturns;    ///< Recent returns for performance calculation
    double winRate;                      ///< Current win rate
    double averageReturn;                ///< Average return over the performance window
    double maxDrawdown;                  ///< Maximum drawdown observed
    double sharpeRatio;                  ///< Risk-adjusted return metric
    int totalTrades;                     ///< Total number of trades
    int winningTrades;                   ///< Number of winning trades
    std::chrono::system_clock::time_point lastUpdate; ///< Last performance update time
};

/**
 * @brief Ensemble trading strategy that combines multiple strategies.
 * 
 * The EnsembleStrategy class implements a meta-strategy that combines
 * multiple trading strategies into a single decision-making system.
 * It dynamically adjusts strategy weights based on performance metrics
 * and implements consensus-based signal generation.
 * 
 * @note This strategy requires at least one sub-strategy to be added
 *       before it can generate meaningful signals.
 */
class EnsembleStrategy : public IStrategy {
public:
    /**
     * @brief Constructs an EnsembleStrategy with the given configuration.
     * 
     * @param config JSON configuration containing:
     *               - position_size: Size of positions to take
     *               - stop_loss: Stop loss percentage
     *               - take_profit: Take profit percentage
     *               - consensus_threshold: Required agreement for signals
     *               - performance_window: Number of trades to consider
     *               - min_win_rate: Minimum acceptable win rate
     *               - max_drawdown_threshold: Maximum allowed drawdown
     *               - min_sharpe_ratio: Minimum acceptable Sharpe ratio
     */
    EnsembleStrategy(const nlohmann::json& config);
    ~EnsembleStrategy() override = default;

    // IStrategy interface implementation
    void initialize() override;
    void update(const MarketData& data) override;
    Signal generateSignal() override;
    void reset() override;
    nlohmann::json getStatus() const override;

    /**
     * @brief Adds a strategy to the ensemble.
     * 
     * @param strategy Shared pointer to the strategy implementation
     * @param weight Initial weight of the strategy
     * @param name Unique identifier for the strategy
     */
    void addStrategy(std::shared_ptr<IStrategy> strategy, double weight, const std::string& name);

    /**
     * @brief Removes a strategy from the ensemble.
     * 
     * @param name Name of the strategy to remove
     */
    void removeStrategy(const std::string& name);

    /**
     * @brief Updates the weights of all strategies in the ensemble.
     * 
     * @param newWeights Map of strategy names to their new weights
     */
    void updateWeights(const std::map<std::string, double>& newWeights);

    /**
     * @brief Updates the performance metrics for a strategy.
     * 
     * @param strategyName Name of the strategy to update
     * @param returnValue Return value of the trade
     * @param isWin Whether the trade was a win
     */
    void updatePerformance(const std::string& strategyName, double returnValue, bool isWin);

private:
    // Configuration
    double positionSize_;                ///< Size of positions to take
    double stopLoss_;                    ///< Stop loss percentage
    double takeProfit_;                  ///< Take profit percentage
    double consensusThreshold_;          ///< Required agreement for signals
    int performanceWindow_;              ///< Number of trades to consider
    double minWinRate_;                  ///< Minimum acceptable win rate
    double maxDrawdownThreshold_;        ///< Maximum allowed drawdown
    double minSharpeRatio_;              ///< Minimum acceptable Sharpe ratio

    // State variables
    std::vector<StrategyWeight> strategies_;     ///< List of strategies in the ensemble
    std::map<std::string, Signal> lastSignals_;  ///< Last signals from each strategy
    std::map<std::string, StrategyPerformance> strategyPerformance_; ///< Performance metrics
    bool isPositionOpen_;                       ///< Whether a position is currently open
    double entryPrice_;                         ///< Entry price of current position
    std::chrono::system_clock::time_point lastWeightUpdate_; ///< Last weight update time

    // Helper methods
    Signal combineSignals() const;
    void updatePosition(const MarketData& data);
    bool shouldClosePosition(double currentPrice) const;
    double calculateStrategyScore(const Signal& signal, const std::string& strategyName) const;
    void updateStrategyWeights();
    void calculatePerformanceMetrics(const std::string& strategyName);
    double calculateSharpeRatio(const std::deque<double>& returns) const;
    double calculateMaxDrawdown(const std::deque<double>& returns) const;
    bool isStrategyValid(const std::string& strategyName) const;
};

} // namespace strategies
} // namespace tradingbot 