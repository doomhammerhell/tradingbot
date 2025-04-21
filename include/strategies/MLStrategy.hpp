#pragma once

#include "strategies/IStrategy.hpp"
#include "core/FeatureEngine.hpp"
#include "core/Logger.hpp"
#include <vector>
#include <string>
#include <map>
#include <memory>

namespace tradingbot {
namespace strategies {

struct MLModelConfig {
    std::string modelType;
    std::map<std::string, double> thresholds;
    std::vector<std::string> features;
    double confidenceThreshold;
};

class MLStrategy : public IStrategy {
public:
    MLStrategy(const nlohmann::json& config);
    ~MLStrategy() override = default;

    // IStrategy interface implementation
    void initialize() override;
    void update(const MarketData& data) override;
    Signal generateSignal() override;
    void reset() override;
    nlohmann::json getStatus() const override;

    // ML-specific methods
    void loadModel(const std::string& modelPath);
    void setFeatureEngine(std::shared_ptr<FeatureEngine> featureEngine);

private:
    // Configuration
    MLModelConfig modelConfig_;
    double positionSize_;
    double stopLoss_;
    double takeProfit_;

    // State variables
    std::shared_ptr<FeatureEngine> featureEngine_;
    bool isPositionOpen_;
    double entryPrice_;
    double lastPrediction_;
    double lastConfidence_;

    // Helper methods
    void loadModelConfig(const nlohmann::json& config);
    double predictSignal(const MarketData& data);
    void updatePosition(const MarketData& data);
    bool shouldClosePosition(double currentPrice) const;
};

} // namespace strategies
} // namespace tradingbot 