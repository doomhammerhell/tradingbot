#include "core/TradingEngine.hpp"
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

namespace tradingbot {
namespace core {

TradingEngine::TradingEngine(
    std::unique_ptr<strategies::IStrategy> strategy,
    std::unique_ptr<execution::IExecutionEngine> executionEngine,
    std::unique_ptr<risk::IRiskManager> riskManager,
    std::unique_ptr<data::IDataFeed> dataFeed
) : strategy_(std::move(strategy)),
    executionEngine_(std::move(executionEngine)),
    riskManager_(std::move(riskManager)),
    dataFeed_(std::move(dataFeed)),
    isRunning_(false),
    totalTrades_(0),
    successfulTrades_(0),
    failedTrades_(0),
    totalPnL_(0.0) {
    
    // Initialize portfolio
    portfolio_.totalBalance = 0.0;
    portfolio_.availableBalance = 0.0;
    portfolio_.totalPnL = 0.0;
}

void TradingEngine::initialize(const std::string& config) {
    try {
        auto json = nlohmann::json::parse(config);
        
        // Parse configuration
        std::string mode = json["mode"].get<std::string>();
        runMode_ = (mode == "live") ? RunMode::LIVE : RunMode::BACKTEST;
        
        symbol_ = json["symbol"].get<std::string>();
        timeframe_ = json["timeframe"].get<std::string>();
        initialBalance_ = json["initial_balance"].get<double>();
        
        // Initialize portfolio
        portfolio_.totalBalance = initialBalance_;
        portfolio_.availableBalance = initialBalance_;
        
        // Initialize components
        strategy_->initialize(json["strategy_config"].dump());
        executionEngine_->initialize(json["execution_config"].dump());
        riskManager_->initialize(json["risk_config"].dump());
        dataFeed_->initialize(json["data_feed_config"].dump());
        
        // Set up data feed callback
        dataFeed_->setMarketDataCallback([this](const std::string& data) {
            processMarketData(data);
        });
        
        Logger::getInstance().info("TradingEngine initialized with config: {}", config);
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to initialize TradingEngine: {}", e.what());
        throw;
    }
}

void TradingEngine::start() {
    if (isRunning_) {
        Logger::getInstance().warn("TradingEngine is already running");
        return;
    }
    
    isRunning_ = true;
    startTime_ = std::chrono::system_clock::now();
    
    // Connect to data feed
    if (!dataFeed_->connect()) {
        Logger::getInstance().error("Failed to connect to data feed");
        isRunning_ = false;
        return;
    }
    
    // Subscribe to market data
    if (!dataFeed_->subscribe(symbol_, timeframe_)) {
        Logger::getInstance().error("Failed to subscribe to {} {}", symbol_, timeframe_);
        isRunning_ = false;
        return;
    }
    
    Logger::getInstance().info("TradingEngine started in {} mode", 
        (runMode_ == RunMode::LIVE) ? "live" : "backtest");
}

void TradingEngine::stop() {
    if (!isRunning_) {
        return;
    }
    
    isRunning_ = false;
    dataFeed_->disconnect();
    
    Logger::getInstance().info("TradingEngine stopped");
}

void TradingEngine::processMarketData(const std::string& marketData) {
    if (!isRunning_) {
        return;
    }
    
    try {
        // Parse market data
        auto json = nlohmann::json::parse(marketData);
        strategies::MarketData data;
        
        data.symbol = symbol_;
        data.timestamp = std::chrono::system_clock::from_time_t(json["timestamp"].get<time_t>());
        data.open = json["open"].get<double>();
        data.high = json["high"].get<double>();
        data.low = json["low"].get<double>();
        data.close = json["close"].get<double>();
        data.volume = json["volume"].get<double>();
        
        // Get trading decision from strategy
        auto decision = strategy_->analyze({data});
        
        // Execute decision if not HOLD
        if (decision.type != strategies::DecisionType::HOLD) {
            executeDecision(decision);
        }
    } catch (const std::exception& e) {
        Logger::getInstance().error("Error processing market data: {}", e.what());
    }
}

void TradingEngine::executeDecision(const strategies::Decision& decision) {
    try {
        // Create order from decision
        execution::Order order;
        order.symbol = symbol_;
        order.type = execution::OrderType::MARKET;
        order.side = (decision.type == strategies::DecisionType::BUY) ? 
            execution::OrderSide::BUY : execution::OrderSide::SELL;
        order.quantity = decision.quantity;
        order.price = decision.price;
        order.timestamp = std::chrono::system_clock::now();
        
        // Check risk management rules
        if (!riskManager_->shouldExecute(order, portfolio_)) {
            Logger::getInstance().info("Order rejected by risk manager: {}", decision.reason);
            return;
        }
        
        // Calculate position size
        order.quantity = riskManager_->calculatePositionSize(order, portfolio_);
        
        // Execute order
        std::string orderId = executionEngine_->execute(order);
        totalTrades_++;
        
        // Update strategy and risk manager
        strategy_->updateOrderStatus(orderId, true);
        riskManager_->updateAfterExecution(order, true);
        successfulTrades_++;
        
        // Update portfolio
        updatePortfolio(order, true);
        
        Logger::getInstance().info("Order executed: {} {} {} @ {}", 
            order.side == execution::OrderSide::BUY ? "BUY" : "SELL",
            order.quantity, order.symbol, order.price);
    } catch (const std::exception& e) {
        Logger::getInstance().error("Error executing order: {}", e.what());
        failedTrades_++;
    }
}

void TradingEngine::updatePortfolio(const execution::Order& order, bool success) {
    if (!success) {
        return;
    }
    
    // Update position
    auto& position = portfolio_.positions[order.symbol];
    if (order.side == execution::OrderSide::BUY) {
        position.quantity += order.quantity;
        position.averagePrice = ((position.averagePrice * (position.quantity - order.quantity)) + 
                               (order.price * order.quantity)) / position.quantity;
    } else {
        position.quantity -= order.quantity;
        double pnl = (order.price - position.averagePrice) * order.quantity;
        position.realizedPnL += pnl;
        totalPnL_ += pnl;
    }
    
    // Update balances
    portfolio_.totalBalance = initialBalance_ + totalPnL_;
    portfolio_.availableBalance = portfolio_.totalBalance;
    
    // Update asset allocation
    for (auto& [symbol, pos] : portfolio_.positions) {
        portfolio_.assetAllocation[symbol] = 
            (pos.quantity * pos.averagePrice) / portfolio_.totalBalance;
    }
}

std::string TradingEngine::getMetrics() const {
    nlohmann::json metrics;
    
    metrics["run_mode"] = (runMode_ == RunMode::LIVE) ? "live" : "backtest";
    metrics["symbol"] = symbol_;
    metrics["timeframe"] = timeframe_;
    metrics["total_trades"] = totalTrades_;
    metrics["successful_trades"] = successfulTrades_;
    metrics["failed_trades"] = failedTrades_;
    metrics["total_pnl"] = totalPnL_;
    metrics["portfolio"] = {
        {"total_balance", portfolio_.totalBalance},
        {"available_balance", portfolio_.availableBalance},
        {"total_pnl", portfolio_.totalPnL}
    };
    
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_);
    metrics["uptime_seconds"] = duration.count();
    
    return metrics.dump(4);
}

} // namespace core
} // namespace tradingbot 