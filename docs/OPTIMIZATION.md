# Optimization Module Documentation

## Overview

The optimization module provides algorithms for finding optimal strategy parameters. It supports both genetic algorithm and hill climbing approaches, with parallel evaluation capabilities.

## Core Components

### Parameter Ranges

```cpp
struct ParameterRange {
    std::string name;     // Parameter name
    double min;           // Minimum value
    double max;           // Maximum value
    bool is_integer;      // Whether parameter is integer
};
```

### Fitness Weights

```cpp
struct FitnessWeights {
    double profit_weight = 0.5;           // Weight for total profit
    double sharpe_weight = 0.4;           // Weight for Sharpe ratio
    double drawdown_weight = 0.1;         // Weight for maximum drawdown
    double win_rate_weight = 0.0;         // Weight for win rate
    double sortino_weight = 0.0;          // Weight for Sortino ratio
    double max_trade_duration_weight = 0.0; // Weight for maximum trade duration
};
```

### Individual

```cpp
struct Individual {
    std::vector<double> parameters;  // Strategy parameters
    double fitness;                  // Fitness score
    json metrics;                    // Performance metrics
};
```

## Genetic Algorithm

### Overview
The genetic algorithm implements a population-based optimization approach with tournament selection, crossover, and mutation.

### Implementation
```cpp
class ParameterOptimizer {
public:
    ParameterOptimizer(
        const std::vector<ParameterRange>& parameter_ranges,
        StrategyEvaluator evaluator,
        FitnessWeights weights,
        size_t population_size,
        size_t generations,
        double mutation_rate,
        double crossover_rate,
        int num_threads
    );

    Individual optimize();
    void setFitnessWeights(const FitnessWeights& weights);

private:
    std::vector<Individual> initializePopulation();
    Individual tournamentSelection(const std::vector<Individual>& population);
    Individual crossover(const Individual& parent1, const Individual& parent2);
    void mutate(Individual& individual);
    double calculateFitness(const json& metrics) const;
    void evaluatePopulation(std::vector<Individual>& population);
    
    std::vector<ParameterRange> parameter_ranges_;
    StrategyEvaluator evaluator_;
    FitnessWeights weights_;
    size_t population_size_;
    size_t generations_;
    double mutation_rate_;
    double crossover_rate_;
    int num_threads_;
};
```

### Algorithm Steps

1. **Initialization**
   - Create initial population
   - Evaluate each individual
   - Calculate fitness scores

2. **Selection**
   - Tournament selection for parents
   - Size of tournament: 2-4 individuals
   - Higher fitness individuals more likely to be selected

3. **Crossover**
   - Uniform crossover
   - Each parameter independently selected from parents
   - Probability controlled by crossover_rate

4. **Mutation**
   - Gaussian mutation for continuous parameters
   - Integer mutation for discrete parameters
   - Probability controlled by mutation_rate

5. **Evaluation**
   - Parallel evaluation of population
   - Calculate performance metrics
   - Update fitness scores

6. **Termination**
   - Maximum generations reached
   - Fitness plateau detected
   - Time limit exceeded

### Example Usage
```cpp
std::vector<ParameterRange> ranges{
    {"z_score_threshold", 1.0, 3.0, false},
    {"window_size", 10, 50, true},
    {"take_profit", 0.1, 1.0, false},
    {"stop_loss", 0.1, 0.5, false}
};

FitnessWeights weights{
    .profit_weight = 0.6,
    .sharpe_weight = 0.3,
    .drawdown_weight = 0.1
};

ParameterOptimizer optimizer(
    ranges,
    evaluator,
    weights,
    100,    // population_size
    50,     // generations
    0.1,    // mutation_rate
    0.8,    // crossover_rate
    4       // num_threads
);

auto result = optimizer.optimize();
```

## Hill Climbing

### Overview
The hill climbing optimizer implements a local search approach with random restarts and adaptive step sizes.

### Implementation
```cpp
class HillClimbingOptimizer {
public:
    HillClimbingOptimizer(
        const std::vector<ParameterRange>& parameter_ranges,
        StrategyEvaluator evaluator,
        FitnessWeights weights,
        size_t max_iterations,
        double step_size,
        int num_threads
    );

    Individual optimize();

private:
    Individual generateNeighbor(const Individual& current);
    void adjustStepSize();
    bool shouldRestart() const;
    
    std::vector<ParameterRange> parameter_ranges_;
    StrategyEvaluator evaluator_;
    FitnessWeights weights_;
    size_t max_iterations_;
    double step_size_;
    int num_threads_;
    size_t no_improvement_count_;
};
```

### Algorithm Steps

1. **Initialization**
   - Generate random starting point
   - Evaluate initial solution
   - Set step size

2. **Neighbor Generation**
   - Generate multiple neighbors in parallel
   - Adjust parameters within step size
   - Respect parameter bounds
   - Handle integer parameters

3. **Selection**
   - Evaluate all neighbors
   - Select best improvement
   - Update current solution

4. **Step Size Adjustment**
   - Increase on improvement
   - Decrease on no improvement
   - Maintain minimum/maximum bounds

5. **Restart**
   - Random restart on plateau
   - Keep track of best solution
   - Reset step size

6. **Termination**
   - Maximum iterations reached
   - No improvement for N iterations
   - Time limit exceeded

### Example Usage
```cpp
HillClimbingOptimizer optimizer(
    ranges,
    evaluator,
    weights,
    1000,   // max_iterations
    0.1,    // step_size
    4       // num_threads
);

auto result = optimizer.optimize();
```

## Parallel Processing

### OpenMP Integration

1. **Population Evaluation**
```cpp
#pragma omp parallel for
for (size_t i = 0; i < population_size_; ++i) {
    population_[i].metrics = evaluator_(population_[i].parameters);
    population_[i].fitness = calculateFitness(population_[i].metrics);
}
```

2. **Neighbor Evaluation**
```cpp
#pragma omp parallel for
for (int i = 0; i < num_threads_; ++i) {
    neighbors[i] = generateNeighbor(best_individual_);
    neighbors[i].metrics = evaluator_(neighbors[i].parameters);
    neighbors[i].fitness = calculateFitness(neighbors[i].metrics);
}
```

### Thread Safety

1. **Random Number Generation**
   - Thread-local random number generators
   - Different seeds for each thread
   - No shared state

2. **Data Access**
   - Read-only access to shared data
   - Thread-local storage for results
   - Synchronized updates

3. **Resource Management**
   - Pre-allocated memory
   - No dynamic allocation in parallel regions
   - Proper cleanup

## Results Export

### JSON Format
```json
{
    "best_individual": {
        "parameters": {
            "z_score_threshold": 2.1,
            "window_size": 25,
            "take_profit": 0.4,
            "stop_loss": 0.2
        },
        "fitness": 0.85,
        "metrics": {
            "total_profit": 1500.0,
            "sharpe_ratio": 1.8,
            "max_drawdown": 0.15,
            "win_rate": 0.65,
            "sortino_ratio": 2.1,
            "max_trade_duration": 5
        }
    },
    "weights": {
        "profit": 0.6,
        "sharpe": 0.3,
        "drawdown": 0.1,
        "win_rate": 0.0,
        "sortino": 0.0,
        "max_trade_duration": 0.0
    },
    "optimization_parameters": {
        "population_size": 100,
        "generations": 50,
        "mutation_rate": 0.1,
        "crossover_rate": 0.8,
        "num_threads": 4
    }
}
```

## Best Practices

1. **Parameter Selection**
   - Choose appropriate ranges
   - Consider parameter interactions
   - Balance exploration/exploitation
   - Monitor parameter distributions

2. **Fitness Function**
   - Weight metrics appropriately
   - Consider risk-adjusted returns
   - Include transaction costs
   - Account for market conditions

3. **Performance**
   - Use parallel processing
   - Optimize evaluation function
   - Cache results when possible
   - Monitor convergence

4. **Validation**
   - Cross-validate results
   - Test on out-of-sample data
   - Check for overfitting
   - Monitor stability 