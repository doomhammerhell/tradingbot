# Trading Bot System Documentation

## Overview

The Trading Bot System is a sophisticated algorithmic trading platform written in C++. It provides a comprehensive suite of tools for backtesting and optimizing trading strategies using various technical indicators and optimization algorithms.

## System Architecture

### Core Components

1. **Strategy Module**
   - Base strategy interface (`IStrategy`)
   - Implemented strategies:
     - Mean Reversion
     - Momentum
     - Bollinger Bands
     - RSI (Relative Strength Index)
   - Each strategy implements signal generation and state management

2. **Optimization Module**
   - Parameter optimization framework
   - Implemented algorithms:
     - Genetic Algorithm
     - Hill Climbing
   - Parallel evaluation support via OpenMP
   - Configurable fitness functions

3. **Backtesting Engine**
   - Historical data processing
   - Performance metrics calculation
   - Trade execution simulation

4. **Data Management**
   - Market data handling
   - CSV data import/export
   - JSON configuration and results storage

## Technical Details

### Strategy Implementation

#### Mean Reversion Strategy
- Uses z-score for identifying overbought/oversold conditions
- Parameters:
  - `z_score_threshold`: Signal threshold
  - `window_size`: Lookback period
  - `take_profit`: Profit target
  - `stop_loss`: Loss limit

#### Momentum Strategy
- Tracks price momentum over a lookback period
- Parameters:
  - `lookback_period`: Momentum calculation window
  - `threshold`: Signal threshold
  - `take_profit`: Profit target
  - `stop_loss`: Loss limit

#### Bollinger Bands Strategy
- Uses standard deviation bands for volatility-based signals
- Parameters:
  - `window_size`: Moving average period
  - `num_std_dev`: Standard deviation multiplier
  - `take_profit`: Profit target
  - `stop_loss`: Loss limit

#### RSI Strategy
- Implements Relative Strength Index indicator
- Parameters:
  - `period`: RSI calculation period
  - `overbought`: Upper threshold
  - `oversold`: Lower threshold
  - `take_profit`: Profit target
  - `stop_loss`: Loss limit

### Optimization Algorithms

#### Genetic Algorithm
- Population-based optimization
- Features:
  - Tournament selection
  - Crossover and mutation
  - Elitism
  - Parallel evaluation
- Parameters:
  - `population_size`: Number of individuals
  - `generations`: Number of iterations
  - `mutation_rate`: Probability of mutation
  - `crossover_rate`: Probability of crossover

#### Hill Climbing
- Local search optimization
- Features:
  - Random restart
  - Adaptive step size
  - Parallel neighbor evaluation
- Parameters:
  - `max_iterations`: Number of iterations
  - `step_size`: Initial step size
  - `step_decay`: Step size reduction rate

### Performance Metrics

The system calculates various performance metrics:
- Total Profit
- Sharpe Ratio
- Maximum Drawdown
- Win Rate
- Sortino Ratio
- Maximum Trade Duration

### Fitness Function

The optimization process uses a weighted fitness function:
```cpp
fitness = profit_weight * total_profit +
          sharpe_weight * sharpe_ratio -
          drawdown_weight * max_drawdown +
          win_rate_weight * win_rate +
          sortino_weight * sortino_ratio -
          max_trade_duration_weight * max_trade_duration
```

## Usage

### Compilation

```bash
# Create build directory
mkdir build && cd build

# Configure with optimization enabled
cmake -DENABLE_OPTIMIZATION=ON ..

# Build
make
```

### Running Optimization

```bash
# Basic usage
./tradingbot_optimize data.csv

# With specific strategy and algorithm
./tradingbot_optimize --strategy=rsi --algorithm=hill_climbing data.csv

# With custom weights
./tradingbot_optimize --weights=profit:0.6,sharpe:0.3,drawdown:0.1 data.csv

# With parallel processing
./tradingbot_optimize --threads=4 data.csv
```

### Command Line Options

- `--strategy=<name>`: Select strategy (default: mean_reversion)
- `--algorithm=<name>`: Select optimization algorithm (genetic or hill_climbing)
- `--weights=<config>`: Configure fitness weights
- `--threads=<n>`: Number of threads for parallel processing
- `--generations=<n>`: Number of generations/iterations
- `--population=<n>`: Population size

## Development

### Adding New Strategies

1. Create strategy header file:
```cpp
#pragma once
#include "IStrategy.hpp"

class NewStrategy : public IStrategy {
public:
    struct Config {
        // Strategy parameters
    };
    
    // Implement required methods
    virtual Signal generateSignal(const MarketData& data) override;
    virtual void updateState(const MarketData& data) override;
    virtual void reset() override;
};
```

2. Implement strategy:
```cpp
#include "NewStrategy.hpp"

// Implement methods
```

3. Add to strategy factory in `main_optimize.cpp`

### Adding New Optimization Algorithms

1. Create optimizer header:
```cpp
#pragma once
#include "ParameterOptimizer.hpp"

class NewOptimizer {
public:
    // Constructor and methods
    Individual optimize();
};
```

2. Implement optimizer
3. Add to main optimization logic

## Dependencies

- C++17 or later
- OpenMP
- nlohmann/json
- CMake 3.10 or later

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

- **Mayckon Giovani**
  - Email: doomhammerhell@gmail.com
  - GitHub: [@doomhammerhell](https://github.com/doomhammerhell)

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a new Pull Request 