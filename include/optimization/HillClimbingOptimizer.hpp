#pragma once

#include "ParameterOptimizer.hpp"
#include <vector>
#include <random>
#include <omp.h>

namespace tradingbot {

class HillClimbingOptimizer {
public:
    HillClimbingOptimizer(
        const std::vector<ParameterRange>& parameter_ranges,
        StrategyEvaluator evaluator,
        FitnessWeights weights = FitnessWeights{},
        size_t max_iterations = 1000,
        double step_size = 0.1,
        int num_threads = 0
    );

    Individual optimize();
    void exportResults(const std::string& filename) const;

private:
    std::vector<ParameterRange> parameter_ranges_;
    StrategyEvaluator evaluator_;
    FitnessWeights weights_;
    size_t max_iterations_;
    double step_size_;
    int num_threads_;
    std::mt19937 rng_;
    Individual best_individual_;

    Individual generateRandomIndividual() const;
    Individual generateNeighbor(const Individual& current) const;
    double calculateFitness(const json& metrics) const;
};

} // namespace tradingbot 