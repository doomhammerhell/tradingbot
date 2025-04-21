#include "strategies/MLStrategy.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

namespace tradingbot {
namespace strategies {

MLStrategy::MLStrategy(const nlohmann::json& config) {
    loadModelConfig(config);
    positionSize_ = config.value("position_size", 1.0);
    stopLoss_ = config.value("stop_loss", 0.02);
    takeProfit_ = config.value("take_profit", 0.03);
    
    reset();
}

void MLStrategy::initialize() {
    spdlog::info("Initializing MLStrategy with model type: {}", modelConfig_.modelType);
}

void MLStrategy::update(const MarketData& data) {
    if (!featureEngine_) {
        spdlog::error("FeatureEngine not set for MLStrategy");
        return;
    }
    
    featureEngine_->update(data);
    updatePosition(data);
}

Signal MLStrategy::generateSignal() {
    if (!featureEngine_) {
        return Signal{SignalType::HOLD, 0.0};
    }
    
    if (isPositionOpen_) {
        return Signal{SignalType::HOLD, positionSize_};
    }
    
    const auto& features = featureEngine_->getFeatures();
    lastPrediction_ = predictSignal(features);
    lastConfidence_ = std::abs(lastPrediction_);
    
    if (lastConfidence_ < modelConfig_.confidenceThreshold) {
        return Signal{SignalType::HOLD, 0.0};
    }
    
    if (lastPrediction_ > 0) {
        return Signal{SignalType::BUY, positionSize_};
    } else if (lastPrediction_ < 0) {
        return Signal{SignalType::SELL, positionSize_};
    }
    
    return Signal{SignalType::HOLD, 0.0};
}

void MLStrategy::reset() {
    isPositionOpen_ = false;
    entryPrice_ = 0.0;
    lastPrediction_ = 0.0;
    lastConfidence_ = 0.0;
}

nlohmann::json MLStrategy::getStatus() const {
    return {
        {"strategy", "ml"},
        {"model_type", modelConfig_.modelType},
        {"position_size", positionSize_},
        {"last_prediction", lastPrediction_},
        {"last_confidence", lastConfidence_},
        {"is_position_open", isPositionOpen_},
        {"entry_price", entryPrice_}
    };
}

void MLStrategy::loadModel(const std::string& modelPath) {
    std::ifstream file(modelPath);
    if (!file.is_open()) {
        spdlog::error("Failed to open model file: {}", modelPath);
        return;
    }
    
    nlohmann::json modelConfig;
    file >> modelConfig;
    
    loadModelConfig(modelConfig);
    spdlog::info("Loaded ML model from: {}", modelPath);
}

void MLStrategy::setFeatureEngine(std::shared_ptr<FeatureEngine> featureEngine) {
    featureEngine_ = featureEngine;
}

void MLStrategy::loadModelConfig(const nlohmann::json& config) {
    modelConfig_.modelType = config.value("model_type", "decision_tree");
    modelConfig_.confidenceThreshold = config.value("confidence_threshold", 0.7);
    
    if (config.contains("thresholds")) {
        for (const auto& [key, value] : config["thresholds"].items()) {
            modelConfig_.thresholds[key] = value;
        }
    }
    
    if (config.contains("features")) {
        modelConfig_.features = config["features"].get<std::vector<std::string>>();
    }
}

double MLStrategy::predictSignal(const MarketData& data) {
    // This is a simplified prediction function
    // In a real implementation, you would use the actual ML model
    // For now, we'll use a simple rule-based approach based on thresholds
    
    const auto& features = featureEngine_->getFeatures();
    double prediction = 0.0;
    
    for (const auto& [feature, threshold] : modelConfig_.thresholds) {
        if (features.count(feature)) {
            double value = features.at(feature);
            if (value > threshold) {
                prediction += 1.0;
            } else if (value < -threshold) {
                prediction -= 1.0;
            }
        }
    }
    
    return prediction;
}

void MLStrategy::updatePosition(const MarketData& data) {
    if (!isPositionOpen_) {
        return;
    }
    
    if (shouldClosePosition(data.price)) {
        isPositionOpen_ = false;
        entryPrice_ = 0.0;
    }
}

bool MLStrategy::shouldClosePosition(double currentPrice) const {
    if (!isPositionOpen_) {
        return false;
    }
    
    double priceChange = std::abs(currentPrice - entryPrice_) / entryPrice_;
    return priceChange >= stopLoss_ || priceChange >= takeProfit_;
}

} // namespace strategies
} // namespace tradingbot 