#include "optimization/ParameterOptimizer.hpp"
#include "optimization/HillClimbingOptimizer.hpp"
#include "strategies/MeanReversionStrategy.hpp"
#include "strategies/MomentumStrategy.hpp"
#include "strategies/BollingerBandsStrategy.hpp"
#include "strategies/RSIStrategy.hpp"
#include "backtest/BacktestEngine.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <map>

using namespace tradingbot;
using json = nlohmann::json;

// Strategy factory function
std::unique_ptr<IStrategy> createStrategy(const std::string& name, const std::vector<double>& params) {
    if (name == "mean_reversion") {
        MeanReversionStrategy::Config config;
        config.z_score_threshold = params[0];
        config.window_size = static_cast<int>(params[1]);
        config.take_profit = params[2];
        config.stop_loss = params[3];
        return std::make_unique<MeanReversionStrategy>(config);
    }
    else if (name == "momentum") {
        MomentumStrategy::Config config;
        config.lookback_period = static_cast<int>(params[0]);
        config.threshold = params[1];
        config.take_profit = params[2];
        config.stop_loss = params[3];
        return std::make_unique<MomentumStrategy>(config);
    }
    else if (name == "bollinger_bands") {
        BollingerBandsStrategy::Config config;
        config.window_size = static_cast<int>(params[0]);
        config.num_std_dev = params[1];
        config.take_profit = params[2];
        config.stop_loss = params[3];
        return std::make_unique<BollingerBandsStrategy>(config);
    }
    else if (name == "rsi") {
        RSIStrategy::Config config;
        config.period = static_cast<int>(params[0]);
        config.overbought = params[1];
        config.oversold = params[2];
        config.take_profit = params[3];
        config.stop_loss = params[4];
        return std::make_unique<RSIStrategy>(config);
    }
    throw std::runtime_error("Unknown strategy: " + name);
}

// Parameter ranges for each strategy
std::map<std::string, std::vector<ParameterRange>> strategy_parameters = {
    {"mean_reversion", {
        {"z_score_threshold", 0.5, 3.0, false},
        {"window_size", 10, 100, true},
        {"take_profit", 0.5, 2.0, false},
        {"stop_loss", 0.1, 1.0, false}
    }},
    {"momentum", {
        {"lookback_period", 5, 50, true},
        {"threshold", 0.01, 0.1, false},
        {"take_profit", 0.5, 2.0, false},
        {"stop_loss", 0.1, 1.0, false}
    }},
    {"bollinger_bands", {
        {"window_size", 10, 100, true},
        {"num_std_dev", 1.0, 3.0, false},
        {"take_profit", 0.5, 2.0, false},
        {"stop_loss", 0.1, 1.0, false}
    }},
    {"rsi", {
        {"period", 5, 30, true},
        {"overbought", 60.0, 80.0, false},
        {"oversold", 20.0, 40.0, false},
        {"take_profit", 0.5, 2.0, false},
        {"stop_loss", 0.1, 1.0, false}
    }}
};

// Function to evaluate a strategy with given parameters
json evaluateStrategy(const std::string& strategy_name, const std::vector<double>& params) {
    auto strategy = createStrategy(strategy_name, params);
    BacktestEngine engine(std::move(strategy));
    engine.run();

    return {
        {"total_profit", engine.getTotalProfit()},
        {"sharpe_ratio", engine.getSharpeRatio()},
        {"max_drawdown", engine.getMaxDrawdown()},
        {"win_rate", engine.getWinRate()},
        {"sortino_ratio", engine.getSortinoRatio()},
        {"max_trade_duration", engine.getMaxTradeDuration()}
    };
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] <data_file>\n";
    std::cout << "Options:\n";
    std::cout << "  --strategy=<name>    Strategy to optimize (default: mean_reversion)\n";
    std::cout << "  --algorithm=<name>   Optimization algorithm (genetic or hill_climbing, default: genetic)\n";
    std::cout << "  --weights=<config>   Fitness weights configuration\n";
    std::cout << "  --threads=<n>        Number of threads (default: auto)\n";
    std::cout << "  --generations=<n>    Number of generations/iterations (default: 100)\n";
    std::cout << "  --population=<n>     Population size (default: 50)\n";
    std::cout << "\nAvailable strategies:\n";
    for (const auto& [name, _] : strategy_parameters) {
        std::cout << "  - " << name << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    // Default parameters
    std::string strategy_name = "mean_reversion";
    std::string algorithm = "genetic";
    FitnessWeights weights;
    int num_threads = 0;
    size_t generations = 100;
    size_t population_size = 50;
    std::string data_file;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.substr(0, 11) == "--strategy=") {
            strategy_name = arg.substr(11);
        }
        else if (arg.substr(0, 12) == "--algorithm=") {
            algorithm = arg.substr(12);
        }
        else if (arg.substr(0, 10) == "--weights=") {
            // Parse weights configuration
            std::string weights_str = arg.substr(10);
            // Format: profit:0.5,sharpe:0.4,drawdown:0.1
            size_t pos = 0;
            while ((pos = weights_str.find(',')) != std::string::npos) {
                std::string pair = weights_str.substr(0, pos);
                size_t colon = pair.find(':');
                if (colon != std::string::npos) {
                    std::string key = pair.substr(0, colon);
                    double value = std::stod(pair.substr(colon + 1));
                    if (key == "profit") weights.profit_weight = value;
                    else if (key == "sharpe") weights.sharpe_weight = value;
                    else if (key == "drawdown") weights.drawdown_weight = value;
                    else if (key == "win_rate") weights.win_rate_weight = value;
                    else if (key == "sortino") weights.sortino_weight = value;
                    else if (key == "max_trade_duration") weights.max_trade_duration_weight = value;
                }
                weights_str.erase(0, pos + 1);
            }
        }
        else if (arg.substr(0, 10) == "--threads=") {
            num_threads = std::stoi(arg.substr(10));
        }
        else if (arg.substr(0, 13) == "--generations=") {
            generations = std::stoul(arg.substr(13));
        }
        else if (arg.substr(0, 12) == "--population=") {
            population_size = std::stoul(arg.substr(12));
        }
        else {
            data_file = arg;
        }
    }

    if (data_file.empty()) {
        std::cout << "Error: Data file not specified\n";
        printUsage(argv[0]);
        return 1;
    }

    if (strategy_parameters.find(strategy_name) == strategy_parameters.end()) {
        std::cout << "Error: Unknown strategy '" << strategy_name << "'\n";
        printUsage(argv[0]);
        return 1;
    }

    if (algorithm != "genetic" && algorithm != "hill_climbing") {
        std::cout << "Error: Unknown algorithm '" << algorithm << "'\n";
        printUsage(argv[0]);
        return 1;
    }

    // Create evaluator
    auto evaluator = [strategy_name](const std::vector<double>& params) {
        return evaluateStrategy(strategy_name, params);
    };

    // Run optimization
    Individual best;
    if (algorithm == "genetic") {
        ParameterOptimizer optimizer(
            strategy_parameters[strategy_name],
            evaluator,
            weights,
            population_size,
            generations,
            0.1,  // mutation rate
            0.8,  // crossover rate
            num_threads
        );
        std::cout << "Starting genetic algorithm optimization for strategy: " << strategy_name << "\n";
        best = optimizer.optimize();
        std::string output_file = "optimization_results_genetic_" + strategy_name + ".json";
        optimizer.exportResults(output_file);
    }
    else {
        HillClimbingOptimizer optimizer(
            strategy_parameters[strategy_name],
            evaluator,
            weights,
            generations,  // iterations
            0.1,         // initial step size
            num_threads
        );
        std::cout << "Starting hill climbing optimization for strategy: " << strategy_name << "\n";
        best = optimizer.optimize();
        std::string output_file = "optimization_results_hill_climbing_" + strategy_name + ".json";
        optimizer.exportResults(output_file);
    }

    // Print best results
    std::cout << "\nBest configuration found:\n";
    for (size_t i = 0; i < best.parameters.size(); ++i) {
        std::cout << "- " << strategy_parameters[strategy_name][i].name 
                  << ": " << best.parameters[i] << "\n";
    }
    std::cout << "\nPerformance metrics:\n";
    for (const auto& [key, value] : best.metrics.items()) {
        std::cout << "- " << key << ": " << value << "\n";
    }

    return 0;
} 