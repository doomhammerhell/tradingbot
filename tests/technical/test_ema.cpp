#include <gtest/gtest.h>
#include "../../include/technical/indicators/EMA.hpp"
#include "../../include/core/IDataFeed.hpp"

using namespace tradingbot;

class MockDataFeed : public core::IDataFeed {
public:
    void initialize(const std::string& symbol, const std::string& timeframe) override {}
    void startStreaming() override {}
    void stopStreaming() override {}
    core::MarketData getHistoricalData(int numBars) const override {
        core::MarketData data;
        // Generate some test data
        for (int i = 0; i < numBars; ++i) {
            data.prices.push_back(100.0 + i);
            data.volumes.push_back(1000.0);
            data.timestamps.push_back(i);
        }
        return data;
    }
    void subscribe(std::function<void(const core::MarketData&)> callback) override {}
    std::string getName() const override { return "MockDataFeed"; }
};

TEST(EMATest, BasicCalculation) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::EMA ema(5);
    
    ema.initialize(dataFeed);
    ema.calculate();
    
    auto values = ema.getValues();
    ASSERT_FALSE(values.empty());
    
    // Check if values are within expected range
    for (const auto& value : values) {
        EXPECT_GT(value, 0.0);
    }
}

TEST(EMATest, PeriodChange) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::EMA ema(5);
    
    ema.initialize(dataFeed);
    ema.calculate();
    auto values1 = ema.getValues();
    
    ema.setPeriod(10);
    auto values2 = ema.getValues();
    
    // Values should be different after period change
    EXPECT_NE(values1, values2);
}

TEST(EMATest, CurrentValue) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::EMA ema(5);
    
    ema.initialize(dataFeed);
    ema.calculate();
    
    double currentValue = ema.getCurrentValue();
    EXPECT_GT(currentValue, 0.0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 