# Backtesting Module Documentation

## Overview

The backtesting module provides a framework for simulating trading strategies on historical data. It includes performance metrics calculation, trade execution simulation, and data management capabilities.

## Core Components

### Market Data

```cpp
struct MarketData {
    double open;           // Opening price
    double high;           // Highest price
    double low;            // Lowest price
    double close;          // Closing price
    double volume;         // Trading volume
    std::chrono::system_clock::time_point timestamp;  // Time of data point
};
```

### Trade

```cpp
struct Trade {
    TradeType type;        // LONG or SHORT
    double entry_price;    // Price at entry
    double exit_price;     // Price at exit
    double quantity;       // Trade size
    double profit;         // Profit/loss
    std::chrono::system_clock::time_point entry_time;  // Time of entry
    std::chrono::system_clock::time_point exit_time;   // Time of exit
    double duration;       // Trade duration in hours
};
```

### Performance Metrics

```cpp
struct PerformanceMetrics {
    double total_profit;           // Total profit/loss
    double sharpe_ratio;           // Risk-adjusted return
    double max_drawdown;           // Maximum drawdown
    double win_rate;               // Percentage of winning trades
    double sortino_ratio;          // Downside risk-adjusted return
    double max_trade_duration;     // Longest trade duration
    double average_profit;         // Average profit per trade
    double profit_factor;          // Gross profit / gross loss
    double recovery_factor;        // Net profit / max drawdown
    double risk_reward_ratio;      // Average profit / average loss
};
```

## Backtesting Engine

### Implementation
```cpp
class BacktestEngine {
public:
    BacktestEngine(
        std::unique_ptr<IStrategy> strategy,
        const std::vector<MarketData>& historical_data,
        double initial_capital,
        double transaction_cost
    );

    void run();
    const std::vector<Trade>& getTrades() const;
    PerformanceMetrics getMetrics() const;
    double getEquityCurve() const;

private:
    Trade executeTrade(Signal signal, const MarketData& data);
    void updatePosition(const Trade& trade);
    PerformanceMetrics calculateMetrics() const;
    double calculateSharpeRatio() const;
    double calculateSortinoRatio() const;
    double calculateMaxDrawdown() const;
    
    std::unique_ptr<IStrategy> strategy_;
    std::vector<MarketData> historical_data_;
    std::vector<Trade> trades_;
    double initial_capital_;
    double current_capital_;
    double transaction_cost_;
    Position current_position_;
};
```

### Trade Execution

1. **Signal Processing**
   - Receive signal from strategy
   - Check current position
   - Validate signal timing
   - Apply risk management

2. **Order Execution**
   - Calculate position size
   - Apply transaction costs
   - Update capital
   - Record trade

3. **Position Management**
   - Track open positions
   - Monitor stop loss
   - Check take profit
   - Handle position closing

### Example Usage
```cpp
// Create strategy
MeanReversionStrategy::Config config{
    .z_score_threshold = 2.0,
    .window_size = 20,
    .take_profit = 0.5,
    .stop_loss = 0.1
};
auto strategy = std::make_unique<MeanReversionStrategy>(config);

// Load historical data
std::vector<MarketData> data = loadHistoricalData("data.csv");

// Create backtesting engine
BacktestEngine engine(
    std::move(strategy),
    data,
    10000.0,    // initial_capital
    0.001       // transaction_cost
);

// Run backtest
engine.run();

// Get results
auto trades = engine.getTrades();
auto metrics = engine.getMetrics();
auto equity_curve = engine.getEquityCurve();
```

## Performance Metrics

### Calculation Methods

1. **Total Profit**
```cpp
double calculateTotalProfit() const {
    return std::accumulate(
        trades_.begin(),
        trades_.end(),
        0.0,
        [](double sum, const Trade& trade) {
            return sum + trade.profit;
        }
    );
}
```

2. **Sharpe Ratio**
```cpp
double calculateSharpeRatio() const {
    std::vector<double> returns;
    returns.reserve(trades_.size());
    
    for (const auto& trade : trades_) {
        returns.push_back(trade.profit / trade.entry_price);
    }
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double variance = 0.0;
    
    for (double r : returns) {
        variance += (r - mean) * (r - mean);
    }
    variance /= returns.size();
    
    double std_dev = std::sqrt(variance);
    return mean / std_dev * std::sqrt(252); // Annualized
}
```

3. **Maximum Drawdown**
```cpp
double calculateMaxDrawdown() const {
    double peak = initial_capital_;
    double max_drawdown = 0.0;
    double current_equity = initial_capital_;
    
    for (const auto& trade : trades_) {
        current_equity += trade.profit;
        if (current_equity > peak) {
            peak = current_equity;
        }
        double drawdown = (peak - current_equity) / peak;
        if (drawdown > max_drawdown) {
            max_drawdown = drawdown;
        }
    }
    
    return max_drawdown;
}
```

4. **Win Rate**
```cpp
double calculateWinRate() const {
    int winning_trades = std::count_if(
        trades_.begin(),
        trades_.end(),
        [](const Trade& trade) {
            return trade.profit > 0;
        }
    );
    
    return static_cast<double>(winning_trades) / trades_.size();
}
```

## Data Management

### Historical Data Loading
```cpp
std::vector<MarketData> loadHistoricalData(const std::string& filename) {
    std::vector<MarketData> data;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    std::string line;
    std::getline(file, line); // Skip header
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        MarketData point;
        
        // Parse CSV line
        std::getline(ss, token, ',');
        point.timestamp = parseTimestamp(token);
        
        std::getline(ss, token, ',');
        point.open = std::stod(token);
        
        std::getline(ss, token, ',');
        point.high = std::stod(token);
        
        std::getline(ss, token, ',');
        point.low = std::stod(token);
        
        std::getline(ss, token, ',');
        point.close = std::stod(token);
        
        std::getline(ss, token, ',');
        point.volume = std::stod(token);
        
        data.push_back(point);
    }
    
    return data;
}
```

### Results Export
```cpp
void exportResults(
    const std::string& filename,
    const std::vector<Trade>& trades,
    const PerformanceMetrics& metrics
) {
    json result;
    
    // Add trades
    result["trades"] = json::array();
    for (const auto& trade : trades) {
        result["trades"].push_back({
            {"type", trade.type == TradeType::LONG ? "LONG" : "SHORT"},
            {"entry_price", trade.entry_price},
            {"exit_price", trade.exit_price},
            {"quantity", trade.quantity},
            {"profit", trade.profit},
            {"entry_time", formatTimestamp(trade.entry_time)},
            {"exit_time", formatTimestamp(trade.exit_time)},
            {"duration", trade.duration}
        });
    }
    
    // Add metrics
    result["metrics"] = {
        {"total_profit", metrics.total_profit},
        {"sharpe_ratio", metrics.sharpe_ratio},
        {"max_drawdown", metrics.max_drawdown},
        {"win_rate", metrics.win_rate},
        {"sortino_ratio", metrics.sortino_ratio},
        {"max_trade_duration", metrics.max_trade_duration},
        {"average_profit", metrics.average_profit},
        {"profit_factor", metrics.profit_factor},
        {"recovery_factor", metrics.recovery_factor},
        {"risk_reward_ratio", metrics.risk_reward_ratio}
    };
    
    // Write to file
    std::ofstream file(filename);
    file << result.dump(4);
}
```

## Best Practices

1. **Data Quality**
   - Validate historical data
   - Handle missing values
   - Check for outliers
   - Ensure chronological order

2. **Transaction Costs**
   - Include commission
   - Account for slippage
   - Consider spread
   - Model market impact

3. **Risk Management**
   - Implement position sizing
   - Set stop losses
   - Use take profits
   - Monitor drawdown

4. **Performance Analysis**
   - Calculate multiple metrics
   - Compare benchmarks
   - Analyze trade distribution
   - Check for overfitting 