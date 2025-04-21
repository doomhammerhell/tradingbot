#include "../../include/risk/BasicRiskManager.hpp"
#include <cmath>
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

namespace tradingbot {
namespace risk {

BasicRiskManager::BasicRiskManager()
    : maxPositionSize_(0.1),      // 10% do capital
      maxRiskPerTrade_(0.02),     // 2% do capital por operação
      maxDailyLoss_(0.05),        // 5% do capital por dia
      stopLossPercentage_(0.02),  // 2% do preço de entrada
      dailyPnL_(0.0),
      totalPnL_(0.0),
      totalTrades_(0),
      rejectedTrades_(0) {
    
    auto& logger = utils::Logger::getInstance();
    logger.info("BasicRiskManager created");
}

void BasicRiskManager::initialize(const std::string& config) {
    auto& logger = utils::Logger::getInstance();
    logger.info("Initializing BasicRiskManager...");
    
    try {
        json configJson = json::parse(config);
        
        if (configJson.contains("max_position_size")) {
            maxPositionSize_ = configJson["max_position_size"].get<double>();
        }
        
        if (configJson.contains("max_risk_per_trade")) {
            maxRiskPerTrade_ = configJson["max_risk_per_trade"].get<double>();
        }
        
        if (configJson.contains("max_daily_loss")) {
            maxDailyLoss_ = configJson["max_daily_loss"].get<double>();
        }
        
        if (configJson.contains("stop_loss_percentage")) {
            stopLossPercentage_ = configJson["stop_loss_percentage"].get<double>();
        }
        
        logger.info("BasicRiskManager initialized with: max_position_size={}, max_risk_per_trade={}, max_daily_loss={}, stop_loss_percentage={}",
                   maxPositionSize_, maxRiskPerTrade_, maxDailyLoss_, stopLossPercentage_);
    } catch (const json::exception& e) {
        logger.error("Failed to parse config: {}", e.what());
        throw std::runtime_error("Invalid configuration");
    }
}

bool BasicRiskManager::shouldExecute(const execution::Order& order, const Portfolio& portfolio) {
    auto& logger = utils::Logger::getInstance();
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verifica se o tamanho da posição excede o máximo permitido
    double positionValue = order.price * order.quantity;
    if (positionValue > portfolio.totalBalance * maxPositionSize_) {
        logger.warn("Position size exceeds maximum allowed: {} > {}", 
                   positionValue, portfolio.totalBalance * maxPositionSize_);
        rejectedTrades_++;
        return false;
    }
    
    // Verifica se o risco por trade excede o máximo permitido
    double riskAmount = positionValue * stopLossPercentage_;
    if (riskAmount > portfolio.totalBalance * maxRiskPerTrade_) {
        logger.warn("Risk per trade exceeds maximum allowed: {} > {}", 
                   riskAmount, portfolio.totalBalance * maxRiskPerTrade_);
        rejectedTrades_++;
        return false;
    }
    
    // Verifica se a perda diária excede o máximo permitido
    if (dailyPnL_ < -portfolio.totalBalance * maxDailyLoss_) {
        logger.warn("Daily loss limit reached: {} < {}", 
                   dailyPnL_, -portfolio.totalBalance * maxDailyLoss_);
        rejectedTrades_++;
        return false;
    }
    
    // Verifica se há saldo disponível
    if (positionValue > portfolio.availableBalance) {
        logger.warn("Insufficient balance: {} > {}", 
                   positionValue, portfolio.availableBalance);
        rejectedTrades_++;
        return false;
    }
    
    return true;
}

double BasicRiskManager::calculatePositionSize(const execution::Order& order, const Portfolio& portfolio) {
    // Calcula o tamanho da posição com base no risco máximo por trade
    double maxRiskAmount = portfolio.totalBalance * maxRiskPerTrade_;
    double stopLossAmount = order.price * stopLossPercentage_;
    double positionSize = maxRiskAmount / stopLossAmount;
    
    // Limita pelo tamanho máximo da posição
    double maxPositionValue = portfolio.totalBalance * maxPositionSize_;
    positionSize = std::min(positionSize, maxPositionValue / order.price);
    
    // Limita pelo saldo disponível
    positionSize = std::min(positionSize, portfolio.availableBalance / order.price);
    
    return positionSize;
}

void BasicRiskManager::updateAfterExecution(const execution::Order& order, bool success) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (success) {
        totalTrades_++;
        
        if (order.side == execution::Order::Side::BUY) {
            entryPrices_[order.symbol] = order.price;
        } else {
            // Calcula PnL para ordens de venda
            auto it = entryPrices_.find(order.symbol);
            if (it != entryPrices_.end()) {
                double entryPrice = it->second;
                double pnl = (order.price - entryPrice) * order.quantity;
                dailyPnL_ += pnl;
                totalPnL_ += pnl;
                entryPrices_.erase(it);
            }
        }
    }
}

std::string BasicRiskManager::getMetrics() const {
    json metrics;
    
    metrics["daily_pnl"] = dailyPnL_;
    metrics["total_pnl"] = totalPnL_;
    metrics["total_trades"] = totalTrades_;
    metrics["rejected_trades"] = rejectedTrades_;
    metrics["active_positions"] = entryPrices_.size();
    
    return metrics.dump(4);
}

} // namespace risk
} // namespace tradingbot 