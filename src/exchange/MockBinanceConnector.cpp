#include "exchange/MockBinanceConnector.hpp"
#include "utils/Logger.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace tradingbot {
namespace exchange {

MockBinanceConnector::MockBinanceConnector()
    : connected_(false)
    , gen_(rd_()) {
    initializeMockData();
}

void MockBinanceConnector::initialize(const std::string& apiKey, const std::string& apiSecret) {
    std::lock_guard<std::mutex> lock(mutex_);
    apiKey_ = apiKey;
    apiSecret_ = apiSecret;
    connected_ = true;
    LOG_INFO("MockBinanceConnector initialized with API key: " + apiKey);
}

Order MockBinanceConnector::placeOrder(const Order& order) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_) {
        throw std::runtime_error("Not connected to exchange");
    }

    Order newOrder = order;
    newOrder.orderId = generateOrderId();
    newOrder.timestamp = std::chrono::system_clock::now();

    // Simulate order execution
    if (order.type == Order::Type::MARKET) {
        // For market orders, execute immediately at current price
        double executionPrice = getCurrentPrice(order.symbol);
        double totalCost = order.quantity * executionPrice;

        // Update balances
        if (order.side == Order::Side::BUY) {
            balances_["USDT"].free -= totalCost;
            balances_["USDT"].locked += totalCost;
            balances_[order.symbol.substr(0, order.symbol.find("USDT"))].free += order.quantity;
        } else {
            balances_[order.symbol.substr(0, order.symbol.find("USDT"))].free -= order.quantity;
            balances_[order.symbol.substr(0, order.symbol.find("USDT"))].locked += order.quantity;
            balances_["USDT"].free += totalCost;
        }
    } else {
        // For limit orders, add to open orders
        openOrders_[newOrder.orderId] = newOrder;
    }

    return newOrder;
}

bool MockBinanceConnector::cancelOrder(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = openOrders_.find(orderId);
    if (it != openOrders_.end()) {
        openOrders_.erase(it);
        return true;
    }
    return false;
}

std::vector<Order> MockBinanceConnector::getOpenOrders() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Order> orders;
    for (const auto& pair : openOrders_) {
        orders.push_back(pair.second);
    }
    return orders;
}

Order MockBinanceConnector::getOrderStatus(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = openOrders_.find(orderId);
    if (it != openOrders_.end()) {
        return it->second;
    }
    throw std::runtime_error("Order not found: " + orderId);
}

std::vector<Balance> MockBinanceConnector::getBalances() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Balance> balances;
    for (const auto& pair : balances_) {
        balances.push_back(pair.second);
    }
    return balances;
}

Balance MockBinanceConnector::getBalance(const std::string& asset) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = balances_.find(asset);
    if (it != balances_.end()) {
        return it->second;
    }
    throw std::runtime_error("Asset not found: " + asset);
}

double MockBinanceConnector::getCurrentPrice(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = currentPrices_.find(symbol);
    if (it != currentPrices_.end()) {
        return it->second;
    }
    throw std::runtime_error("Symbol not found: " + symbol);
}

std::vector<std::string> MockBinanceConnector::getAvailableSymbols() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> symbols;
    for (const auto& pair : currentPrices_) {
        symbols.push_back(pair.first);
    }
    return symbols;
}

std::string MockBinanceConnector::getExchangeName() const {
    return "MockBinance";
}

bool MockBinanceConnector::isConnected() const {
    return connected_;
}

void MockBinanceConnector::initializeMockData() {
    // Initialize with some mock balances
    balances_["BTC"] = Balance{"BTC", 1.0, 0.0, 1.0};
    balances_["ETH"] = Balance{"ETH", 10.0, 0.0, 10.0};
    balances_["USDT"] = Balance{"USDT", 10000.0, 0.0, 10000.0};

    // Initialize with some mock prices
    currentPrices_["BTCUSDT"] = 50000.0;
    currentPrices_["ETHUSDT"] = 3000.0;
}

std::string MockBinanceConnector::generateOrderId() {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << gen_();
    return ss.str();
}

void MockBinanceConnector::updatePrices() {
    // Simulate price movements
    std::normal_distribution<double> distribution(0.0, 0.01);
    for (auto& pair : currentPrices_) {
        double change = distribution(gen_);
        pair.second *= (1.0 + change);
    }
}

} // namespace exchange
} // namespace tradingbot 