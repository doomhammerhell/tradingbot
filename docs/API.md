# API Documentation

## Overview

This document provides comprehensive documentation for all APIs used in the trading bot system, including both internal and external interfaces.

## External APIs

### Exchange APIs

#### Binance API
```cpp
class BinanceAPI {
public:
    // Market Data
    MarketData getMarketData(const std::string& symbol, const std::string& interval);
    std::vector<MarketData> getHistoricalData(
        const std::string& symbol,
        const std::string& interval,
        int limit
    );

    // Account Management
    AccountInfo getAccountInfo();
    std::vector<Order> getOpenOrders();
    std::vector<Trade> getTradeHistory();

    // Order Management
    Order placeOrder(const OrderRequest& request);
    void cancelOrder(const std::string& orderId);
    Order getOrderStatus(const std::string& orderId);

private:
    std::string api_key_;
    std::string api_secret_;
    HttpClient client_;
};
```

#### Coinbase API
```cpp
class CoinbaseAPI {
public:
    // Market Data
    MarketData getMarketData(const std::string& symbol);
    std::vector<MarketData> getHistoricalData(
        const std::string& symbol,
        const std::string& granularity,
        int limit
    );

    // Account Management
    AccountInfo getAccountInfo();
    std::vector<Wallet> getWallets();
    std::vector<Transaction> getTransactions();

    // Order Management
    Order placeOrder(const OrderRequest& request);
    void cancelOrder(const std::string& orderId);
    Order getOrderStatus(const std::string& orderId);

private:
    std::string api_key_;
    std::string api_secret_;
    HttpClient client_;
};
```

### Data Provider APIs

#### Alpha Vantage API
```cpp
class AlphaVantageAPI {
public:
    // Market Data
    MarketData getRealTimeData(const std::string& symbol);
    std::vector<MarketData> getHistoricalData(
        const std::string& symbol,
        const std::string& interval,
        const std::string& output_size
    );

    // Technical Indicators
    std::vector<double> getSMA(
        const std::string& symbol,
        int time_period,
        const std::string& interval
    );
    std::vector<double> getRSI(
        const std::string& symbol,
        int time_period,
        const std::string& interval
    );

private:
    std::string api_key_;
    HttpClient client_;
};
```

## Internal APIs

### Strategy API

```cpp
class StrategyAPI {
public:
    // Strategy Management
    void addStrategy(const std::string& name, const json& config);
    void removeStrategy(const std::string& name);
    void updateStrategy(const std::string& name, const json& config);
    
    // Strategy Execution
    Signal getSignal(const std::string& strategy_name, const MarketData& data);
    void updateStrategyState(const std::string& strategy_name, const MarketData& data);
    
    // Strategy Information
    json getStrategyConfig(const std::string& name);
    std::vector<std::string> getActiveStrategies();
    json getStrategyMetrics(const std::string& name);

private:
    std::map<std::string, std::unique_ptr<IStrategy>> strategies_;
    StrategyFactory factory_;
};
```

### Backtesting API

```cpp
class BacktestingAPI {
public:
    // Backtest Management
    void startBacktest(const BacktestConfig& config);
    void stopBacktest();
    void pauseBacktest();
    void resumeBacktest();
    
    // Results
    json getBacktestResults();
    std::vector<Trade> getTrades();
    PerformanceMetrics getMetrics();
    std::vector<double> getEquityCurve();
    
    // Configuration
    void setInitialCapital(double capital);
    void setTransactionCost(double cost);
    void setTimeRange(const TimeRange& range);

private:
    BacktestEngine engine_;
    BacktestConfig config_;
};
```

### Optimization API

```cpp
class OptimizationAPI {
public:
    // Optimization Management
    void startOptimization(const OptimizationConfig& config);
    void stopOptimization();
    void pauseOptimization();
    void resumeOptimization();
    
    // Results
    json getOptimizationResults();
    std::vector<Individual> getPopulation();
    Individual getBestIndividual();
    
    // Configuration
    void setParameterRanges(const std::vector<ParameterRange>& ranges);
    void setFitnessWeights(const FitnessWeights& weights);
    void setOptimizationMethod(const std::string& method);

private:
    ParameterOptimizer optimizer_;
    OptimizationConfig config_;
};
```

## API Endpoints

### REST Endpoints

1. **Strategy Management**
```
POST /api/strategies
GET /api/strategies
GET /api/strategies/{name}
PUT /api/strategies/{name}
DELETE /api/strategies/{name}
```

2. **Backtesting**
```
POST /api/backtest/start
POST /api/backtest/stop
POST /api/backtest/pause
POST /api/backtest/resume
GET /api/backtest/results
GET /api/backtest/trades
GET /api/backtest/metrics
```

3. **Optimization**
```
POST /api/optimization/start
POST /api/optimization/stop
POST /api/optimization/pause
POST /api/optimization/resume
GET /api/optimization/results
GET /api/optimization/population
GET /api/optimization/best
```

### WebSocket Endpoints

1. **Market Data**
```
ws://api/tradingbot/market-data
ws://api/tradingbot/trades
ws://api/tradingbot/orders
```

2. **System Status**
```
ws://api/tradingbot/status
ws://api/tradingbot/logs
ws://api/tradingbot/alerts
```

## Data Formats

### Request Formats

1. **Strategy Configuration**
```json
{
    "name": "mean_reversion",
    "parameters": {
        "z_score_threshold": 2.0,
        "window_size": 20,
        "take_profit": 0.5,
        "stop_loss": 0.1
    }
}
```

2. **Backtest Configuration**
```json
{
    "strategy": "mean_reversion",
    "symbol": "BTCUSDT",
    "interval": "1h",
    "start_time": "2023-01-01T00:00:00Z",
    "end_time": "2023-12-31T23:59:59Z",
    "initial_capital": 10000.0,
    "transaction_cost": 0.001
}
```

3. **Optimization Configuration**
```json
{
    "strategy": "mean_reversion",
    "parameter_ranges": [
        {
            "name": "z_score_threshold",
            "min": 1.0,
            "max": 3.0,
            "is_integer": false
        },
        {
            "name": "window_size",
            "min": 10,
            "max": 50,
            "is_integer": true
        }
    ],
    "fitness_weights": {
        "profit": 0.6,
        "sharpe": 0.3,
        "drawdown": 0.1
    }
}
```

### Response Formats

1. **Market Data**
```json
{
    "symbol": "BTCUSDT",
    "timestamp": "2023-01-01T00:00:00Z",
    "open": 42000.0,
    "high": 42500.0,
    "low": 41800.0,
    "close": 42300.0,
    "volume": 100.5
}
```

2. **Trade**
```json
{
    "id": "trade_123",
    "symbol": "BTCUSDT",
    "type": "LONG",
    "entry_price": 42000.0,
    "exit_price": 42500.0,
    "quantity": 0.1,
    "profit": 50.0,
    "entry_time": "2023-01-01T00:00:00Z",
    "exit_time": "2023-01-01T01:00:00Z",
    "duration": 1.0
}
```

3. **Performance Metrics**
```json
{
    "total_profit": 1500.0,
    "sharpe_ratio": 1.8,
    "max_drawdown": 0.15,
    "win_rate": 0.65,
    "sortino_ratio": 2.1,
    "max_trade_duration": 5,
    "average_profit": 50.0,
    "profit_factor": 2.5,
    "recovery_factor": 3.0,
    "risk_reward_ratio": 2.0
}
```

## Error Handling

### Error Codes

1. **HTTP Status Codes**
   - 200: Success
   - 400: Bad Request
   - 401: Unauthorized
   - 403: Forbidden
   - 404: Not Found
   - 429: Too Many Requests
   - 500: Internal Server Error

2. **Custom Error Codes**
```json
{
    "error": {
        "code": "INVALID_PARAMETER",
        "message": "Invalid parameter value",
        "details": {
            "parameter": "window_size",
            "value": -10,
            "constraint": "Must be positive"
        }
    }
}
```

### Error Responses

1. **Validation Error**
```json
{
    "error": {
        "code": "VALIDATION_ERROR",
        "message": "Invalid request parameters",
        "details": [
            {
                "field": "z_score_threshold",
                "error": "Must be between 1.0 and 3.0"
            },
            {
                "field": "window_size",
                "error": "Must be between 10 and 50"
            }
        ]
    }
}
```

2. **Authentication Error**
```json
{
    "error": {
        "code": "AUTHENTICATION_ERROR",
        "message": "Invalid API key",
        "details": {
            "reason": "API key expired"
        }
    }
}
```

## Rate Limiting

### Limits

1. **REST API**
   - 100 requests per minute
   - 1000 requests per hour
   - 10000 requests per day

2. **WebSocket**
   - 10 connections per IP
   - 100 messages per second
   - 1000 messages per minute

### Headers

```
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 95
X-RateLimit-Reset: 1625097600
```

## Authentication

### API Keys

1. **Generation**
```bash
curl -X POST https://api.tradingbot.com/v1/auth/api-keys \
  -H "Authorization: Bearer {token}" \
  -H "Content-Type: application/json" \
  -d '{"name": "my-api-key", "permissions": ["read", "write"]}'
```

2. **Usage**
```bash
curl -X GET https://api.tradingbot.com/v1/strategies \
  -H "X-API-Key: {api_key}" \
  -H "X-API-Secret: {api_secret}"
```

### JWT Tokens

1. **Login**
```bash
curl -X POST https://api.tradingbot.com/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username": "user", "password": "pass"}'
```

2. **Token Refresh**
```bash
curl -X POST https://api.tradingbot.com/v1/auth/refresh \
  -H "Authorization: Bearer {refresh_token}"
```

## Best Practices

1. **API Usage**
   - Use HTTPS for all requests
   - Implement exponential backoff
   - Cache responses when appropriate
   - Handle rate limits gracefully

2. **Security**
   - Rotate API keys regularly
   - Use secure storage for credentials
   - Implement request signing
   - Validate all inputs

3. **Performance**
   - Use compression for large responses
   - Implement pagination for large datasets
   - Use WebSocket for real-time data
   - Cache frequently accessed data

4. **Error Handling**
   - Implement proper error handling
   - Log errors appropriately
   - Provide meaningful error messages
   - Handle network issues gracefully 