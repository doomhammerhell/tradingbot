#include "../../include/core/TradingBot.hpp"
#include <fstream>
#include <json/json.h>
#include <chrono>
#include <thread>

namespace tradingbot {
namespace core {

TradingBot::TradingBot()
    : isRunning_(false)
    , currentPosition_(0.0)
    , currentBalance_(0.0)
{}

TradingBot::~TradingBot() {
    stop();
}

void TradingBot::initialize(const std::string& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Failed to open config file: " + configPath);
    }

    Json::Value root;
    configFile >> root;

    // Initialize data provider
    dataProvider_ = std::make_shared<data::BinanceDataProvider>();
    dataProvider_->initialize(root["data_provider_config"].asString());

    // Initialize order executor
    orderExecutor_ = std::make_shared<execution::BinanceOrderExecutor>();
    orderExecutor_->initialize(root["order_executor_config"].asString());

    // Initialize risk manager
    risk::RiskParameters riskParams;
    riskParams.maxPositionSize = root["risk"]["max_position_size"].asDouble();
    riskParams.maxLeverage = root["risk"]["max_leverage"].asDouble();
    riskParams.maxDrawdown = root["risk"]["max_drawdown"].asDouble();
    riskParams.maxDailyLoss = root["risk"]["max_daily_loss"].asDouble();
    riskParams.maxOrderSize = root["risk"]["max_order_size"].asDouble();
    riskParams.stopLossPercentage = root["risk"]["stop_loss_percentage"].asDouble();
    riskParams.takeProfitPercentage = root["risk"]["take_profit_percentage"].asDouble();
    riskParams.riskPerTrade = root["risk"]["risk_per_trade"].asDouble();

    riskManager_ = std::make_shared<risk::BasicRiskManager>();
    riskManager_->initialize(riskParams);

    // Initialize strategy
    strategies::StrategyParameters strategyParams;
    strategyParams.symbol = root["strategy"]["symbol"].asString();
    strategyParams.timeframe = root["strategy"]["timeframe"].asString();
    strategyParams.initialCapital = root["strategy"]["initial_capital"].asDouble();

    strategy_ = std::make_shared<strategies::MACDStrategy>();
    strategy_->initialize(strategyParams);

    // Connect to data provider and order executor
    dataProvider_->connect();
    orderExecutor_->connect();

    // Subscribe to market data updates
    dataProvider_->subscribe(strategyParams.symbol,
        [this](const MarketData& data) {
            processMarketData(data);
        });

    // Subscribe to order updates
    orderExecutor_->subscribeOrderUpdates(
        [this](const Order& order) {
            processOrderUpdate(order);
        });
}

void TradingBot::start() {
    if (isRunning_) {
        return;
    }

    isRunning_ = true;
    tradingThread_ = std::thread(&TradingBot::tradingLoop, this);
}

void TradingBot::stop() {
    if (!isRunning_) {
        return;
    }

    isRunning_ = false;
    if (tradingThread_.joinable()) {
        tradingThread_.join();
    }

    dataProvider_->disconnect();
    orderExecutor_->disconnect();
}

MarketData TradingBot::getCurrentMarketData() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return currentMarketData_;
}

double TradingBot::getCurrentPosition() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return currentPosition_;
}

double TradingBot::getCurrentBalance() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return currentBalance_;
}

std::vector<Order> TradingBot::getCurrentOrders() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return currentOrders_;
}

std::string TradingBot::getStatus() const {
    return isRunning_ ? "Running" : "Stopped";
}

void TradingBot::tradingLoop() {
    while (isRunning_) {
        executeStrategy();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void TradingBot::processMarketData(const MarketData& data) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    currentMarketData_ = data;
    strategy_->onMarketData(data);
}

void TradingBot::processOrderUpdate(const Order& order) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Update current orders
    auto it = std::find_if(currentOrders_.begin(), currentOrders_.end(),
        [&order](const Order& o) { return o.id == order.id; });
    
    if (it != currentOrders_.end()) {
        if (order.status == OrderStatus::FILLED ||
            order.status == OrderStatus::CANCELED ||
            order.status == OrderStatus::REJECTED ||
            order.status == OrderStatus::EXPIRED) {
            currentOrders_.erase(it);
        } else {
            *it = order;
        }
    } else if (order.status != OrderStatus::FILLED &&
               order.status != OrderStatus::CANCELED &&
               order.status != OrderStatus::REJECTED &&
               order.status != OrderStatus::EXPIRED) {
        currentOrders_.push_back(order);
    }

    // Update position and balance
    if (order.status == OrderStatus::FILLED) {
        double positionChange = (order.side == OrderSide::BUY ? 1 : -1) * 
                              order.executedQuantity;
        currentPosition_ += positionChange;
        
        double pnl = -positionChange * (order.averagePrice - order.price);
        currentBalance_ += pnl;
    }

    strategy_->onOrderUpdate(order);
    riskManager_->updateMetrics(order);
}

void TradingBot::executeStrategy() {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Generate trading signals
    auto signals = strategy_->generateSignals();
    
    // Process each signal
    for (const auto& signal : signals) {
        // Validate order with risk manager
        if (!riskManager_->validateOrder(signal)) {
            continue;
        }
        
        // Place order
        try {
            auto order = orderExecutor_->placeOrder(signal);
            currentOrders_.push_back(order);
        } catch (const std::exception& e) {
            // Log error
        }
    }
}

} // namespace core
} // namespace tradingbot 