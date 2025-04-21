#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "optimization/ParameterOptimizer.hpp"
#include <nlohmann/json.hpp>

using namespace tradingbot;
using json = nlohmann::json;

// Mock evaluator function for testing
json mockEvaluator(const std::vector<double>& params) {
    // Simple test function that returns metrics based on parameters
    double fitness = 0.0;
    for (double param : params) {
        fitness += param;
    }
    
    return {
        {"total_profit", fitness * 100},
        {"sharpe_ratio", fitness * 0.5},
        {"max_drawdown", -fitness * 0.1},
        {"win_rate", fitness * 0.2}
    };
}

TEST_CASE("ParameterOptimizer initialization", "[optimization]") {
    std::vector<ParameterRange> ranges = {
        {"param1", 0.0, 1.0, false},
        {"param2", 0.0, 1.0, false}
    };
    
    ParameterOptimizer optimizer(ranges, mockEvaluator, 10, 5);
    
    SECTION("Population size is correct") {
        REQUIRE(optimizer.getBestIndividual().parameters.size() == 2);
    }
}

TEST_CASE("ParameterOptimizer optimization", "[optimization]") {
    std::vector<ParameterRange> ranges = {
        {"param1", 0.0, 1.0, false},
        {"param2", 0.0, 1.0, false}
    };
    
    ParameterOptimizer optimizer(ranges, mockEvaluator, 10, 5);
    
    SECTION("Optimization improves fitness") {
        Individual best = optimizer.optimize();
        REQUIRE(best.fitness > 0.0);
        
        // Check that parameters are within bounds
        for (size_t i = 0; i < best.parameters.size(); ++i) {
            REQUIRE(best.parameters[i] >= ranges[i].min);
            REQUIRE(best.parameters[i] <= ranges[i].max);
        }
    }
}

TEST_CASE("ParameterOptimizer integer parameters", "[optimization]") {
    std::vector<ParameterRange> ranges = {
        {"param1", 0.0, 1.0, false},
        {"param2", 1, 10, true}  // Integer parameter
    };
    
    ParameterOptimizer optimizer(ranges, mockEvaluator, 10, 5);
    Individual best = optimizer.optimize();
    
    SECTION("Integer parameters are rounded") {
        REQUIRE(std::round(best.parameters[1]) == best.parameters[1]);
    }
}

TEST_CASE("ParameterOptimizer export results", "[optimization]") {
    std::vector<ParameterRange> ranges = {
        {"param1", 0.0, 1.0, false},
        {"param2", 0.0, 1.0, false}
    };
    
    ParameterOptimizer optimizer(ranges, mockEvaluator, 10, 5);
    optimizer.optimize();
    
    SECTION("Results can be exported") {
        REQUIRE_NOTHROW(optimizer.exportResults("test_results.json"));
    }
}

TEST_CASE("ParameterOptimizer fitness calculation", "[optimization]") {
    std::vector<ParameterRange> ranges = {
        {"param1", 0.0, 1.0, false},
        {"param2", 0.0, 1.0, false}
    };
    
    ParameterOptimizer optimizer(ranges, mockEvaluator, 10, 5);
    
    SECTION("Fitness is calculated correctly") {
        json metrics = {
            {"total_profit", 100.0},
            {"sharpe_ratio", 1.5},
            {"max_drawdown", -0.1}
        };
        
        double fitness = optimizer.calculateFitness(metrics);
        REQUIRE(fitness > 0.0);
    }
} 