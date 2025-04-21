# Trading Strategies Documentation

## Overview

The trading bot implements several technical analysis-based trading strategies. Each strategy follows the `IStrategy` interface and can be used independently or in combination with others.

## Strategy Interface

```cpp
class IStrategy {
public:
    virtual ~IStrategy() = default;
    virtual Signal generateSignal(const MarketData& data) = 0;
    virtual void updateState(const MarketData& data) = 0;
    virtual void reset() = 0;
};
```

## Mean Reversion Strategy

### Overview
The Mean Reversion strategy identifies overbought and oversold conditions using statistical measures, assuming prices will revert to their mean.

### Implementation
```cpp
class MeanReversionStrategy : public IStrategy {
public:
    struct Config {
        double z_score_threshold;  // Signal threshold (default: 2.0)
        int window_size;          // Lookback period (default: 20)
        double take_profit;       // Profit target (default: 0.5)
        double stop_loss;         // Loss limit (default: 0.1)
    };

    MeanReversionStrategy(const Config& config);
    Signal generateSignal(const MarketData& data) override;
    void updateState(const MarketData& data) override;
    void reset() override;

private:
    double calculateZScore(const std::vector<double>& prices) const;
    std::vector<double> price_history_;
    Config config_;
};
```

### Signal Generation
1. Calculate moving average over window_size
2. Calculate standard deviation
3. Compute z-score: (current_price - mean) / std_dev
4. Generate signals:
   - BUY when z-score < -threshold
   - SELL when z-score > threshold
   - HOLD otherwise

### Parameters
- `z_score_threshold`: Controls sensitivity (higher = less signals)
- `window_size`: Affects mean calculation stability
- `take_profit`: Profit target percentage
- `stop_loss`: Maximum loss percentage

### Example Usage
```cpp
MeanReversionStrategy::Config config{
    .z_score_threshold = 2.0,
    .window_size = 20,
    .take_profit = 0.5,
    .stop_loss = 0.1
};
MeanReversionStrategy strategy(config);

MarketData data{...};
auto signal = strategy.generateSignal(data);
```

## Momentum Strategy

### Overview
The Momentum strategy identifies trends by tracking price changes over a lookback period.

### Implementation
```cpp
class MomentumStrategy : public IStrategy {
public:
    struct Config {
        int lookback_period;      // Momentum calculation window
        double threshold;         // Signal threshold
        double take_profit;       // Profit target
        double stop_loss;         // Loss limit
    };

    MomentumStrategy(const Config& config);
    Signal generateSignal(const MarketData& data) override;
    void updateState(const MarketData& data) override;
    void reset() override;

private:
    double calculateMomentum(const std::vector<double>& prices) const;
    std::vector<double> price_history_;
    Config config_;
};
```

### Signal Generation
1. Calculate price change over lookback_period
2. Normalize by initial price
3. Generate signals:
   - BUY when momentum > threshold
   - SELL when momentum < -threshold
   - HOLD otherwise

### Parameters
- `lookback_period`: Affects trend sensitivity
- `threshold`: Controls signal frequency
- `take_profit`: Profit target percentage
- `stop_loss`: Maximum loss percentage

### Example Usage
```cpp
MomentumStrategy::Config config{
    .lookback_period = 10,
    .threshold = 0.02,
    .take_profit = 0.5,
    .stop_loss = 0.1
};
MomentumStrategy strategy(config);
```

## Bollinger Bands Strategy

### Overview
The Bollinger Bands strategy uses volatility-based bands to identify potential price reversals.

### Implementation
```cpp
class BollingerBandsStrategy : public IStrategy {
public:
    struct Config {
        int window_size;          // Moving average period
        double num_std_dev;       // Standard deviation multiplier
        double take_profit;       // Profit target
        double stop_loss;         // Loss limit
    };

    BollingerBandsStrategy(const Config& config);
    Signal generateSignal(const MarketData& data) override;
    void updateState(const MarketData& data) override;
    void reset() override;

private:
    struct Bands {
        double upper;
        double middle;
        double lower;
    };
    Bands calculateBands(const std::vector<double>& prices) const;
    std::vector<double> price_history_;
    Config config_;
};
```

### Signal Generation
1. Calculate moving average (middle band)
2. Calculate standard deviation
3. Compute upper/lower bands: middle ± (std_dev * num_std_dev)
4. Generate signals:
   - BUY when price crosses below lower band
   - SELL when price crosses above upper band
   - HOLD otherwise

### Parameters
- `window_size`: Affects band smoothness
- `num_std_dev`: Controls band width
- `take_profit`: Profit target percentage
- `stop_loss`: Maximum loss percentage

### Example Usage
```cpp
BollingerBandsStrategy::Config config{
    .window_size = 20,
    .num_std_dev = 2.0,
    .take_profit = 0.5,
    .stop_loss = 0.1
};
BollingerBandsStrategy strategy(config);
```

## RSI Strategy

### Overview
The RSI (Relative Strength Index) strategy uses momentum oscillator to identify overbought/oversold conditions.

### Implementation
```cpp
class RSIStrategy : public IStrategy {
public:
    struct Config {
        int period;              // RSI calculation period
        double overbought;       // Upper threshold
        double oversold;         // Lower threshold
        double take_profit;      // Profit target
        double stop_loss;        // Loss limit
    };

    RSIStrategy(const Config& config);
    Signal generateSignal(const MarketData& data) override;
    void updateState(const MarketData& data) override;
    void reset() override;

private:
    double calculateRSI(const std::vector<double>& prices) const;
    std::vector<double> price_history_;
    Config config_;
};
```

### Signal Generation
1. Calculate price changes
2. Separate gains and losses
3. Compute average gain/loss
4. Calculate RSI: 100 - (100 / (1 + RS))
5. Generate signals:
   - BUY when RSI < oversold
   - SELL when RSI > overbought
   - HOLD otherwise

### Parameters
- `period`: Affects RSI sensitivity
- `overbought`: Upper threshold (default: 70)
- `oversold`: Lower threshold (default: 30)
- `take_profit`: Profit target percentage
- `stop_loss`: Maximum loss percentage

### Example Usage
```cpp
RSIStrategy::Config config{
    .period = 14,
    .overbought = 70.0,
    .oversold = 30.0,
    .take_profit = 0.5,
    .stop_loss = 0.1
};
RSIStrategy strategy(config);
```

## Custom Strategy Development

### Creating a New Strategy

1. Create header file:
```cpp
#pragma once
#include "IStrategy.hpp"

class CustomStrategy : public IStrategy {
public:
    struct Config {
        // Add strategy-specific parameters
    };

    CustomStrategy(const Config& config);
    Signal generateSignal(const MarketData& data) override;
    void updateState(const MarketData& data) override;
    void reset() override;

private:
    // Add private members
    Config config_;
};
```

2. Implement the strategy:
```cpp
#include "CustomStrategy.hpp"

CustomStrategy::CustomStrategy(const Config& config)
    : config_(config) {}

Signal CustomStrategy::generateSignal(const MarketData& data) {
    // Implement signal generation logic
    return Signal::HOLD;
}

void CustomStrategy::updateState(const MarketData& data) {
    // Update internal state
}

void CustomStrategy::reset() {
    // Reset internal state
}
```

3. Add to strategy factory:
```cpp
std::unique_ptr<IStrategy> createStrategy(
    const std::string& name,
    const json& config
) {
    if (name == "custom") {
        return std::make_unique<CustomStrategy>(
            CustomStrategy::Config{
                // Initialize from config
            }
        );
    }
    // ... other strategies
}
```

### Best Practices

1. **State Management**
   - Keep state minimal
   - Document state transitions
   - Handle edge cases
   - Implement proper reset

2. **Performance**
   - Pre-allocate memory
   - Use efficient algorithms
   - Minimize calculations
   - Cache results when possible

3. **Testing**
   - Unit test signal generation
   - Test edge cases
   - Verify state management
   - Performance benchmark

4. **Documentation**
   - Document parameters
   - Explain signal logic
   - Provide examples
   - Note limitations 