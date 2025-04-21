#include "../../include/strategies/MACDStrategy.hpp"

namespace tradingbot {
namespace strategies {

MACDStrategy::MACDStrategy()
    : lastPrice_(0.0)
    , currentPosition_(0.0)
    , totalPnL_(0.0)
    , winningTrades_(0)
    , losingTrades_(0)
{
    macd_ = std::make_shared<technical::indicators::MACD>();
}

void MACDStrategy::initialize(const StrategyParameters& params) {
    params_ = params;
    macd_->initialize(params.indicators[0]->getDataFeed());
    reset();
}

void MACDStrategy::onMarketData(const core::MarketData& data) {
    lastPrice_ = data.price;
    macd_->calculate();
}

void MACDStrategy::onOrderUpdate(const core::Order& order) {
    if (order.status == core::OrderStatus::FILLED) {
        updatePerformanceMetrics(order);
        currentPosition_ += (order.side == core::OrderSide::BUY ? 1 : -1) * order.executedQuantity;
    }
}

std::vector<core::Order> MACDStrategy::generateSignals() {
    std::vector<core::Order> signals;
    
    if (!macd_ || lastPrice_ == 0.0) {
        return signals;
    }

    double currentMACD = macd_->getCurrentValue();
    double currentSignal = macd_->getCurrentSignal();
    double currentHistogram = macd_->getCurrentHistogram();

    // Generate buy signal when MACD crosses above signal line
    if (currentMACD > currentSignal && currentHistogram > 0 && currentPosition_ <= 0) {
        core::Order order;
        order.symbol = params_.symbol;
        order.type = core::OrderType::MARKET;
        order.side = core::OrderSide::BUY;
        order.quantity = 1.0; // Fixed quantity for simplicity
        signals.push_back(order);
    }
    // Generate sell signal when MACD crosses below signal line
    else if (currentMACD < currentSignal && currentHistogram < 0 && currentPosition_ >= 0) {
        core::Order order;
        order.symbol = params_.symbol;
        order.type = core::OrderType::MARKET;
        order.side = core::OrderSide::SELL;
        order.quantity = 1.0; // Fixed quantity for simplicity
        signals.push_back(order);
    }

    return signals;
}

std::string MACDStrategy::getName() const {
    return "MACD Strategy";
}

StrategyParameters MACDStrategy::getParameters() const {
    return params_;
}

std::map<std::string, double> MACDStrategy::getPerformanceMetrics() const {
    return {
        {"total_pnl", totalPnL_},
        {"winning_trades", static_cast<double>(winningTrades_)},
        {"losing_trades", static_cast<double>(losingTrades_)},
        {"win_rate", winningTrades_ + losingTrades_ > 0 ? 
            static_cast<double>(winningTrades_) / (winningTrades_ + losingTrades_) : 0.0}
    };
}

void MACDStrategy::reset() {
    lastPrice_ = 0.0;
    currentPosition_ = 0.0;
    totalPnL_ = 0.0;
    winningTrades_ = 0;
    losingTrades_ = 0;
}

void MACDStrategy::updatePerformanceMetrics(const core::Order& order) {
    double pnl = (order.side == core::OrderSide::SELL ? 1 : -1) * 
                 order.executedQuantity * 
                 (order.averagePrice - lastPrice_);
    
    totalPnL_ += pnl;
    
    if (pnl > 0) {
        winningTrades_++;
    } else if (pnl < 0) {
        losingTrades_++;
    }
}

} // namespace strategies
} // namespace tradingbot 