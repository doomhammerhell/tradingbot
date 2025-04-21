#include <gtest/gtest.h>
#include "core/TradingEngine.hpp"
#include "strategies/MovingAverageStrategy.hpp"
#include "execution/MockExecutionEngine.hpp"
#include "risk/BasicRiskManager.hpp"
#include "data/HistoricalDataFeed.hpp"
#include "utils/Logger.hpp"
#include <memory>
#include <chrono>

using namespace tradingbot;
using namespace tradingbot::core;
using namespace tradingbot::strategies;
using namespace tradingbot::execution;
using namespace tradingbot::risk;
using namespace tradingbot::data;

class TradingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create components
        strategy_ = std::make_unique<MovingAverageStrategy>();
        executionEngine_ = std::make_unique<MockExecutionEngine>();
        riskManager_ = std::make_unique<BasicRiskManager>();
        dataFeed_ = std::make_unique<HistoricalDataFeed>();
        
        // Create trading engine
        engine_ = std::make_unique<TradingEngine>(
            std::move(strategy_),
            std::move(executionEngine_),
            std::move(riskManager_),
            std::move(dataFeed_)
        );
        
        // Configure test data
        config_ = R"({
            "mode": "backtest",
            "symbol": "BTCUSDT",
            "timeframe": "1h",
            "initial_balance": 10000.0,
            "strategy_config": {
                "short_period": 10,
                "long_period": 20
            },
            "execution_config": {
                "simulate_latency": false
            },
            "risk_config": {
                "max_position_size": 1.0,
                "max_risk_per_trade": 0.02,
                "max_daily_loss": 0.05
            },
            "data_feed_config": {
                "data_path": "test_data",
                "format": "csv"
            }
        })";
    }
    
    void TearDown() override {
        engine_->stop();
    }
    
    std::unique_ptr<TradingEngine> engine_;
    std::unique_ptr<MovingAverageStrategy> strategy_;
    std::unique_ptr<MockExecutionEngine> executionEngine_;
    std::unique_ptr<BasicRiskManager> riskManager_;
    std::unique_ptr<HistoricalDataFeed> dataFeed_;
    std::string config_;
};

TEST_F(TradingEngineTest, Initialization) {
    EXPECT_NO_THROW(engine_->initialize(config_));
    
    // Verify metrics after initialization
    auto metrics = nlohmann::json::parse(engine_->getMetrics());
    EXPECT_EQ(metrics["run_mode"], "backtest");
    EXPECT_EQ(metrics["symbol"], "BTCUSDT");
    EXPECT_EQ(metrics["timeframe"], "1h");
    EXPECT_EQ(metrics["total_trades"], 0);
    EXPECT_EQ(metrics["successful_trades"], 0);
    EXPECT_EQ(metrics["failed_trades"], 0);
    EXPECT_DOUBLE_EQ(metrics["total_pnl"], 0.0);
    EXPECT_DOUBLE_EQ(metrics["portfolio"]["total_balance"], 10000.0);
    EXPECT_DOUBLE_EQ(metrics["portfolio"]["available_balance"], 10000.0);
}

TEST_F(TradingEngineTest, StartStop) {
    engine_->initialize(config_);
    
    // Start engine
    EXPECT_NO_THROW(engine_->start());
    
    // Verify engine is running
    auto metrics = nlohmann::json::parse(engine_->getMetrics());
    EXPECT_GT(metrics["uptime_seconds"], 0);
    
    // Stop engine
    EXPECT_NO_THROW(engine_->stop());
}

TEST_F(TradingEngineTest, MarketDataProcessing) {
    engine_->initialize(config_);
    engine_->start();
    
    // Create test market data
    std::string marketData = R"({
        "timestamp": 1640995200,
        "open": 50000.0,
        "high": 51000.0,
        "low": 49000.0,
        "close": 50500.0,
        "volume": 100.0
    })";
    
    // Process market data
    EXPECT_NO_THROW(engine_->processMarketData(marketData));
}

TEST_F(TradingEngineTest, OrderExecution) {
    engine_->initialize(config_);
    engine_->start();
    
    // Create test market data that should trigger a buy signal
    std::string marketData = R"({
        "timestamp": 1640995200,
        "open": 50000.0,
        "high": 51000.0,
        "low": 49000.0,
        "close": 50500.0,
        "volume": 100.0
    })";
    
    // Process market data
    engine_->processMarketData(marketData);
    
    // Verify metrics after execution
    auto metrics = nlohmann::json::parse(engine_->getMetrics());
    EXPECT_GT(metrics["total_trades"], 0);
}

TEST_F(TradingEngineTest, RiskManagement) {
    engine_->initialize(config_);
    engine_->start();
    
    // Create test market data that should trigger a buy signal
    std::string marketData = R"({
        "timestamp": 1640995200,
        "open": 50000.0,
        "high": 51000.0,
        "low": 49000.0,
        "close": 50500.0,
        "volume": 100.0
    })";
    
    // Process market data multiple times to test risk limits
    for (int i = 0; i < 10; ++i) {
        engine_->processMarketData(marketData);
    }
    
    // Verify metrics after multiple executions
    auto metrics = nlohmann::json::parse(engine_->getMetrics());
    EXPECT_GT(metrics["total_trades"], 0);
    EXPECT_LE(metrics["total_trades"], 10); // Should be limited by risk management
}

TEST_F(TradingEngineTest, PortfolioUpdates) {
    engine_->initialize(config_);
    engine_->start();
    
    // Create test market data for a buy signal
    std::string buyData = R"({
        "timestamp": 1640995200,
        "open": 50000.0,
        "high": 51000.0,
        "low": 49000.0,
        "close": 50500.0,
        "volume": 100.0
    })";
    
    // Process buy signal
    engine_->processMarketData(buyData);
    
    // Create test market data for a sell signal
    std::string sellData = R"({
        "timestamp": 1640998800,
        "open": 51000.0,
        "high": 52000.0,
        "low": 50000.0,
        "close": 51500.0,
        "volume": 100.0
    })";
    
    // Process sell signal
    engine_->processMarketData(sellData);
    
    // Verify portfolio updates
    auto metrics = nlohmann::json::parse(engine_->getMetrics());
    EXPECT_NE(metrics["portfolio"]["total_balance"], 10000.0); // Should have changed
    EXPECT_NE(metrics["total_pnl"], 0.0); // Should have PnL
}

TEST_F(TradingEngineTest, ErrorHandling) {
    engine_->initialize(config_);
    engine_->start();
    
    // Test invalid market data
    std::string invalidData = "invalid json";
    EXPECT_NO_THROW(engine_->processMarketData(invalidData));
    
    // Test empty market data
    std::string emptyData = "{}";
    EXPECT_NO_THROW(engine_->processMarketData(emptyData));
    
    // Verify metrics after error handling
    auto metrics = nlohmann::json::parse(engine_->getMetrics());
    EXPECT_EQ(metrics["failed_trades"], 0); // Should not count as failed trades
} 