#include <gtest/gtest.h>
#include "execution/MockExecutionEngine.hpp"
#include "core/IExecutionEngine.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tradingbot {
namespace execution {

class MockExecutionEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_unique<MockExecutionEngine>();
        engine_->initialize("{}");
    }
    
    std::unique_ptr<MockExecutionEngine> engine_;
};

TEST_F(MockExecutionEngineTest, ShouldExecuteOrderSuccessfully) {
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    order.quantity = 0.1;
    
    bool result = engine_->execute(order);
    EXPECT_TRUE(result);
    
    auto metrics = json::parse(engine_->getMetrics());
    EXPECT_EQ(metrics["total_orders"], 1);
    EXPECT_EQ(metrics["successful_orders"], 1);
    EXPECT_EQ(metrics["failed_orders"], 0);
}

TEST_F(MockExecutionEngineTest, ShouldFailOrderExecution) {
    engine_->setNextExecutionResult(false);
    
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    order.quantity = 0.1;
    
    bool result = engine_->execute(order);
    EXPECT_FALSE(result);
    
    auto metrics = json::parse(engine_->getMetrics());
    EXPECT_EQ(metrics["total_orders"], 1);
    EXPECT_EQ(metrics["successful_orders"], 0);
    EXPECT_EQ(metrics["failed_orders"], 1);
}

TEST_F(MockExecutionEngineTest, ShouldCancelOrder) {
    // Primeiro executa uma ordem
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    order.quantity = 0.1;
    
    engine_->execute(order);
    
    // Obtém o ID da ordem
    auto metrics = json::parse(engine_->getMetrics());
    std::string orderId = metrics["active_orders"].get<std::string>();
    
    // Cancela a ordem
    bool result = engine_->cancelOrder(orderId);
    EXPECT_TRUE(result);
    
    // Verifica o status
    std::string status = engine_->getOrderStatus(orderId);
    EXPECT_EQ(status, "CANCELLED");
    
    metrics = json::parse(engine_->getMetrics());
    EXPECT_EQ(metrics["cancelled_orders"], 1);
}

TEST_F(MockExecutionEngineTest, ShouldNotCancelExecutedOrder) {
    // Executa uma ordem com sucesso
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    order.quantity = 0.1;
    
    engine_->execute(order);
    
    // Tenta cancelar a ordem
    auto metrics = json::parse(engine_->getMetrics());
    std::string orderId = metrics["active_orders"].get<std::string>();
    
    bool result = engine_->cancelOrder(orderId);
    EXPECT_FALSE(result);
}

TEST_F(MockExecutionEngineTest, ShouldClearHistory) {
    // Executa algumas ordens
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    order.quantity = 0.1;
    
    engine_->execute(order);
    engine_->setNextExecutionResult(false);
    engine_->execute(order);
    
    // Limpa o histórico
    engine_->clearHistory();
    
    // Verifica se as métricas foram resetadas
    auto metrics = json::parse(engine_->getMetrics());
    EXPECT_EQ(metrics["total_orders"], 0);
    EXPECT_EQ(metrics["successful_orders"], 0);
    EXPECT_EQ(metrics["failed_orders"], 0);
    EXPECT_EQ(metrics["cancelled_orders"], 0);
    EXPECT_EQ(metrics["active_orders"], 0);
}

} // namespace execution
} // namespace tradingbot 