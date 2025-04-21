#include <gtest/gtest.h>
#include "../../include/technical/indicators/RSI.hpp"
#include "../../include/core/IDataFeed.hpp"

using namespace tradingbot;

class MockDataFeed : public core::IDataFeed {
public:
    void initialize(const std::string& symbol, const std::string& timeframe) override {}
    void startStreaming() override {}
    void stopStreaming() override {}
    core::MarketData getHistoricalData(int numBars) const override {
        core::MarketData data;
        // Generate some test data with alternating up and down movements
        for (int i = 0; i < numBars; ++i) {
            data.prices.push_back(100.0 + (i % 2 == 0 ? 1.0 : -1.0));
            data.volumes.push_back(1000.0);
            data.timestamps.push_back(i);
        }
        return data;
    }
    void subscribe(std::function<void(const core::MarketData&)> callback) override {}
    std::string getName() const override { return "MockDataFeed"; }
};

TEST(RSITest, BasicCalculation) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::RSI rsi(14);
    
    rsi.initialize(dataFeed);
    rsi.calculate();
    
    auto values = rsi.getValues();
    ASSERT_FALSE(values.empty());
    
    // RSI values should be between 0 and 100
    for (const auto& value : values) {
        EXPECT_GE(value, 0.0);
        EXPECT_LE(value, 100.0);
    }
}

TEST(RSITest, PeriodChange) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::RSI rsi(14);
    
    rsi.initialize(dataFeed);
    rsi.calculate();
    auto values1 = rsi.getValues();
    
    rsi.setPeriod(7);
    auto values2 = rsi.getValues();
    
    // Values should be different after period change
    EXPECT_NE(values1, values2);
}

TEST(RSITest, CurrentValue) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::RSI rsi(14);
    
    rsi.initialize(dataFeed);
    rsi.calculate();
    
    double currentValue = rsi.getCurrentValue();
    EXPECT_GE(currentValue, 0.0);
    EXPECT_LE(currentValue, 100.0);
}

TEST(RSITest, Thresholds) {
    technical::indicators::RSI rsi(14);
    EXPECT_EQ(rsi.getOverboughtThreshold(), 70.0);
    EXPECT_EQ(rsi.getOversoldThreshold(), 30.0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 