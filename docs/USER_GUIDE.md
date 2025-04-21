# User Guide

## Overview

This guide provides step-by-step instructions for installing, configuring, and using the trading bot system. It covers everything from initial setup to advanced usage scenarios.

## Installation

### Prerequisites

1. **System Requirements**
   - Operating System: Linux (Ubuntu 20.04+), macOS (10.15+), or Windows (10+ with WSL2)
   - CPU: 2+ cores
   - RAM: 4+ GB
   - Storage: 10+ GB free space
   - Internet connection

2. **Required Software**
   - C++17 compatible compiler
   - CMake 3.10+
   - Git
   - OpenMP
   - nlohmann/json

### Installation Steps

1. **Linux (Ubuntu)**
```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential cmake git libomp-dev nlohmann-json3-dev

# Clone repository
git clone https://github.com/doomhammerhell/tradingbot.git
cd tradingbot

# Build project
mkdir build && cd build
cmake ..
make -j$(nproc)
```

2. **macOS**
```bash
# Install Homebrew if not installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake git libomp nlohmann-json

# Clone repository
git clone https://github.com/doomhammerhell/tradingbot.git
cd tradingbot

# Build project
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

3. **Windows (WSL2)**
```bash
# Follow Ubuntu instructions above
# Or use Visual Studio with C++ workload
```

## Configuration

### Initial Setup

1. **Create Configuration File**
```bash
cp config.example.json config.json
```

2. **Edit Configuration**
```json
{
    "api": {
        "binance": {
            "api_key": "YOUR_API_KEY",
            "api_secret": "YOUR_API_SECRET"
        },
        "coinbase": {
            "api_key": "YOUR_API_KEY",
            "api_secret": "YOUR_API_SECRET"
        }
    },
    "trading": {
        "initial_capital": 10000.0,
        "transaction_cost": 0.001,
        "risk_per_trade": 0.02
    },
    "strategies": {
        "mean_reversion": {
            "enabled": true,
            "parameters": {
                "z_score_threshold": 2.0,
                "window_size": 20,
                "take_profit": 0.5,
                "stop_loss": 0.1
            }
        }
    }
}
```

3. **Environment Variables**
```bash
export TRADINGBOT_API_KEY=your_api_key
export TRADINGBOT_API_SECRET=your_api_secret
```

## Basic Usage

### Starting the Bot

1. **Command Line**
```bash
./tradingbot --config config.json
```

2. **Docker**
```bash
docker run -d \
  -v $(pwd)/config.json:/app/config.json \
  -e TRADINGBOT_API_KEY=your_api_key \
  -e TRADINGBOT_API_SECRET=your_api_secret \
  tradingbot:latest
```

### Managing Strategies

1. **List Strategies**
```bash
./tradingbot --list-strategies
```

2. **Enable Strategy**
```bash
./tradingbot --enable-strategy mean_reversion
```

3. **Disable Strategy**
```bash
./tradingbot --disable-strategy mean_reversion
```

### Monitoring

1. **View Status**
```bash
./tradingbot --status
```

2. **View Logs**
```bash
./tradingbot --logs
```

3. **View Metrics**
```bash
./tradingbot --metrics
```

## Advanced Usage

### Backtesting

1. **Run Backtest**
```bash
./tradingbot --backtest \
  --strategy mean_reversion \
  --symbol BTCUSDT \
  --interval 1h \
  --start 2023-01-01 \
  --end 2023-12-31
```

2. **View Results**
```bash
./tradingbot --backtest-results
```

### Optimization

1. **Optimize Strategy**
```bash
./tradingbot --optimize \
  --strategy mean_reversion \
  --parameter z_score_threshold:1.0:3.0 \
  --parameter window_size:10:50 \
  --generations 50 \
  --population 100
```

2. **View Optimization Results**
```bash
./tradingbot --optimization-results
```

### Custom Strategies

1. **Create Strategy File**
```cpp
// my_strategy.hpp
#pragma once
#include "IStrategy.hpp"

class MyStrategy : public IStrategy {
public:
    struct Config {
        double parameter1;
        int parameter2;
    };

    MyStrategy(const Config& config);
    Signal generateSignal(const MarketData& data) override;
    void updateState(const MarketData& data) override;
    void reset() override;

private:
    Config config_;
};
```

2. **Implement Strategy**
```cpp
// my_strategy.cpp
#include "my_strategy.hpp"

MyStrategy::MyStrategy(const Config& config)
    : config_(config) {}

Signal MyStrategy::generateSignal(const MarketData& data) {
    // Implement signal generation logic
    return Signal::HOLD;
}

void MyStrategy::updateState(const MarketData& data) {
    // Update internal state
}

void MyStrategy::reset() {
    // Reset internal state
}
```

3. **Register Strategy**
```json
{
    "strategies": {
        "my_strategy": {
            "enabled": true,
            "parameters": {
                "parameter1": 1.0,
                "parameter2": 10
            }
        }
    }
}
```

## Troubleshooting

### Common Issues

1. **API Connection Issues**
   - Check API keys
   - Verify network connection
   - Check rate limits
   - Review API documentation

2. **Strategy Issues**
   - Check parameter ranges
   - Review strategy logic
   - Check data quality
   - Monitor performance

3. **Performance Issues**
   - Check system resources
   - Review configuration
   - Monitor memory usage
   - Check for bottlenecks

### Debugging

1. **Enable Debug Mode**
```bash
./tradingbot --debug
```

2. **View Detailed Logs**
```bash
./tradingbot --verbose
```

3. **Check System Status**
```bash
./tradingbot --system-status
```

## Best Practices

1. **Risk Management**
   - Start with small capital
   - Use stop losses
   - Diversify strategies
   - Monitor performance

2. **Strategy Development**
   - Test thoroughly
   - Use backtesting
   - Optimize parameters
   - Monitor live performance

3. **System Maintenance**
   - Regular updates
   - Monitor logs
   - Backup configuration
   - Check system health

4. **Security**
   - Secure API keys
   - Use strong passwords
   - Regular key rotation
   - Monitor access

## FAQ

1. **How do I reset the bot?**
```bash
./tradingbot --reset
```

2. **How do I update the bot?**
```bash
git pull
cd build
cmake ..
make -j$(nproc)
```

3. **How do I backup my configuration?**
```bash
cp config.json config.backup.json
```

4. **How do I restore from backup?**
```bash
cp config.backup.json config.json
```

## Support

1. **Documentation**
   - [API Documentation](API.md)
   - [Strategy Documentation](STRATEGIES.md)
   - [Development Guide](DEVELOPMENT.md)

2. **Community**
   - GitHub Issues
   - Discord Server
   - Stack Overflow

3. **Contact**
   - Email: support@tradingbot.com
   - GitHub: github.com/doomhammerhell/tradingbot
   - Twitter: @tradingbot 