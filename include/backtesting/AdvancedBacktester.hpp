#pragma once

#include "IBacktester.hpp"
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <random>
#include <Eigen/Dense>
#include <matplotlibcpp.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_statistics.h>

namespace tradingbot {
namespace backtesting {

class AdvancedBacktester : public IBacktester {
public:
    AdvancedBacktester();
    ~AdvancedBacktester() override;

    // Configuration
    void initialize(const BacktestConfig& config) override;
    void setStrategy(std::shared_ptr<strategies::IStrategy> strategy) override;
    void setMarketData(const std::vector<core::MarketData>& data) override;

    // Execution
    void run() override;
    void pause() override;
    void resume() override;
    void stop() override;

    // Results
    PerformanceMetrics getPerformanceMetrics() const override;
    std::vector<Trade> getTrades() const override;
    std::vector<core::MarketData> getMarketData() const override;
    std::vector<double> getEquityCurve() const override;
    std::vector<double> getDrawdownCurve() const override;

    // Optimization
    void optimizeParameters(
        const std::map<std::string, std::pair<double, double>>& parameterRanges,
        size_t populationSize,
        size_t generations,
        const std::string& optimizationMetric) override;

    // Analysis
    void analyzeTrades() override;
    void generateReport(const std::string& outputPath) override;
    void visualizeResults() override;

    // Replay
    void startReplay() override;
    void stopReplay() override;
    void setReplaySpeed(double speed) override;
    void jumpToTime(std::chrono::system_clock::time_point time) override;

    // Events
    void subscribeToTradeUpdates(
        std::function<void(const Trade&)> callback) override;
    void subscribeToPerformanceUpdates(
        std::function<void(const PerformanceMetrics&)> callback) override;
    void subscribeToMarketDataUpdates(
        std::function<void(const core::MarketData&)> callback) override;

private:
    struct MarketSimulator {
        std::random_device rd;
        std::mt19937 gen;
        std::normal_distribution<double> priceDist;
        std::normal_distribution<double> volumeDist;
        std::exponential_distribution<double> latencyDist;

        MarketSimulator(double slippage, std::chrono::milliseconds latency)
            : gen(rd())
            , priceDist(0.0, slippage)
            , volumeDist(0.0, 0.1)
            , latencyDist(1.0 / latency.count())
        {}

        double applySlippage(double price, core::OrderSide side) {
            double slippage = priceDist(gen);
            return side == core::OrderSide::BUY ? price * (1.0 + slippage) : price * (1.0 - slippage);
        }

        std::chrono::milliseconds getLatency() {
            return std::chrono::milliseconds(static_cast<long>(latencyDist(gen)));
        }
    };

    struct GeneticOptimizer {
        struct Individual {
            std::map<std::string, double> parameters;
            double fitness;
        };

        std::vector<Individual> population;
        std::map<std::string, std::pair<double, double>> parameterRanges;
        size_t populationSize;
        size_t generations;
        std::string optimizationMetric;
        double mutationRate;
        double crossoverRate;

        void initialize();
        void evolve();
        Individual crossover(const Individual& parent1, const Individual& parent2);
        void mutate(Individual& individual);
        double evaluateFitness(const Individual& individual);
        Individual tournamentSelection();
    };

    // Configuration
    BacktestConfig config_;
    std::shared_ptr<strategies::IStrategy> strategy_;
    std::vector<core::MarketData> marketData_;
    MarketSimulator marketSimulator_;
    GeneticOptimizer optimizer_;

    // State
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    std::atomic<double> replaySpeed_;
    std::thread backtestThread_;
    std::thread replayThread_;
    std::mutex stateMutex_;
    std::condition_variable stateCondition_;

    // Results
    PerformanceMetrics performanceMetrics_;
    std::vector<Trade> trades_;
    std::vector<double> equityCurve_;
    std::vector<double> drawdownCurve_;

    // Event handlers
    std::vector<std::function<void(const Trade&)>> tradeCallbacks_;
    std::vector<std::function<void(const PerformanceMetrics&)>> performanceCallbacks_;
    std::vector<std::function<void(const core::MarketData&)>> marketDataCallbacks_;

    // Processing methods
    void processMarketData();
    void executeOrder(const core::Order& order);
    void updatePerformanceMetrics(const Trade& trade);
    void calculateMetrics();
    void generateVisualizations();
    void exportResults(const std::string& outputPath);
    void runOptimization();
    void runReplay();
    void applyMarketImpact(core::Order& order);
    void simulateOrderExecution(core::Order& order);
    void handleMarketDataUpdate(const core::MarketData& data);
    void handleTradeUpdate(const Trade& trade);
    void handlePerformanceUpdate(const PerformanceMetrics& metrics);
};

} // namespace backtesting
} // namespace tradingbot 