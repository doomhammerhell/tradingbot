#pragma once

#include <vector>
#include <string>
#include <functional>
#include <random>
#include <memory>
#include <nlohmann/json.hpp>
#include "data/MarketData.hpp"
#include "strategies/IStrategy.hpp"
#include <omp.h>

namespace tradingbot {

using json = nlohmann::json;

/**
 * @brief Structure representing a parameter range for optimization
 */
struct ParameterRange {
    std::string name;
    double min;
    double max;
    bool is_integer;
};

/**
 * @brief Structure representing fitness weights for optimization
 */
struct FitnessWeights {
    double profit_weight = 0.5;
    double sharpe_weight = 0.4;
    double drawdown_weight = 0.1;
    double win_rate_weight = 0.0;
    double sortino_weight = 0.0;
    double max_trade_duration_weight = 0.0;
};

/**
 * @brief Structure representing an individual in the genetic algorithm
 */
struct Individual {
    std::vector<double> parameters;
    double fitness;
    json metrics;
};

/**
 * @brief Callback type for strategy evaluation
 */
using StrategyEvaluator = std::function<json(const std::vector<double>&)>;

/**
 * @brief Class implementing parameter optimization using genetic algorithm
 */
class ParameterOptimizer {
public:
    /**
     * @brief Constructor for ParameterOptimizer
     * @param parameter_ranges List of parameter ranges to optimize
     * @param evaluator Function to evaluate strategy performance
     * @param weights Weights for fitness calculation
     * @param population_size Size of the genetic algorithm population
     * @param generations Number of generations to run
     * @param mutation_rate Probability of mutation
     * @param crossover_rate Probability of crossover
     * @param num_threads Number of threads for parallel evaluation (0 for auto)
     */
    ParameterOptimizer(
        const std::vector<ParameterRange>& parameter_ranges,
        StrategyEvaluator evaluator,
        FitnessWeights weights = FitnessWeights{},
        size_t population_size = 50,
        size_t generations = 100,
        double mutation_rate = 0.1,
        double crossover_rate = 0.8,
        int num_threads = 0
    );

    /**
     * @brief Run the optimization process
     * @return Best individual found
     */
    Individual optimize();

    /**
     * @brief Get the best individual from the current population
     * @return Best individual
     */
    Individual getBestIndividual() const;

    /**
     * @brief Export optimization results to JSON
     * @param filename Output file path
     */
    void exportResults(const std::string& filename) const;

    /**
     * @brief Set new fitness weights
     * @param weights New fitness weights
     */
    void setFitnessWeights(const FitnessWeights& weights);

private:
    std::vector<ParameterRange> parameter_ranges_;
    StrategyEvaluator evaluator_;
    FitnessWeights weights_;
    size_t population_size_;
    size_t generations_;
    double mutation_rate_;
    double crossover_rate_;
    int num_threads_;
    std::vector<Individual> population_;
    std::mt19937 rng_;

    /**
     * @brief Initialize the population with random individuals
     */
    void initializePopulation();

    /**
     * @brief Evaluate fitness of all individuals in the population
     */
    void evaluatePopulation();

    /**
     * @brief Select parents for the next generation using tournament selection
     * @return Selected parent
     */
    Individual selectParent();

    /**
     * @brief Perform crossover between two parents
     * @param parent1 First parent
     * @param parent2 Second parent
     * @return Child individual
     */
    Individual crossover(const Individual& parent1, const Individual& parent2);

    /**
     * @brief Mutate an individual's parameters
     * @param individual Individual to mutate
     */
    void mutate(Individual& individual);

    /**
     * @brief Create a new generation using selection, crossover, and mutation
     */
    void createNewGeneration();

    /**
     * @brief Calculate fitness score based on strategy metrics
     * @param metrics Strategy performance metrics
     * @return Fitness score
     */
    double calculateFitness(const json& metrics) const;
};

} // namespace tradingbot 