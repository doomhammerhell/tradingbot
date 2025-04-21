#include "optimization/HillClimbingOptimizer.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace tradingbot {

HillClimbingOptimizer::HillClimbingOptimizer(
    const std::vector<ParameterRange>& parameter_ranges,
    StrategyEvaluator evaluator,
    FitnessWeights weights,
    size_t max_iterations,
    double step_size,
    int num_threads
) : parameter_ranges_(parameter_ranges),
    evaluator_(evaluator),
    weights_(weights),
    max_iterations_(max_iterations),
    step_size_(step_size),
    num_threads_(num_threads),
    rng_(std::random_device{}()) {
    
    if (num_threads_ == 0) {
        num_threads_ = omp_get_max_threads();
    }
    omp_set_num_threads(num_threads_);
}

Individual HillClimbingOptimizer::optimize() {
    // Generate random initial solution
    best_individual_ = generateRandomIndividual();
    best_individual_.metrics = evaluator_(best_individual_.parameters);
    best_individual_.fitness = calculateFitness(best_individual_.metrics);

    std::cout << "Starting hill climbing optimization...\n";
    std::cout << "Initial fitness: " << best_individual_.fitness << "\n";

    // Main optimization loop
    for (size_t iter = 0; iter < max_iterations_; ++iter) {
        // Generate and evaluate neighbors in parallel
        std::vector<Individual> neighbors(num_threads_);
        #pragma omp parallel for
        for (int i = 0; i < num_threads_; ++i) {
            neighbors[i] = generateNeighbor(best_individual_);
            neighbors[i].metrics = evaluator_(neighbors[i].parameters);
            neighbors[i].fitness = calculateFitness(neighbors[i].metrics);
        }

        // Find best neighbor
        auto best_neighbor = std::max_element(neighbors.begin(), neighbors.end(),
            [](const Individual& a, const Individual& b) {
                return a.fitness < b.fitness;
            });

        // Update best solution if improvement found
        if (best_neighbor->fitness > best_individual_.fitness) {
            best_individual_ = *best_neighbor;
            std::cout << "Iteration " << iter + 1 << "/" << max_iterations_ 
                      << " - New best fitness: " << best_individual_.fitness << "\n";
        }

        // Reduce step size over time
        step_size_ *= 0.99;
    }

    return best_individual_;
}

void HillClimbingOptimizer::exportResults(const std::string& filename) const {
    json results;
    results["best_individual"] = {
        {"parameters", best_individual_.parameters},
        {"fitness", best_individual_.fitness},
        {"metrics", best_individual_.metrics}
    };
    results["weights"] = {
        {"profit", weights_.profit_weight},
        {"sharpe", weights_.sharpe_weight},
        {"drawdown", weights_.drawdown_weight},
        {"win_rate", weights_.win_rate_weight},
        {"sortino", weights_.sortino_weight},
        {"max_trade_duration", weights_.max_trade_duration_weight}
    };

    std::ofstream file(filename);
    file << std::setw(4) << results << std::endl;
}

Individual HillClimbingOptimizer::generateRandomIndividual() const {
    Individual individual;
    individual.parameters.resize(parameter_ranges_.size());
    
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (size_t i = 0; i < parameter_ranges_.size(); ++i) {
        const auto& range = parameter_ranges_[i];
        double value = range.min + dist(rng_) * (range.max - range.min);
        if (range.is_integer) {
            value = std::round(value);
        }
        individual.parameters[i] = value;
    }
    
    return individual;
}

Individual HillClimbingOptimizer::generateNeighbor(const Individual& current) const {
    Individual neighbor = current;
    
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::uniform_int_distribution<size_t> param_dist(0, parameter_ranges_.size() - 1);
    
    // Select random parameter to modify
    size_t param_idx = param_dist(rng_);
    const auto& range = parameter_ranges_[param_idx];
    
    // Generate random step
    double step = dist(rng_) * step_size_ * (range.max - range.min);
    double new_value = current.parameters[param_idx] + step;
    
    // Ensure value stays within bounds
    new_value = std::max(range.min, std::min(range.max, new_value));
    if (range.is_integer) {
        new_value = std::round(new_value);
    }
    
    neighbor.parameters[param_idx] = new_value;
    return neighbor;
}

double HillClimbingOptimizer::calculateFitness(const json& metrics) const {
    double fitness = 0.0;
    
    if (weights_.profit_weight > 0) {
        fitness += weights_.profit_weight * metrics["total_profit"].get<double>();
    }
    
    if (weights_.sharpe_weight > 0) {
        fitness += weights_.sharpe_weight * metrics["sharpe_ratio"].get<double>();
    }
    
    if (weights_.drawdown_weight > 0) {
        fitness -= weights_.drawdown_weight * std::abs(metrics["max_drawdown"].get<double>());
    }
    
    if (weights_.win_rate_weight > 0) {
        fitness += weights_.win_rate_weight * metrics["win_rate"].get<double>();
    }
    
    if (weights_.sortino_weight > 0) {
        fitness += weights_.sortino_weight * metrics["sortino_ratio"].get<double>();
    }
    
    if (weights_.max_trade_duration_weight > 0) {
        fitness -= weights_.max_trade_duration_weight * metrics["max_trade_duration"].get<double>();
    }
    
    return fitness;
}

} // namespace tradingbot 