#include "execution/MockExecutionEngine.hpp"
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>

using json = nlohmann::json;

namespace tradingbot {
namespace execution {

MockExecutionEngine::MockExecutionEngine()
    : nextExecutionResult_(true),
      totalOrders_(0),
      successfulOrders_(0),
      failedOrders_(0),
      cancelledOrders_(0) {
    
    auto& logger = utils::Logger::getInstance();
    logger.info("MockExecutionEngine created");
}

void MockExecutionEngine::initialize(const std::string& config) {
    auto& logger = utils::Logger::getInstance();
    logger.info("Initializing MockExecutionEngine...");
    
    try {
        json configJson = json::parse(config);
        // Configurações específicas do mock podem ser adicionadas aqui
        logger.info("MockExecutionEngine initialized");
    } catch (const json::exception& e) {
        logger.error("Failed to parse config: {}", e.what());
        throw std::runtime_error("Invalid configuration");
    }
}

bool MockExecutionEngine::execute(const core::Order& order) {
    auto& logger = utils::Logger::getInstance();
    std::lock_guard<std::mutex> lock(mutex_);
    
    totalOrders_++;
    
    // Gera um ID de ordem se não foi fornecido
    std::string orderId = nextOrderId_.empty() ? 
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) : 
        nextOrderId_;
    
    // Registra a ordem no histórico
    OrderStatus status;
    status.order = order;
    status.executed = nextExecutionResult_;
    status.cancelled = false;
    status.status = nextExecutionResult_ ? "FILLED" : "REJECTED";
    
    orderHistory_[orderId] = status;
    
    if (nextExecutionResult_) {
        successfulOrders_++;
        logger.info("Order executed successfully: {}", orderId);
    } else {
        failedOrders_++;
        logger.error("Order execution failed: {}", orderId);
    }
    
    // Reseta o próximo resultado
    nextExecutionResult_ = true;
    nextOrderId_ = "";
    
    return status.executed;
}

bool MockExecutionEngine::cancelOrder(const std::string& orderId) {
    auto& logger = utils::Logger::getInstance();
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orderHistory_.find(orderId);
    if (it == orderHistory_.end()) {
        logger.warn("Order not found: {}", orderId);
        return false;
    }
    
    if (it->second.executed) {
        logger.warn("Cannot cancel executed order: {}", orderId);
        return false;
    }
    
    it->second.cancelled = true;
    it->second.status = "CANCELLED";
    cancelledOrders_++;
    
    logger.info("Order cancelled: {}", orderId);
    return true;
}

std::string MockExecutionEngine::getOrderStatus(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orderHistory_.find(orderId);
    if (it == orderHistory_.end()) {
        return "NOT_FOUND";
    }
    
    return it->second.status;
}

std::string MockExecutionEngine::getMetrics() const {
    json metrics;
    
    metrics["total_orders"] = totalOrders_;
    metrics["successful_orders"] = successfulOrders_;
    metrics["failed_orders"] = failedOrders_;
    metrics["cancelled_orders"] = cancelledOrders_;
    metrics["active_orders"] = orderHistory_.size();
    
    return metrics.dump(4);
}

void MockExecutionEngine::setNextExecutionResult(bool result) {
    nextExecutionResult_ = result;
}

void MockExecutionEngine::setNextOrderId(const std::string& orderId) {
    nextOrderId_ = orderId;
}

void MockExecutionEngine::clearHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    orderHistory_.clear();
    totalOrders_ = 0;
    successfulOrders_ = 0;
    failedOrders_ = 0;
    cancelledOrders_ = 0;
}

} // namespace execution
} // namespace tradingbot 