#include <gtest/gtest.h>
#include "../../include/strategies/MovingAverageStrategy.hpp"
#include <vector>

using namespace tradingbot::strategies;

class MovingAverageStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        strategy_ = std::make_unique<MovingAverageStrategy>();
    }

    std::unique_ptr<MovingAverageStrategy> strategy_;
};

TEST_F(MovingAverageStrategyTest, ShouldBuyWhenPriceCrossesAboveMA) {
    // Arrange
    std::vector<double> prices = {10.0, 11.0, 12.0, 13.0, 14.0}; // Upward trend
    std::vector<double> ma = {9.0, 10.0, 11.0, 12.0, 13.0}; // MA slightly below prices

    // Act
    bool shouldBuy = strategy_->shouldBuy(prices, ma);

    // Assert
    EXPECT_TRUE(shouldBuy);
}

TEST_F(MovingAverageStrategyTest, ShouldNotBuyWhenPriceBelowMA) {
    // Arrange
    std::vector<double> prices = {10.0, 9.0, 8.0, 7.0, 6.0}; // Downward trend
    std::vector<double> ma = {11.0, 10.0, 9.0, 8.0, 7.0}; // MA above prices

    // Act
    bool shouldBuy = strategy_->shouldBuy(prices, ma);

    // Assert
    EXPECT_FALSE(shouldBuy);
}

TEST_F(MovingAverageStrategyTest, ShouldNotBuyWhenPriceEqualsMA) {
    // Arrange
    std::vector<double> prices = {10.0, 10.0, 10.0, 10.0, 10.0};
    std::vector<double> ma = {10.0, 10.0, 10.0, 10.0, 10.0};

    // Act
    bool shouldBuy = strategy_->shouldBuy(prices, ma);

    // Assert
    EXPECT_FALSE(shouldBuy);
}

TEST_F(MovingAverageStrategyTest, ShouldBuyWhenPriceCrossesMAFromBelow) {
    // Arrange
    std::vector<double> prices = {9.0, 9.5, 10.0, 10.5, 11.0}; // Crossing MA from below
    std::vector<double> ma = {10.0, 10.0, 10.0, 10.0, 10.0}; // Flat MA

    // Act
    bool shouldBuy = strategy_->shouldBuy(prices, ma);

    // Assert
    EXPECT_TRUE(shouldBuy);
}

TEST_F(MovingAverageStrategyTest, ShouldNotBuyWhenPriceCrossesMAFromAbove) {
    // Arrange
    std::vector<double> prices = {11.0, 10.5, 10.0, 9.5, 9.0}; // Crossing MA from above
    std::vector<double> ma = {10.0, 10.0, 10.0, 10.0, 10.0}; // Flat MA

    // Act
    bool shouldBuy = strategy_->shouldBuy(prices, ma);

    // Assert
    EXPECT_FALSE(shouldBuy);
} 