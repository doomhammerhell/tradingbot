# Trading Bot

A sophisticated algorithmic trading platform written in C++ that provides tools for backtesting and optimizing trading strategies using various technical indicators and optimization algorithms.

## Features

- Multiple trading strategies:
  - Mean Reversion
  - Momentum
  - Bollinger Bands
  - RSI (Relative Strength Index)
- Optimization algorithms:
  - Genetic Algorithm
  - Hill Climbing
- Parallel processing support
- Configurable fitness functions
- Comprehensive performance metrics
- JSON-based configuration and results

## Quick Start

### Prerequisites

- C++17 or later
- OpenMP
- nlohmann/json
- CMake 3.10 or later

### Installation

```bash
# Clone the repository
git clone https://github.com/doomhammerhell/tradingbot.git
cd tradingbot

# Create build directory
mkdir build && cd build

# Configure and build
cmake -DENABLE_OPTIMIZATION=ON ..
make
```

### Usage

```bash
# Basic optimization
./tradingbot_optimize data.csv

# With specific strategy and algorithm
./tradingbot_optimize --strategy=rsi --algorithm=hill_climbing data.csv

# With custom weights
./tradingbot_optimize --weights=profit:0.6,sharpe:0.3,drawdown:0.1 data.csv
```

## Documentation

For detailed documentation, please see the [docs](docs/README.md) directory.

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
