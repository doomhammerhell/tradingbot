#pragma once

#include "core/IRiskManager.hpp"
#include <string>
#include <map>
#include <mutex>
#include <atomic>

namespace tradingbot {
namespace risk {

class BasicRiskManager : public core::IRiskManager {
public:
    BasicRiskManager();
    ~BasicRiskManager() override = default;

    void initialize(const std::string& config) override;
    bool shouldExecute(const core::Order& order, const core::Portfolio& portfolio) override;
    double calculatePositionSize(const core::Order& order, const core::Portfolio& portfolio) override;
    void updateAfterExecution(const core::Order& order, bool success) override;
    std::string getMetrics() const override;
    bool validateOrder(const core::Order& order) override;
    void updateMetrics(const core::Order& executedOrder) override;
    bool checkRiskLimits() override;
    RiskParameters getCurrentRiskMetrics() const override;
    double getCurrentDrawdown() const override;
    double getDailyPnL() const override;
    void resetDailyMetrics() override;

private:
    // Parâmetros de risco
    double maxPositionSize_;        // Tamanho máximo da posição em percentual do capital
    double maxRiskPerTrade_;        // Risco máximo por operação em percentual do capital
    double maxDailyLoss_;           // Perda máxima diária em percentual do capital
    double stopLossPercentage_;     // Stop loss em percentual do preço de entrada
    
    // Métricas
    std::atomic<double> dailyPnL_;
    std::atomic<double> totalPnL_;
    std::atomic<int> totalTrades_;
    std::atomic<int> rejectedTrades_;

    RiskParameters params_;
    double currentBalance_;
    double maxBalance_;
    double maxDailyPnL_;
    double minDailyPnL_;

    std::mutex mutex_;
    std::map<std::string, double> entryPrices_;  // symbol -> price

    bool checkPositionSize(const core::Order& order) const;
    bool checkDailyLoss() const;
    bool checkDrawdown() const;
};

} // namespace risk
} // namespace tradingbot 