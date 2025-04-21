#include <gtest/gtest.h>
#include "../../../include/technical/indicators/StochRSI.hpp"
#include "../../../include/core/IDataFeed.hpp"
#include <memory>
#include <vector>

using namespace tradingbot::technical::indicators;
using namespace tradingbot::core;

class MockDataFeed : public IDataFeed {
public:
    MockDataFeed(const std::vector<double>& prices) : prices_(prices) {}
    
    std::vector<double> getPrices() const override {
        return prices_;
    }
    
    double getCurrentPrice() const override {
        return prices_.empty() ? 0.0 : prices_.back();
    }
    
    void update() override {}
    
private:
    std::vector<double> prices_;
};

class StochRSITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a sample price series
        std::vector<double> prices = {
            10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0,
            20.0, 19.0, 18.0, 17.0, 16.0, 15.0, 14.0, 13.0, 12.0, 11.0
        };
        
        dataFeed_ = std::make_shared<MockDataFeed>(prices);
        stochRSI_ = std::make_unique<StochRSI>(14, 14, 3);
        stochRSI_->initialize(dataFeed_);
    }
    
    std::shared_ptr<IDataFeed> dataFeed_;
    std::unique_ptr<StochRSI> stochRSI_;
};

TEST_F(StochRSITest, Initialization) {
    EXPECT_EQ(stochRSI_->getName(), "StochRSI_14_14_3");
}

TEST_F(StochRSITest, Calculate) {
    stochRSI_->calculate();
    
    const auto& kValues = stochRSI_->getKValues();
    const auto& dValues = stochRSI_->getDValues();
    
    // Verify that we have calculated values
    EXPECT_FALSE(kValues.empty());
    EXPECT_FALSE(dValues.empty());
    
    // Verify that D values are calculated from K values
    EXPECT_EQ(dValues.size(), kValues.size() - 2); // dPeriod = 3
    
    // Verify that values are within expected range (0-100)
    for (const auto& k : kValues) {
        EXPECT_GE(k, 0.0);
        EXPECT_LE(k, 100.0);
    }
    
    for (const auto& d : dValues) {
        EXPECT_GE(d, 0.0);
        EXPECT_LE(d, 100.0);
    }
}

TEST_F(StochRSITest, GetCurrentValues) {
    stochRSI_->calculate();
    
    double currentK = stochRSI_->getCurrentK();
    double currentD = stochRSI_->getCurrentD();
    
    EXPECT_GE(currentK, 0.0);
    EXPECT_LE(currentK, 100.0);
    EXPECT_GE(currentD, 0.0);
    EXPECT_LE(currentD, 100.0);
}

TEST_F(StochRSITest, EmptyDataFeed) {
    auto emptyDataFeed = std::make_shared<MockDataFeed>(std::vector<double>{});
    stochRSI_->initialize(emptyDataFeed);
    stochRSI_->calculate();
    
    EXPECT_TRUE(stochRSI_->getKValues().empty());
    EXPECT_TRUE(stochRSI_->getDValues().empty());
    EXPECT_EQ(stochRSI_->getCurrentK(), 50.0);
    EXPECT_EQ(stochRSI_->getCurrentD(), 50.0);
} 