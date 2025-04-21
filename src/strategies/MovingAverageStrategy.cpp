#include "strategies/MovingAverageStrategy.hpp"
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <cmath>

using json = nlohmann::json;

namespace tradingbot {
namespace strategies {

MovingAverageStrategy::MovingAverageStrategy()
    : period_(20),
      positionSize_(0.1),
      lastPrice_(0.0),
      lastMA_(0.0),
      totalTrades_(0),
      successfulTrades_(0),
      failedTrades_(0),
      totalPnL_(0.0) {
}

void MovingAverageStrategy::initialize(const std::string& config) {
    auto& logger = utils::Logger::getInstance();
    logger.info("Initializing MovingAverageStrategy...");
    
    try {
        json configJson = json::parse(config);
        
        if (configJson.contains("period")) {
            period_ = configJson["period"].get<int>();
        }
        
        if (configJson.contains("position_size")) {
            positionSize_ = configJson["position_size"].get<double>();
        }
        
        logger.info("MovingAverageStrategy initialized with period={}, position_size={}",
                   period_, positionSize_);
    } catch (const json::exception& e) {
        logger.error("Failed to parse strategy config: {}", e.what());
        throw std::runtime_error("Invalid strategy configuration");
    }
}

core::Decision MovingAverageStrategy::makeDecision(const core::MarketData& data) {
    auto& logger = utils::Logger::getInstance();
    
    // Atualiza histórico de preços
    priceHistory_.push_back(data.price);
    if (priceHistory_.size() > period_) {
        priceHistory_.pop_front();
    }
    
    // Se não temos dados suficientes, aguarda
    if (priceHistory_.size() < period_) {
        return {core::Decision::Type::HOLD, 0.0, 0.0, "Waiting for more data"};
    }
    
    // Calcula média móvel
    double currentMA = calculateSMA();
    lastMA_ = currentMA;
    lastPrice_ = data.price;
    
    // Verifica cruzamentos
    if (hasCrossedAboveMA(data.price)) {
        logger.info("Price crossed above MA: price={}, MA={}", data.price, currentMA);
        return {
            core::Decision::Type::BUY,
            positionSize_,
            data.price,
            "Price crossed above MA"
        };
    } else if (hasCrossedBelowMA(data.price)) {
        logger.info("Price crossed below MA: price={}, MA={}", data.price, currentMA);
        return {
            core::Decision::Type::SELL,
            positionSize_,
            data.price,
            "Price crossed below MA"
        };
    }
    
    return {core::Decision::Type::HOLD, 0.0, 0.0, "No signal"};
}

void MovingAverageStrategy::updateOrderStatus(const core::Decision& decision, bool success) {
    totalTrades_++;
    if (success) {
        successfulTrades_++;
        // TODO: Calcular PnL real quando tivermos dados de execução
    } else {
        failedTrades_++;
    }
}

std::string MovingAverageStrategy::getMetrics() const {
    json metrics;
    metrics["period"] = period_;
    metrics["position_size"] = positionSize_;
    metrics["total_trades"] = totalTrades_;
    metrics["successful_trades"] = successfulTrades_;
    metrics["failed_trades"] = failedTrades_;
    metrics["total_pnl"] = totalPnL_;
    metrics["win_rate"] = totalTrades_ > 0 ? 
        static_cast<double>(successfulTrades_) / totalTrades_ : 0.0;
    
    return metrics.dump(4);
}

double MovingAverageStrategy::calculateSMA() const {
    if (priceHistory_.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double price : priceHistory_) {
        sum += price;
    }
    
    return sum / priceHistory_.size();
}

bool MovingAverageStrategy::hasCrossedAboveMA(double currentPrice) const {
    return lastPrice_ <= lastMA_ && currentPrice > lastMA_;
}

bool MovingAverageStrategy::hasCrossedBelowMA(double currentPrice) const {
    return lastPrice_ >= lastMA_ && currentPrice < lastMA_;
}

} // namespace strategies
} // namespace tradingbot 