#pragma once

#include "core/IStrategy.hpp"
#include <deque>
#include <string>

namespace tradingbot {
namespace strategies {

class MovingAverageStrategy : public core::IStrategy {
public:
    MovingAverageStrategy();
    
    void initialize(const std::string& config) override;
    core::Decision makeDecision(const core::MarketData& data) override;
    void updateOrderStatus(const core::Decision& decision, bool success) override;
    std::string getMetrics() const override;
    
private:
    // Calcula a média móvel simples
    double calculateSMA() const;
    
    // Verifica se o preço cruzou a média móvel para cima
    bool hasCrossedAboveMA(double currentPrice) const;
    
    // Verifica se o preço cruzou a média móvel para baixo
    bool hasCrossedBelowMA(double currentPrice) const;
    
    // Parâmetros da estratégia
    int period_;                // Período da média móvel
    double positionSize_;       // Tamanho da posição em percentual do capital
    double lastPrice_;          // Último preço processado
    double lastMA_;            // Última média móvel calculada
    
    // Histórico de preços
    std::deque<double> priceHistory_;
    
    // Métricas
    int totalTrades_;
    int successfulTrades_;
    int failedTrades_;
    double totalPnL_;
};

} // namespace strategies
} // namespace tradingbot 