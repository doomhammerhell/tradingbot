#include <gtest/gtest.h>
#include "../../include/technical/indicators/MACD.hpp"
#include "../../include/core/IDataFeed.hpp"

using namespace tradingbot;

class MockDataFeed : public core::IDataFeed {
public:
    void initialize(const std::string& symbol, const std::string& timeframe) override {}
    void startStreaming() override {}
    void stopStreaming() override {}
    core::MarketData getHistoricalData(int numBars) const override {
        core::MarketData data;
        // Generate some test data with a trend
        for (int i = 0; i < numBars; ++i) {
            data.prices.push_back(100.0 + i * 0.1);
            data.volumes.push_back(1000.0);
            data.timestamps.push_back(i);
        }
        return data;
    }
    void subscribe(std::function<void(const core::MarketData&)> callback) override {}
    std::string getName() const override { return "MockDataFeed"; }
};

TEST(MACDTest, BasicCalculation) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::MACD macd(12, 26, 9);
    
    macd.initialize(dataFeed);
    macd.calculate();
    
    auto macdLine = macd.getMACDLine();
    auto signalLine = macd.getSignalLine();
    auto histogram = macd.getHistogram();
    
    ASSERT_FALSE(macdLine.empty());
    ASSERT_FALSE(signalLine.empty());
    ASSERT_FALSE(histogram.empty());
    
    // Check if values are within expected range
    for (const auto& value : macdLine) {
        EXPECT_NE(value, 0.0);
    }
}

TEST(MACDTest, CurrentValues) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::MACD macd(12, 26, 9);
    
    macd.initialize(dataFeed);
    macd.calculate();
    
    double currentMACD = macd.getCurrentValue();
    double currentSignal = macd.getCurrentSignal();
    double currentHistogram = macd.getCurrentHistogram();
    
    EXPECT_NE(currentMACD, 0.0);
    EXPECT_NE(currentSignal, 0.0);
    EXPECT_NE(currentHistogram, 0.0);
}

TEST(MACDTest, HistogramCalculation) {
    auto dataFeed = std::make_shared<MockDataFeed>();
    technical::indicators::MACD macd(12, 26, 9);
    
    macd.initialize(dataFeed);
    macd.calculate();
    
    auto macdLine = macd.getMACDLine();
    auto signalLine = macd.getSignalLine();
    auto histogram = macd.getHistogram();
    
    // Verify histogram calculation
    for (size_t i = 0; i < histogram.size(); ++i) {
        EXPECT_DOUBLE_EQ(histogram[i], macdLine[i] - signalLine[i]);
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 