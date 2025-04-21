#pragma once

#include "core/IExecutionEngine.hpp"
#include <map>
#include <mutex>
#include <atomic>

namespace tradingbot {
namespace execution {

class MockExecutionEngine : public core::IExecutionEngine {
public:
    MockExecutionEngine();
    
    void initialize(const std::string& config) override;
    bool execute(const core::Order& order) override;
    bool cancelOrder(const std::string& orderId) override;
    std::string getOrderStatus(const std::string& orderId) override;
    std::string getMetrics() const override;
    
    // Métodos específicos para testes
    void setNextExecutionResult(bool result);
    void setNextOrderId(const std::string& orderId);
    void clearHistory();
    
private:
    struct OrderStatus {
        core::Order order;
        bool executed;
        bool cancelled;
        std::string status;
    };
    
    std::map<std::string, OrderStatus> orderHistory_;
    std::mutex mutex_;
    std::atomic<bool> nextExecutionResult_;
    std::string nextOrderId_;
    
    // Métricas
    std::atomic<int> totalOrders_;
    std::atomic<int> successfulOrders_;
    std::atomic<int> failedOrders_;
    std::atomic<int> cancelledOrders_;
};

} // namespace execution
} // namespace tradingbot 