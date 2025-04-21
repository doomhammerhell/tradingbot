#include "optimization/ParameterOptimizer.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace tradingbot {

ParameterOptimizer::ParameterOptimizer(
    const std::vector<ParameterRange>& parameter_ranges,
    StrategyEvaluator evaluator,
    FitnessWeights weights,
    size_t population_size,
    size_t generations,
    double mutation_rate,
    double crossover_rate,
    int num_threads
) : parameter_ranges_(parameter_ranges),
    evaluator_(evaluator),
    weights_(weights),
    population_size_(population_size),
    generations_(generations),
    mutation_rate_(mutation_rate),
    crossover_rate_(crossover_rate),
    num_threads_(num_threads),
    rng_(std::random_device{}()) {
    
    if (num_threads_ == 0) {
        num_threads_ = omp_get_max_threads();
    }
    omp_set_num_threads(num_threads_);
}

void ParameterOptimizer::setFitnessWeights(const FitnessWeights& weights) {
    weights_ = weights;
}

Individual ParameterOptimizer::optimize() {
    initializePopulation();
    evaluatePopulation();

    for (size_t gen = 0; gen < generations_; ++gen) {
        createNewGeneration();
        evaluatePopulation();

        // Log progress
        const auto& best = getBestIndividual();
        std::cout << "Generation " << gen + 1 << "/" << generations_ << "\n";
        std::cout << "Best fitness: " << best.fitness << "\n";
        std::cout << "Parameters: ";
        for (size_t i = 0; i < best.parameters.size(); ++i) {
            std::cout << parameter_ranges_[i].name << "=" << best.parameters[i] << " ";
        }
        std::cout << "\nMetrics:\n";
        for (const auto& [key, value] : best.metrics.items()) {
            std::cout << "- " << key << ": " << value << "\n";
        }
        std::cout << "\n";
    }

    return getBestIndividual();
}

Individual ParameterOptimizer::getBestIndividual() const {
    return *std::max_element(population_.begin(), population_.end(),
        [](const Individual& a, const Individual& b) {
            return a.fitness < b.fitness;
        });
}

void ParameterOptimizer::exportResults(const std::string& filename) const {
    json results;
    results["best_individual"] = {
        {"parameters", getBestIndividual().parameters},
        {"fitness", getBestIndividual().fitness},
        {"metrics", getBestIndividual().metrics}
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

void ParameterOptimizer::initializePopulation() {
    population_.resize(population_size_);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    #pragma omp parallel for
    for (size_t i = 0; i < population_size_; ++i) {
        population_[i].parameters.resize(parameter_ranges_.size());
        for (size_t j = 0; j < parameter_ranges_.size(); ++j) {
            const auto& range = parameter_ranges_[j];
            double value = range.min + dist(rng_) * (range.max - range.min);
            if (range.is_integer) {
                value = std::round(value);
            }
            population_[i].parameters[j] = value;
        }
    }
}

void ParameterOptimizer::evaluatePopulation() {
    #pragma omp parallel for
    for (size_t i = 0; i < population_size_; ++i) {
        population_[i].metrics = evaluator_(population_[i].parameters);
        population_[i].fitness = calculateFitness(population_[i].metrics);
    }
}

Individual ParameterOptimizer::selectParent() {
    // Tournament selection
    const size_t tournament_size = 3;
    std::uniform_int_distribution<size_t> dist(0, population_size_ - 1);
    
    Individual best = population_[dist(rng_)];
    for (size_t i = 1; i < tournament_size; ++i) {
        const Individual& candidate = population_[dist(rng_)];
        if (candidate.fitness > best.fitness) {
            best = candidate;
        }
    }
    return best;
}

Individual ParameterOptimizer::crossover(const Individual& parent1, const Individual& parent2) {
    Individual child;
    child.parameters.resize(parameter_ranges_.size());
    
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (size_t i = 0; i < parameter_ranges_.size(); ++i) {
        if (dist(rng_) < crossover_rate_) {
            child.parameters[i] = parent1.parameters[i];
        } else {
            child.parameters[i] = parent2.parameters[i];
        }
    }
    
    return child;
}

void ParameterOptimizer::mutate(Individual& individual) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (size_t i = 0; i < parameter_ranges_.size(); ++i) {
        if (dist(rng_) < mutation_rate_) {
            const auto& range = parameter_ranges_[i];
            double value = range.min + dist(rng_) * (range.max - range.min);
            if (range.is_integer) {
                value = std::round(value);
            }
            individual.parameters[i] = value;
        }
    }
}

void ParameterOptimizer::createNewGeneration() {
    std::vector<Individual> new_population;
    new_population.reserve(population_size_);
    
    // Keep the best individual (elitism)
    new_population.push_back(getBestIndividual());
    
    // Create rest of the new population
    while (new_population.size() < population_size_) {
        Individual parent1 = selectParent();
        Individual parent2 = selectParent();
        Individual child = crossover(parent1, parent2);
        mutate(child);
        new_population.push_back(child);
    }
    
    population_ = std::move(new_population);
}

double ParameterOptimizer::calculateFitness(const json& metrics) const {
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