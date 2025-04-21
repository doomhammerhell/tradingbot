#include "../../include/backtesting/AdvancedBacktester.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <json/json.h>
#include <Eigen/Dense>
#include <matplotlibcpp.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_statistics.h>

namespace tradingbot {
namespace backtesting {

AdvancedBacktester::AdvancedBacktester()
    : running_(false)
    , paused_(false)
    , replaySpeed_(1.0)
{}

AdvancedBacktester::~AdvancedBacktester() {
    stop();
}

void AdvancedBacktester::initialize(const BacktestConfig& config) {
    config_ = config;
    marketSimulator_ = MarketSimulator(config.slippage, config.latency);
}

void AdvancedBacktester::setStrategy(std::shared_ptr<strategies::IStrategy> strategy) {
    strategy_ = strategy;
}

void AdvancedBacktester::setMarketData(const std::vector<core::MarketData>& data) {
    marketData_ = data;
}

void AdvancedBacktester::run() {
    if (running_) return;
    
    running_ = true;
    paused_ = false;
    backtestThread_ = std::thread(&AdvancedBacktester::processMarketData, this);
}

void AdvancedBacktester::pause() {
    if (!running_ || paused_) return;
    paused_ = true;
}

void AdvancedBacktester::resume() {
    if (!running_ || !paused_) return;
    paused_ = false;
    stateCondition_.notify_one();
}

void AdvancedBacktester::stop() {
    if (!running_) return;
    
    running_ = false;
    paused_ = false;
    stateCondition_.notify_one();
    
    if (backtestThread_.joinable()) {
        backtestThread_.join();
    }
    
    if (replayThread_.joinable()) {
        replayThread_.join();
    }
}

void AdvancedBacktester::processMarketData() {
    for (const auto& data : marketData_) {
        if (!running_) break;
        
        if (paused_) {
            std::unique_lock<std::mutex> lock(stateMutex_);
            stateCondition_.wait(lock, [this] { return !paused_ || !running_; });
            if (!running_) break;
        }
        
        handleMarketDataUpdate(data);
        
        if (strategy_) {
            auto signals = strategy_->analyze(data);
            for (const auto& signal : signals) {
                if (signal.type == core::SignalType::BUY || signal.type == core::SignalType::SELL) {
                    core::Order order;
                    order.symbol = data.symbol;
                    order.side = signal.type == core::SignalType::BUY ? core::OrderSide::BUY : core::OrderSide::SELL;
                    order.type = core::OrderType::MARKET;
                    order.quantity = signal.quantity;
                    order.price = data.price;
                    
                    executeOrder(order);
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    calculateMetrics();
    generateVisualizations();
}

void AdvancedBacktester::executeOrder(const core::Order& order) {
    core::Order executedOrder = order;
    applyMarketImpact(executedOrder);
    simulateOrderExecution(executedOrder);
    
    Trade trade;
    trade.symbol = executedOrder.symbol;
    trade.entryPrice = executedOrder.price;
    trade.exitPrice = executedOrder.price;
    trade.quantity = executedOrder.quantity;
    trade.side = executedOrder.side;
    trade.entryTime = std::chrono::system_clock::now();
    trade.exitTime = trade.entryTime;
    trade.profitLoss = (trade.exitPrice - trade.entryPrice) * trade.quantity * 
                      (trade.side == core::OrderSide::BUY ? 1.0 : -1.0);
    
    trades_.push_back(trade);
    updatePerformanceMetrics(trade);
    handleTradeUpdate(trade);
}

void AdvancedBacktester::applyMarketImpact(core::Order& order) {
    double impact = 0.001 * (order.quantity / marketData_.back().volume);
    order.price *= (1.0 + impact * (order.side == core::OrderSide::BUY ? 1.0 : -1.0));
}

void AdvancedBacktester::simulateOrderExecution(core::Order& order) {
    order.price = marketSimulator_.applySlippage(order.price, order.side);
    std::this_thread::sleep_for(marketSimulator_.getLatency());
}

void AdvancedBacktester::updatePerformanceMetrics(const Trade& trade) {
    performanceMetrics_.totalTrades++;
    performanceMetrics_.winningTrades += (trade.profitLoss > 0) ? 1 : 0;
    performanceMetrics_.totalProfitLoss += trade.profitLoss;
    performanceMetrics_.largestWin = std::max(performanceMetrics_.largestWin, trade.profitLoss);
    performanceMetrics_.largestLoss = std::min(performanceMetrics_.largestLoss, trade.profitLoss);
    
    double currentEquity = config_.initialCapital + performanceMetrics_.totalProfitLoss;
    equityCurve_.push_back(currentEquity);
    
    double peakEquity = *std::max_element(equityCurve_.begin(), equityCurve_.end());
    double currentDrawdown = (peakEquity - currentEquity) / peakEquity;
    drawdownCurve_.push_back(currentDrawdown);
    performanceMetrics_.maxDrawdown = std::max(performanceMetrics_.maxDrawdown, currentDrawdown);
    
    handlePerformanceUpdate(performanceMetrics_);
}

void AdvancedBacktester::calculateMetrics() {
    if (trades_.empty()) return;
    
    // Calculate returns
    std::vector<double> returns;
    for (size_t i = 1; i < equityCurve_.size(); ++i) {
        returns.push_back((equityCurve_[i] - equityCurve_[i-1]) / equityCurve_[i-1]);
    }
    
    // Calculate Sharpe ratio
    double meanReturn = gsl_stats_mean(returns.data(), 1, returns.size());
    double stdDev = gsl_stats_sd(returns.data(), 1, returns.size());
    performanceMetrics_.sharpeRatio = (stdDev > 0) ? meanReturn / stdDev * std::sqrt(252) : 0.0;
    
    // Calculate win rate
    performanceMetrics_.winRate = static_cast<double>(performanceMetrics_.winningTrades) / 
                                 performanceMetrics_.totalTrades;
    
    // Calculate profit factor
    double totalWins = 0.0;
    double totalLosses = 0.0;
    for (const auto& trade : trades_) {
        if (trade.profitLoss > 0) {
            totalWins += trade.profitLoss;
        } else {
            totalLosses += std::abs(trade.profitLoss);
        }
    }
    performanceMetrics_.profitFactor = (totalLosses > 0) ? totalWins / totalLosses : 0.0;
}

void AdvancedBacktester::generateVisualizations() {
    namespace plt = matplotlibcpp;
    
    // Plot equity curve
    plt::figure();
    plt::plot(equityCurve_);
    plt::title("Equity Curve");
    plt::xlabel("Time");
    plt::ylabel("Equity");
    plt::save("equity_curve.png");
    plt::close();
    
    // Plot drawdown curve
    plt::figure();
    plt::plot(drawdownCurve_);
    plt::title("Drawdown Curve");
    plt::xlabel("Time");
    plt::ylabel("Drawdown");
    plt::save("drawdown_curve.png");
    plt::close();
    
    // Plot trade distribution
    std::vector<double> profits;
    for (const auto& trade : trades_) {
        profits.push_back(trade.profitLoss);
    }
    
    plt::figure();
    plt::hist(profits, 50);
    plt::title("Trade Profit Distribution");
    plt::xlabel("Profit/Loss");
    plt::ylabel("Frequency");
    plt::save("trade_distribution.png");
    plt::close();
}

void AdvancedBacktester::exportResults(const std::string& outputPath) {
    Json::Value root;
    
    // Export performance metrics
    root["totalTrades"] = performanceMetrics_.totalTrades;
    root["winningTrades"] = performanceMetrics_.winningTrades;
    root["winRate"] = performanceMetrics_.winRate;
    root["totalProfitLoss"] = performanceMetrics_.totalProfitLoss;
    root["sharpeRatio"] = performanceMetrics_.sharpeRatio;
    root["maxDrawdown"] = performanceMetrics_.maxDrawdown;
    root["profitFactor"] = performanceMetrics_.profitFactor;
    root["largestWin"] = performanceMetrics_.largestWin;
    root["largestLoss"] = performanceMetrics_.largestLoss;
    
    // Export trades
    Json::Value tradesArray;
    for (const auto& trade : trades_) {
        Json::Value tradeObj;
        tradeObj["symbol"] = trade.symbol;
        tradeObj["entryPrice"] = trade.entryPrice;
        tradeObj["exitPrice"] = trade.exitPrice;
        tradeObj["quantity"] = trade.quantity;
        tradeObj["side"] = static_cast<int>(trade.side);
        tradeObj["profitLoss"] = trade.profitLoss;
        tradeObj["entryTime"] = std::chrono::system_clock::to_time_t(trade.entryTime);
        tradeObj["exitTime"] = std::chrono::system_clock::to_time_t(trade.exitTime);
        tradesArray.append(tradeObj);
    }
    root["trades"] = tradesArray;
    
    // Write to file
    std::ofstream file(outputPath);
    Json::StyledWriter writer;
    file << writer.write(root);
    file.close();
}

void AdvancedBacktester::optimizeParameters(
    const std::map<std::string, std::pair<double, double>>& parameterRanges,
    size_t populationSize,
    size_t generations,
    const std::string& optimizationMetric) {
    
    optimizer_.parameterRanges = parameterRanges;
    optimizer_.populationSize = populationSize;
    optimizer_.generations = generations;
    optimizer_.optimizationMetric = optimizationMetric;
    optimizer_.mutationRate = 0.1;
    optimizer_.crossoverRate = 0.8;
    
    optimizer_.initialize();
    runOptimization();
}

void AdvancedBacktester::runOptimization() {
    for (size_t gen = 0; gen < optimizer_.generations; ++gen) {
        optimizer_.evolve();
        
        // Update strategy with best parameters
        auto bestIndividual = *std::max_element(
            optimizer_.population.begin(),
            optimizer_.population.end(),
            [](const auto& a, const auto& b) { return a.fitness < b.fitness; }
        );
        
        if (strategy_) {
            strategy_->setParameters(bestIndividual.parameters);
        }
    }
}

void AdvancedBacktester::GeneticOptimizer::initialize() {
    population.resize(populationSize);
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (auto& individual : population) {
        for (const auto& [param, range] : parameterRanges) {
            std::uniform_real_distribution<double> dist(range.first, range.second);
            individual.parameters[param] = dist(gen);
        }
        individual.fitness = evaluateFitness(individual);
    }
}

void AdvancedBacktester::GeneticOptimizer::evolve() {
    std::vector<Individual> newPopulation;
    newPopulation.reserve(populationSize);
    
    // Elitism: keep best individual
    auto bestIndividual = *std::max_element(
        population.begin(),
        population.end(),
        [](const auto& a, const auto& b) { return a.fitness < b.fitness; }
    );
    newPopulation.push_back(bestIndividual);
    
    // Generate new population
    while (newPopulation.size() < populationSize) {
        if (std::uniform_real_distribution<double>(0, 1)(gen) < crossoverRate) {
            auto parent1 = tournamentSelection();
            auto parent2 = tournamentSelection();
            auto child = crossover(parent1, parent2);
            mutate(child);
            child.fitness = evaluateFitness(child);
            newPopulation.push_back(child);
        } else {
            auto individual = tournamentSelection();
            mutate(individual);
            individual.fitness = evaluateFitness(individual);
            newPopulation.push_back(individual);
        }
    }
    
    population = std::move(newPopulation);
}

AdvancedBacktester::GeneticOptimizer::Individual 
AdvancedBacktester::GeneticOptimizer::crossover(
    const Individual& parent1,
    const Individual& parent2) {
    
    Individual child;
    for (const auto& [param, _] : parameterRanges) {
        if (std::uniform_real_distribution<double>(0, 1)(gen) < 0.5) {
            child.parameters[param] = parent1.parameters.at(param);
        } else {
            child.parameters[param] = parent2.parameters.at(param);
        }
    }
    return child;
}

void AdvancedBacktester::GeneticOptimizer::mutate(Individual& individual) {
    for (auto& [param, value] : individual.parameters) {
        if (std::uniform_real_distribution<double>(0, 1)(gen) < mutationRate) {
            auto range = parameterRanges[param];
            std::uniform_real_distribution<double> dist(range.first, range.second);
            value = dist(gen);
        }
    }
}

double AdvancedBacktester::GeneticOptimizer::evaluateFitness(const Individual& individual) {
    if (!strategy_) return 0.0;
    
    strategy_->setParameters(individual.parameters);
    run();
    stop();
    
    if (optimizationMetric == "sharpe_ratio") {
        return performanceMetrics_.sharpeRatio;
    } else if (optimizationMetric == "profit_factor") {
        return performanceMetrics_.profitFactor;
    } else if (optimizationMetric == "win_rate") {
        return performanceMetrics_.winRate;
    } else {
        return performanceMetrics_.totalProfitLoss;
    }
}

AdvancedBacktester::GeneticOptimizer::Individual 
AdvancedBacktester::GeneticOptimizer::tournamentSelection() {
    const size_t tournamentSize = 3;
    std::vector<Individual> tournament;
    tournament.reserve(tournamentSize);
    
    for (size_t i = 0; i < tournamentSize; ++i) {
        size_t index = std::uniform_int_distribution<size_t>(0, populationSize - 1)(gen);
        tournament.push_back(population[index]);
    }
    
    return *std::max_element(
        tournament.begin(),
        tournament.end(),
        [](const auto& a, const auto& b) { return a.fitness < b.fitness; }
    );
}

void AdvancedBacktester::analyzeTrades() {
    // Implement trade analysis
}

void AdvancedBacktester::generateReport(const std::string& outputPath) {
    exportResults(outputPath);
}

void AdvancedBacktester::visualizeResults() {
    generateVisualizations();
}

void AdvancedBacktester::startReplay() {
    if (running_) return;
    
    running_ = true;
    paused_ = false;
    replayThread_ = std::thread(&AdvancedBacktester::runReplay, this);
}

void AdvancedBacktester::stopReplay() {
    stop();
}

void AdvancedBacktester::setReplaySpeed(double speed) {
    replaySpeed_ = speed;
}

void AdvancedBacktester::jumpToTime(std::chrono::system_clock::time_point time) {
    // Implement time jump
}

void AdvancedBacktester::runReplay() {
    auto startTime = marketData_.front().timestamp;
    auto endTime = marketData_.back().timestamp;
    auto currentTime = startTime;
    
    while (running_ && currentTime <= endTime) {
        if (paused_) {
            std::unique_lock<std::mutex> lock(stateMutex_);
            stateCondition_.wait(lock, [this] { return !paused_ || !running_; });
            if (!running_) break;
        }
        
        for (const auto& data : marketData_) {
            if (data.timestamp > currentTime) break;
            if (data.timestamp == currentTime) {
                handleMarketDataUpdate(data);
            }
        }
        
        currentTime += std::chrono::milliseconds(static_cast<long>(1000.0 / replaySpeed_));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void AdvancedBacktester::subscribeToTradeUpdates(
    std::function<void(const Trade&)> callback) {
    tradeCallbacks_.push_back(callback);
}

void AdvancedBacktester::subscribeToPerformanceUpdates(
    std::function<void(const PerformanceMetrics&)> callback) {
    performanceCallbacks_.push_back(callback);
}

void AdvancedBacktester::subscribeToMarketDataUpdates(
    std::function<void(const core::MarketData&)> callback) {
    marketDataCallbacks_.push_back(callback);
}

void AdvancedBacktester::handleMarketDataUpdate(const core::MarketData& data) {
    for (const auto& callback : marketDataCallbacks_) {
        callback(data);
    }
}

void AdvancedBacktester::handleTradeUpdate(const Trade& trade) {
    for (const auto& callback : tradeCallbacks_) {
        callback(trade);
    }
}

void AdvancedBacktester::handlePerformanceUpdate(const PerformanceMetrics& metrics) {
    for (const auto& callback : performanceCallbacks_) {
        callback(metrics);
    }
}

} // namespace backtesting
} // namespace tradingbot 