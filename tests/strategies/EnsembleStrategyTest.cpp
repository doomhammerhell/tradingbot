#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "strategies/EnsembleStrategy.hpp"
#include "data/MarketData.hpp"
#include <memory>
#include <vector>

using namespace tradingbot;
using namespace tradingbot::strategies;

// Mock strategy for testing
class MockStrategy : public IStrategy {
public:
    MockStrategy(SignalType signalType) : signalType_(signalType) {}
    
    void initialize() override {}
    void update(const MarketData&) override {}
    Signal generateSignal() override {
        return Signal{signalType_, 1.0};
    }
    void reset() override {}
    nlohmann::json getStatus() const override {
        return nlohmann::json{{"type", "mock"}};
    }
    
private:
    SignalType signalType_;
};

TEST_CASE("EnsembleStrategy initialization", "[strategies]") {
    nlohmann::json config = {
        {"position_size", 1.0},
        {"stop_loss", 0.02},
        {"take_profit", 0.03},
        {"consensus_threshold", 0.6}
    };
    
    EnsembleStrategy strategy(config);
    
    SECTION("Default configuration values") {
        REQUIRE(strategy.getStatus()["position_size"] == 1.0);
        REQUIRE(strategy.getStatus()["stop_loss"] == 0.02);
        REQUIRE(strategy.getStatus()["take_profit"] == 0.03);
        REQUIRE(strategy.getStatus()["consensus_threshold"] == 0.6);
    }
}

TEST_CASE("EnsembleStrategy signal generation", "[strategies]") {
    nlohmann::json config = {
        {"position_size", 1.0},
        {"stop_loss", 0.02},
        {"take_profit", 0.03},
        {"consensus_threshold", 0.6}
    };
    
    EnsembleStrategy strategy(config);
    
    SECTION("Empty strategy list returns HOLD") {
        auto signal = strategy.generateSignal();
        REQUIRE(signal.type == SignalType::HOLD);
        REQUIRE(signal.size == 0.0);
    }
    
    SECTION("Single strategy signal is passed through") {
        auto mockStrategy = std::make_shared<MockStrategy>(SignalType::BUY);
        strategy.addStrategy(mockStrategy, 1.0, "mock");
        
        MarketData data;
        strategy.update(data);
        auto signal = strategy.generateSignal();
        
        REQUIRE(signal.type == SignalType::BUY);
        REQUIRE(signal.size == 1.0);
    }
    
    SECTION("Multiple strategies with consensus") {
        auto buyStrategy = std::make_shared<MockStrategy>(SignalType::BUY);
        auto sellStrategy = std::make_shared<MockStrategy>(SignalType::SELL);
        
        strategy.addStrategy(buyStrategy, 0.7, "buy");
        strategy.addStrategy(sellStrategy, 0.3, "sell");
        
        MarketData data;
        strategy.update(data);
        auto signal = strategy.generateSignal();
        
        REQUIRE(signal.type == SignalType::BUY);
        REQUIRE(signal.size == 1.0);
    }
}

TEST_CASE("EnsembleStrategy performance tracking", "[strategies]") {
    nlohmann::json config = {
        {"position_size", 1.0},
        {"stop_loss", 0.02},
        {"take_profit", 0.03},
        {"consensus_threshold", 0.6},
        {"performance_window", 10},
        {"min_win_rate", 0.4},
        {"max_drawdown_threshold", 0.2},
        {"min_sharpe_ratio", 0.5}
    };
    
    EnsembleStrategy strategy(config);
    auto mockStrategy = std::make_shared<MockStrategy>(SignalType::BUY);
    strategy.addStrategy(mockStrategy, 1.0, "mock");
    
    SECTION("Performance metrics update correctly") {
        // Simulate some trades
        for (int i = 0; i < 5; ++i) {
            strategy.updatePerformance("mock", 0.01, true); // Winning trade
        }
        for (int i = 0; i < 5; ++i) {
            strategy.updatePerformance("mock", -0.01, false); // Losing trade
        }
        
        auto status = strategy.getStatus();
        auto mockStatus = status["strategies"]["mock"];
        
        REQUIRE_THAT(mockStatus["win_rate"], 
                    Catch::Matchers::WithinRel(0.5, 0.01));
        REQUIRE_THAT(mockStatus["average_return"], 
                    Catch::Matchers::WithinRel(0.0, 0.01));
        REQUIRE(mockStatus["total_trades"] == 10);
        REQUIRE(mockStatus["winning_trades"] == 5);
    }
    
    SECTION("Strategy validation works correctly") {
        // Good performance
        for (int i = 0; i < 10; ++i) {
            strategy.updatePerformance("mock", 0.01, true);
        }
        REQUIRE(strategy.isStrategyValid("mock"));
        
        // Poor performance
        for (int i = 0; i < 10; ++i) {
            strategy.updatePerformance("mock", -0.3, false);
        }
        REQUIRE_FALSE(strategy.isStrategyValid("mock"));
    }
}

TEST_CASE("EnsembleStrategy weight updates", "[strategies]") {
    nlohmann::json config = {
        {"position_size", 1.0},
        {"stop_loss", 0.02},
        {"take_profit", 0.03},
        {"consensus_threshold", 0.6}
    };
    
    EnsembleStrategy strategy(config);
    auto strategy1 = std::make_shared<MockStrategy>(SignalType::BUY);
    auto strategy2 = std::make_shared<MockStrategy>(SignalType::BUY);
    
    strategy.addStrategy(strategy1, 0.5, "strategy1");
    strategy.addStrategy(strategy2, 0.5, "strategy2");
    
    SECTION("Weights update based on performance") {
        // Strategy 1 performs well
        for (int i = 0; i < 10; ++i) {
            strategy.updatePerformance("strategy1", 0.01, true);
        }
        
        // Strategy 2 performs poorly
        for (int i = 0; i < 10; ++i) {
            strategy.updatePerformance("strategy2", -0.01, false);
        }
        
        strategy.updateStrategyWeights();
        auto status = strategy.getStatus();
        
        REQUIRE(status["strategies"]["strategy1"]["weight"] > 
                status["strategies"]["strategy2"]["weight"]);
    }
} 