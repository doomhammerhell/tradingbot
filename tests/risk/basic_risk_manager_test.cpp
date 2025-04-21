#include <gtest/gtest.h>
#include "risk/BasicRiskManager.hpp"
#include "core/IRiskManager.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tradingbot {
namespace risk {

class BasicRiskManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<BasicRiskManager>();
        
        // Configuração padrão para os testes
        json config = {
            {"max_position_size", 0.1},
            {"max_risk_per_trade", 0.02},
            {"max_daily_loss", 0.05},
            {"stop_loss_percentage", 0.02}
        };
        
        manager_->initialize(config.dump());
        
        // Portfólio padrão para os testes
        portfolio_.totalBalance = 10000.0;
        portfolio_.availableBalance = 10000.0;
    }
    
    std::unique_ptr<BasicRiskManager> manager_;
    core::Portfolio portfolio_;
};

TEST_F(BasicRiskManagerTest, ShouldAllowValidOrder) {
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    order.quantity = 0.1;  // 5000 USDT, 50% do portfólio
    
    bool result = manager_->shouldExecute(order, portfolio_);
    EXPECT_TRUE(result);
}

TEST_F(BasicRiskManagerTest, ShouldRejectOrderExceedingPositionSize) {
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    order.quantity = 0.3;  // 15000 USDT, 150% do portfólio
    
    bool result = manager_->shouldExecute(order, portfolio_);
    EXPECT_FALSE(result);
    
    auto metrics = json::parse(manager_->getMetrics());
    EXPECT_EQ(metrics["rejected_trades"], 1);
}

TEST_F(BasicRiskManagerTest, ShouldRejectOrderExceedingDailyLoss) {
    // Simula uma perda diária que excede o limite
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::SELL;
    order.price = 45000.0;
    order.quantity = 0.1;
    
    // Executa a ordem para atualizar o PnL diário
    manager_->updateAfterExecution(order, true);
    
    // Tenta executar outra ordem
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    
    bool result = manager_->shouldExecute(order, portfolio_);
    EXPECT_FALSE(result);
}

TEST_F(BasicRiskManagerTest, ShouldCalculateCorrectPositionSize) {
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    
    double positionSize = manager_->calculatePositionSize(order, portfolio_);
    
    // O tamanho máximo da posição deve ser limitado pelo risco por trade
    // maxRiskAmount = 10000 * 0.02 = 200 USDT
    // stopLossAmount = 50000 * 0.02 = 1000 USDT
    // positionSize = 200 / 1000 = 0.2 BTC
    EXPECT_NEAR(positionSize, 0.2, 0.001);
}

TEST_F(BasicRiskManagerTest, ShouldUpdateMetricsAfterExecution) {
    core::Order order;
    order.symbol = "BTCUSDT";
    order.type = core::Order::Type::MARKET;
    order.side = core::Order::Side::BUY;
    order.price = 50000.0;
    order.quantity = 0.1;
    
    // Executa a ordem
    manager_->updateAfterExecution(order, true);
    
    auto metrics = json::parse(manager_->getMetrics());
    EXPECT_EQ(metrics["total_trades"], 1);
    EXPECT_EQ(metrics["active_positions"], 1);
    
    // Executa uma ordem de venda
    order.side = core::Order::Side::SELL;
    order.price = 55000.0;
    
    manager_->updateAfterExecution(order, true);
    
    metrics = json::parse(manager_->getMetrics());
    EXPECT_EQ(metrics["total_trades"], 2);
    EXPECT_EQ(metrics["active_positions"], 0);
    EXPECT_NEAR(metrics["daily_pnl"].get<double>(), 500.0, 0.001);  // (55000 - 50000) * 0.1
}

} // namespace risk
} // namespace tradingbot 