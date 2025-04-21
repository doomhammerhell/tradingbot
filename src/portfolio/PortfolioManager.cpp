#include "portfolio/PortfolioManager.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>

namespace tradingbot {
namespace portfolio {

PortfolioManager::PortfolioManager() {
    portfolio_.totalBalance = 0.0;
    portfolio_.availableBalance = 0.0;
    portfolio_.totalPnL = 0.0;
    portfolio_.dailyPnL = 0.0;
    portfolio_.maxDrawdown = 0.0;
    portfolio_.riskFreeRate = 0.02; // 2% annual risk-free rate
}

void PortfolioManager::initialize(const std::string& config) {
    try {
        config_ = nlohmann::json::parse(config);
        
        portfolio_.totalBalance = config_["initial_balance"].get<double>();
        portfolio_.availableBalance = portfolio_.totalBalance;
        
        if (config_.contains("risk_free_rate")) {
            portfolio_.riskFreeRate = config_["risk_free_rate"].get<double>();
        }
        
        Logger::getInstance().info("PortfolioManager initialized with config: {}", config);
    } catch (const std::exception& e) {
        Logger::getInstance().error("Failed to initialize PortfolioManager: {}", e.what());
        throw;
    }
}

void PortfolioManager::updatePosition(const std::string& symbol, double quantity, 
                                    double price, double stopLoss, double takeProfit) {
    auto& position = portfolio_.positions[symbol];
    
    if (position.quantity == 0.0) {
        // New position
        position.symbol = symbol;
        position.quantity = quantity;
        position.averagePrice = price;
        position.entryTime = std::chrono::system_clock::now();
        position.stopLoss = stopLoss;
        position.takeProfit = takeProfit;
    } else {
        // Update existing position
        double totalValue = (position.quantity * position.averagePrice) + (quantity * price);
        position.quantity += quantity;
        position.averagePrice = totalValue / position.quantity;
        
        if (stopLoss > 0.0) position.stopLoss = stopLoss;
        if (takeProfit > 0.0) position.takeProfit = takeProfit;
    }
    
    position.currentPrice = price;
    position.lastUpdateTime = std::chrono::system_clock::now();
    updatePositionMetrics(position);
    updatePortfolioMetrics();
}

void PortfolioManager::closePosition(const std::string& symbol, double price) {
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return;
    }
    
    auto& position = it->second;
    double pnl = (price - position.averagePrice) * position.quantity;
    position.realizedPnL += pnl;
    portfolio_.totalPnL += pnl;
    
    portfolio_.positions.erase(it);
    updatePortfolioMetrics();
}

Position PortfolioManager::getPosition(const std::string& symbol) const {
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return Position{};
    }
    return it->second;
}

double PortfolioManager::calculatePositionSize(const std::string& symbol, double price, 
                                            double stopLoss, double riskAmount) const {
    if (stopLoss <= 0.0 || riskAmount <= 0.0) {
        return 0.0;
    }
    
    double riskPerUnit = std::abs(price - stopLoss);
    double positionSize = riskAmount / riskPerUnit;
    
    // Apply position limits
    double maxPosition = portfolio_.availableBalance / price;
    return std::min(positionSize, maxPosition);
}

double PortfolioManager::calculateRiskPerTrade(double accountRisk) const {
    return portfolio_.totalBalance * accountRisk;
}

bool PortfolioManager::checkPositionLimits(const std::string& symbol, double quantity) const {
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return true;
    }
    
    const auto& position = it->second;
    double newQuantity = position.quantity + quantity;
    double positionValue = newQuantity * position.currentPrice;
    double portfolioValue = calculatePortfolioValue();
    
    // Check if position exceeds maximum allocation
    auto allocIt = portfolio_.assetAllocation.find(symbol);
    if (allocIt != portfolio_.assetAllocation.end()) {
        double currentWeight = positionValue / portfolioValue;
        if (currentWeight > allocIt->second.maxWeight) {
            return false;
        }
    }
    
    return true;
}

void PortfolioManager::setAssetAllocation(const std::string& symbol, double targetWeight, 
                                        double maxDeviation, double minWeight, double maxWeight) {
    AssetAllocation allocation;
    allocation.targetWeight = targetWeight;
    allocation.maxDeviation = maxDeviation;
    allocation.minWeight = minWeight;
    allocation.maxWeight = maxWeight;
    
    portfolio_.assetAllocation[symbol] = allocation;
    updateAssetAllocation();
}

bool PortfolioManager::needsRebalancing() const {
    for (const auto& [symbol, allocation] : portfolio_.assetAllocation) {
        double currentWeight = 0.0;
        auto posIt = portfolio_.positions.find(symbol);
        if (posIt != portfolio_.positions.end()) {
            currentWeight = calculatePositionValue(posIt->second) / calculatePortfolioValue();
        }
        
        if (std::abs(currentWeight - allocation.targetWeight) > allocation.maxDeviation) {
            return true;
        }
    }
    return false;
}

std::vector<std::pair<std::string, double>> PortfolioManager::calculateRebalanceOrders() const {
    std::vector<std::pair<std::string, double>> orders;
    double portfolioValue = calculatePortfolioValue();
    
    for (const auto& [symbol, allocation] : portfolio_.assetAllocation) {
        double currentValue = 0.0;
        auto posIt = portfolio_.positions.find(symbol);
        if (posIt != portfolio_.positions.end()) {
            currentValue = calculatePositionValue(posIt->second);
        }
        
        double targetValue = portfolioValue * allocation.targetWeight;
        double difference = targetValue - currentValue;
        
        if (std::abs(difference) > 0.0) {
            orders.emplace_back(symbol, difference);
        }
    }
    
    return orders;
}

double PortfolioManager::calculateSharpeRatio() const {
    if (dailyReturns_.empty()) {
        return 0.0;
    }
    
    double mean = std::accumulate(dailyReturns_.begin(), dailyReturns_.end(), 0.0) / dailyReturns_.size();
    double variance = 0.0;
    for (double ret : dailyReturns_) {
        variance += std::pow(ret - mean, 2);
    }
    variance /= dailyReturns_.size();
    double stdDev = std::sqrt(variance);
    
    return (mean - portfolio_.riskFreeRate/252.0) / stdDev * std::sqrt(252.0);
}

double PortfolioManager::calculateSortinoRatio() const {
    if (dailyReturns_.empty()) {
        return 0.0;
    }
    
    double mean = std::accumulate(dailyReturns_.begin(), dailyReturns_.end(), 0.0) / dailyReturns_.size();
    double downsideVariance = 0.0;
    for (double ret : dailyReturns_) {
        if (ret < 0.0) {
            downsideVariance += std::pow(ret, 2);
        }
    }
    downsideVariance /= dailyReturns_.size();
    double downsideStdDev = std::sqrt(downsideVariance);
    
    return (mean - portfolio_.riskFreeRate/252.0) / downsideStdDev * std::sqrt(252.0);
}

double PortfolioManager::calculateMaxDrawdown() const {
    if (dailyReturns_.empty()) {
        return 0.0;
    }
    
    double peak = 1.0;
    double maxDrawdown = 0.0;
    double currentValue = 1.0;
    
    for (double ret : dailyReturns_) {
        currentValue *= (1.0 + ret);
        if (currentValue > peak) {
            peak = currentValue;
        }
        double drawdown = (peak - currentValue) / peak;
        maxDrawdown = std::max(maxDrawdown, drawdown);
    }
    
    return maxDrawdown;
}

double PortfolioManager::calculateWinRate() const {
    if (tradeResults_.empty()) {
        return 0.0;
    }
    
    size_t winningTrades = std::count_if(tradeResults_.begin(), tradeResults_.end(),
        [](double result) { return result > 0.0; });
    
    return static_cast<double>(winningTrades) / tradeResults_.size();
}

double PortfolioManager::calculateProfitFactor() const {
    if (tradeResults_.empty()) {
        return 0.0;
    }
    
    double grossProfit = 0.0;
    double grossLoss = 0.0;
    
    for (double result : tradeResults_) {
        if (result > 0.0) {
            grossProfit += result;
        } else {
            grossLoss += std::abs(result);
        }
    }
    
    return grossLoss > 0.0 ? grossProfit / grossLoss : 0.0;
}

Portfolio PortfolioManager::getPortfolio() const {
    return portfolio_;
}

void PortfolioManager::updatePortfolioMetrics() {
    double portfolioValue = calculatePortfolioValue();
    portfolio_.totalBalance = portfolioValue;
    portfolio_.availableBalance = portfolioValue;
    
    // Update daily returns
    double dailyReturn = calculateDailyReturns();
    dailyReturns_.push_back(dailyReturn);
    if (dailyReturns_.size() > 252) { // Keep last year of returns
        dailyReturns_.erase(dailyReturns_.begin());
    }
    
    // Update max drawdown
    portfolio_.maxDrawdown = calculateMaxDrawdown();
}

std::string PortfolioManager::getMetrics() const {
    nlohmann::json metrics;
    
    metrics["total_balance"] = portfolio_.totalBalance;
    metrics["available_balance"] = portfolio_.availableBalance;
    metrics["total_pnl"] = portfolio_.totalPnL;
    metrics["daily_pnl"] = portfolio_.dailyPnL;
    metrics["max_drawdown"] = portfolio_.maxDrawdown;
    metrics["sharpe_ratio"] = calculateSharpeRatio();
    metrics["sortino_ratio"] = calculateSortinoRatio();
    metrics["win_rate"] = calculateWinRate();
    metrics["profit_factor"] = calculateProfitFactor();
    
    // Position metrics
    nlohmann::json positions;
    for (const auto& [symbol, position] : portfolio_.positions) {
        positions[symbol] = {
            {"quantity", position.quantity},
            {"average_price", position.averagePrice},
            {"current_price", position.currentPrice},
            {"unrealized_pnl", position.unrealizedPnL},
            {"realized_pnl", position.realizedPnL}
        };
    }
    metrics["positions"] = positions;
    
    // Asset allocation metrics
    nlohmann::json allocation;
    for (const auto& [symbol, alloc] : portfolio_.assetAllocation) {
        allocation[symbol] = {
            {"target_weight", alloc.targetWeight},
            {"current_weight", alloc.currentWeight},
            {"max_deviation", alloc.maxDeviation}
        };
    }
    metrics["asset_allocation"] = allocation;
    
    return metrics.dump(4);
}

void PortfolioManager::updateAssetAllocation() {
    double portfolioValue = calculatePortfolioValue();
    
    for (auto& [symbol, allocation] : portfolio_.assetAllocation) {
        double currentValue = 0.0;
        auto posIt = portfolio_.positions.find(symbol);
        if (posIt != portfolio_.positions.end()) {
            currentValue = calculatePositionValue(posIt->second);
        }
        
        allocation.currentWeight = currentValue / portfolioValue;
    }
}

void PortfolioManager::updatePositionMetrics(Position& position) {
    position.unrealizedPnL = (position.currentPrice - position.averagePrice) * position.quantity;
}

double PortfolioManager::calculatePositionValue(const Position& position) const {
    return position.quantity * position.currentPrice;
}

double PortfolioManager::calculatePortfolioValue() const {
    double value = portfolio_.availableBalance;
    for (const auto& [symbol, position] : portfolio_.positions) {
        value += calculatePositionValue(position);
    }
    return value;
}

double PortfolioManager::calculateDailyReturns() const {
    if (dailyReturns_.empty()) {
        return 0.0;
    }
    return dailyReturns_.back();
}

void PortfolioManager::setTargetVolatility(double targetVol) {
    portfolio_.targetVolatility = targetVol;
    Logger::getInstance().info("Set target volatility to: {}", targetVol);
}

void PortfolioManager::updateVolatilityMetrics() {
    // Calculate portfolio volatility
    std::vector<double> portfolioReturns;
    for (const auto& [symbol, returns] : portfolio_.historicalReturns) {
        if (!returns.empty()) {
            portfolioReturns.push_back(returns.back());
        }
    }
    
    if (!portfolioReturns.empty()) {
        double mean = std::accumulate(portfolioReturns.begin(), portfolioReturns.end(), 0.0) 
                     / portfolioReturns.size();
        double variance = 0.0;
        for (double ret : portfolioReturns) {
            variance += std::pow(ret - mean, 2);
        }
        variance /= portfolioReturns.size();
        portfolio_.currentVolatility = std::sqrt(variance) * std::sqrt(252.0); // Annualized
    }
    
    // Update individual position volatilities
    for (auto& [symbol, position] : portfolio_.positions) {
        position.volatility = calculateHistoricalVolatility(symbol);
    }
}

bool PortfolioManager::isVolatilityWithinTarget() const {
    if (portfolio_.targetVolatility <= 0.0) {
        return true; // No target set
    }
    
    double tolerance = 0.01; // 1% tolerance
    return std::abs(portfolio_.currentVolatility - portfolio_.targetVolatility) 
           <= portfolio_.targetVolatility * tolerance;
}

std::vector<std::pair<std::string, double>> PortfolioManager::calculateVolatilityAdjustments() const {
    std::vector<std::pair<std::string, double>> adjustments;
    
    if (portfolio_.targetVolatility <= 0.0 || portfolio_.currentVolatility <= 0.0) {
        return adjustments;
    }
    
    double adjustmentFactor = portfolio_.targetVolatility / portfolio_.currentVolatility;
    
    for (const auto& [symbol, position] : portfolio_.positions) {
        double currentValue = calculatePositionValue(position);
        double adjustedValue = currentValue * adjustmentFactor;
        double difference = adjustedValue - currentValue;
        
        if (std::abs(difference) > 0.0) {
            adjustments.emplace_back(symbol, difference);
        }
    }
    
    return adjustments;
}

void PortfolioManager::updateCorrelationMetrics() {
    for (auto& [symbol, position] : portfolio_.positions) {
        position.correlation = calculatePortfolioCorrelation(symbol);
    }
}

double PortfolioManager::calculatePortfolioCorrelation(const std::string& symbol) const {
    if (portfolio_.positions.size() < 2) {
        return 0.0; // Need at least 2 positions for correlation
    }
    
    double maxCorrelation = 0.0;
    for (const auto& [otherSymbol, otherPosition] : portfolio_.positions) {
        if (symbol != otherSymbol) {
            double correlation = calculateCorrelation(symbol, otherSymbol);
            maxCorrelation = std::max(maxCorrelation, std::abs(correlation));
        }
    }
    
    return maxCorrelation;
}

bool PortfolioManager::isCorrelationAcceptable(const std::string& symbol) const {
    if (portfolio_.correlationThreshold <= 0.0) {
        return true; // No threshold set
    }
    
    double correlation = calculatePortfolioCorrelation(symbol);
    return correlation <= portfolio_.correlationThreshold;
}

double PortfolioManager::calculateCorrelationAdjustedSize(const std::string& symbol, double price,
                                                       double stopLoss, double riskAmount) const {
    double baseSize = calculatePositionSize(symbol, price, stopLoss, riskAmount);
    double correlation = calculatePortfolioCorrelation(symbol);
    
    // Reduce position size based on correlation
    // Higher correlation = smaller position
    double correlationFactor = 1.0 - correlation;
    return baseSize * correlationFactor;
}

double PortfolioManager::calculateVolatilityAdjustedSize(const std::string& symbol, double price,
                                                      double riskAmount) const {
    double volatility = calculateHistoricalVolatility(symbol);
    if (volatility <= 0.0) {
        return 0.0;
    }
    
    // Inverse volatility weighting
    // Higher volatility = smaller position
    double volatilityFactor = portfolio_.targetVolatility / volatility;
    return (riskAmount / price) * volatilityFactor;
}

double PortfolioManager::calculateOptimalPositionSize(const std::string& symbol, double price,
                                                   double stopLoss, double riskAmount) const {
    double correlationSize = calculateCorrelationAdjustedSize(symbol, price, stopLoss, riskAmount);
    double volatilitySize = calculateVolatilityAdjustedSize(symbol, price, riskAmount);
    
    // Take the minimum of the two sizes to be conservative
    return std::min(correlationSize, volatilitySize);
}

double PortfolioManager::calculateHistoricalVolatility(const std::string& symbol) const {
    auto it = portfolio_.historicalReturns.find(symbol);
    if (it == portfolio_.historicalReturns.end() || it->second.size() < 20) {
        return 0.0;
    }
    
    const auto& returns = it->second;
    size_t lookback = std::min(returns.size(), static_cast<size_t>(20));
    auto start = returns.end() - lookback;
    
    double mean = std::accumulate(start, returns.end(), 0.0) / lookback;
    double variance = 0.0;
    for (auto it = start; it != returns.end(); ++it) {
        variance += std::pow(*it - mean, 2);
    }
    variance /= lookback;
    
    return std::sqrt(variance) * std::sqrt(252.0); // Annualized volatility
}

double PortfolioManager::calculateCorrelation(const std::string& symbol1, 
                                           const std::string& symbol2) const {
    auto it1 = portfolio_.historicalReturns.find(symbol1);
    auto it2 = portfolio_.historicalReturns.find(symbol2);
    
    if (it1 == portfolio_.historicalReturns.end() || 
        it2 == portfolio_.historicalReturns.end()) {
        return 0.0;
    }
    
    const auto& returns1 = it1->second;
    const auto& returns2 = it2->second;
    
    size_t minSize = std::min(returns1.size(), returns2.size());
    if (minSize < 2) {
        return 0.0;
    }
    
    double mean1 = std::accumulate(returns1.end() - minSize, returns1.end(), 0.0) / minSize;
    double mean2 = std::accumulate(returns2.end() - minSize, returns2.end(), 0.0) / minSize;
    
    double covariance = 0.0;
    double variance1 = 0.0;
    double variance2 = 0.0;
    
    for (size_t i = 0; i < minSize; ++i) {
        double diff1 = returns1[returns1.size() - minSize + i] - mean1;
        double diff2 = returns2[returns2.size() - minSize + i] - mean2;
        covariance += diff1 * diff2;
        variance1 += diff1 * diff1;
        variance2 += diff2 * diff2;
    }
    
    covariance /= minSize;
    variance1 /= minSize;
    variance2 /= minSize;
    
    if (variance1 <= 0.0 || variance2 <= 0.0) {
        return 0.0;
    }
    
    return covariance / (std::sqrt(variance1) * std::sqrt(variance2));
}

void PortfolioManager::updateHistoricalReturns(const std::string& symbol, double returnValue) {
    auto& returns = portfolio_.historicalReturns[symbol];
    returns.push_back(returnValue);
    
    // Keep only the last 252 days of returns (1 year)
    if (returns.size() > 252) {
        returns.erase(returns.begin());
    }
}

void PortfolioManager::setMaxDrawdown(double maxDrawdown) {
    portfolio_.maxAllowedDrawdown = maxDrawdown;
    Logger::getInstance().info("Set maximum allowed drawdown to: {}", maxDrawdown);
}

bool PortfolioManager::isDrawdownWithinLimits() const {
    return portfolio_.currentDrawdown <= portfolio_.maxAllowedDrawdown;
}

void PortfolioManager::updateDrawdownMetrics() {
    double portfolioValue = calculatePortfolioValue();
    double peakValue = portfolioValue;
    
    // Update position drawdowns
    for (auto& [symbol, position] : portfolio_.positions) {
        double positionValue = calculatePositionValue(position);
        if (positionValue > position.peakValue) {
            position.peakValue = positionValue;
            position.drawdown = 0.0;
        } else {
            position.drawdown = (position.peakValue - positionValue) / position.peakValue;
        }
        peakValue = std::max(peakValue, positionValue);
    }
    
    // Update portfolio drawdown
    portfolio_.currentDrawdown = (peakValue - portfolioValue) / peakValue;
    portfolio_.maxDrawdown = std::max(portfolio_.maxDrawdown, portfolio_.currentDrawdown);
}

std::vector<std::pair<std::string, double>> PortfolioManager::calculateDrawdownAdjustments() const {
    std::vector<std::pair<std::string, double>> adjustments;
    
    if (portfolio_.currentDrawdown <= portfolio_.maxAllowedDrawdown) {
        return adjustments;
    }
    
    // Calculate required reduction to stay within drawdown limits
    double excessDrawdown = portfolio_.currentDrawdown - portfolio_.maxAllowedDrawdown;
    double requiredReduction = excessDrawdown * calculatePortfolioValue();
    
    // Distribute reduction across positions based on their drawdown
    double totalDrawdown = 0.0;
    for (const auto& [symbol, position] : portfolio_.positions) {
        totalDrawdown += position.drawdown;
    }
    
    for (const auto& [symbol, position] : portfolio_.positions) {
        if (position.drawdown > 0.0) {
            double reduction = (position.drawdown / totalDrawdown) * requiredReduction;
            double currentValue = calculatePositionValue(position);
            double newValue = currentValue - reduction;
            double price = position.currentPrice;
            double quantity = newValue / price;
            double adjustment = quantity - position.quantity;
            
            if (std::abs(adjustment) > 0.0) {
                adjustments.emplace_back(symbol, adjustment);
            }
        }
    }
    
    return adjustments;
}

void PortfolioManager::setRiskParityMethod(RiskParityMethod method) {
    portfolio_.riskParityMethod = method;
    Logger::getInstance().info("Set risk parity method to: {}", static_cast<int>(method));
}

void PortfolioManager::calculateRiskParityWeights() {
    switch (portfolio_.riskParityMethod) {
        case RiskParityMethod::EQUAL_RISK_CONTRIBUTION:
            calculateEqualRiskContributionWeights();
            break;
        case RiskParityMethod::INVERSE_VOLATILITY:
            calculateInverseVolatilityWeights();
            break;
        case RiskParityMethod::MINIMUM_VARIANCE:
            calculateMinimumVarianceWeights();
            break;
        case RiskParityMethod::MAXIMUM_DIVERSIFICATION:
            calculateMaximumDiversificationWeights();
            break;
    }
}

void PortfolioManager::calculateEqualRiskContributionWeights() {
    // Calculate covariance matrix
    std::map<std::string, std::map<std::string, double>> covariance;
    for (const auto& [symbol1, _] : portfolio_.positions) {
        for (const auto& [symbol2, __] : portfolio_.positions) {
            covariance[symbol1][symbol2] = calculateCovariance(symbol1, symbol2);
        }
    }
    
    // Calculate risk contributions
    for (auto& [symbol, allocation] : portfolio_.assetAllocation) {
        double riskContribution = 0.0;
        for (const auto& [otherSymbol, otherAllocation] : portfolio_.assetAllocation) {
            riskContribution += allocation.currentWeight * otherAllocation.currentWeight 
                              * covariance[symbol][otherSymbol];
        }
        allocation.riskParityWeight = riskContribution;
    }
    
    // Normalize weights
    double totalRisk = 0.0;
    for (const auto& [_, allocation] : portfolio_.assetAllocation) {
        totalRisk += allocation.riskParityWeight;
    }
    
    for (auto& [_, allocation] : portfolio_.assetAllocation) {
        allocation.riskParityWeight /= totalRisk;
    }
}

void PortfolioManager::calculateInverseVolatilityWeights() {
    double totalInverseVol = 0.0;
    
    // Calculate inverse volatility weights
    for (auto& [symbol, allocation] : portfolio_.assetAllocation) {
        double volatility = calculateHistoricalVolatility(symbol);
        if (volatility > 0.0) {
            allocation.riskParityWeight = 1.0 / volatility;
            totalInverseVol += allocation.riskParityWeight;
        }
    }
    
    // Normalize weights
    for (auto& [_, allocation] : portfolio_.assetAllocation) {
        allocation.riskParityWeight /= totalInverseVol;
    }
}

void PortfolioManager::calculateMinimumVarianceWeights() {
    // Implement minimum variance portfolio optimization
    // This is a simplified version - in practice, you'd use a proper optimization library
    std::map<std::string, std::map<std::string, double>> covariance;
    for (const auto& [symbol1, _] : portfolio_.positions) {
        for (const auto& [symbol2, __] : portfolio_.positions) {
            covariance[symbol1][symbol2] = calculateCovariance(symbol1, symbol2);
        }
    }
    
    // Simple heuristic: weight inversely proportional to row sum of covariance matrix
    double totalWeight = 0.0;
    for (auto& [symbol, allocation] : portfolio_.assetAllocation) {
        double rowSum = 0.0;
        for (const auto& [otherSymbol, _] : portfolio_.positions) {
            rowSum += covariance[symbol][otherSymbol];
        }
        allocation.riskParityWeight = 1.0 / rowSum;
        totalWeight += allocation.riskParityWeight;
    }
    
    // Normalize weights
    for (auto& [_, allocation] : portfolio_.assetAllocation) {
        allocation.riskParityWeight /= totalWeight;
    }
}

void PortfolioManager::calculateMaximumDiversificationWeights() {
    // Calculate correlation matrix
    std::map<std::string, std::map<std::string, double>> correlation;
    for (const auto& [symbol1, _] : portfolio_.positions) {
        for (const auto& [symbol2, __] : portfolio_.positions) {
            correlation[symbol1][symbol2] = calculateCorrelation(symbol1, symbol2);
        }
    }
    
    // Calculate diversification ratio
    double totalWeight = 0.0;
    for (auto& [symbol, allocation] : portfolio_.assetAllocation) {
        double diversification = 0.0;
        for (const auto& [otherSymbol, otherAllocation] : portfolio_.assetAllocation) {
            diversification += (1.0 - correlation[symbol][otherSymbol]) 
                             * otherAllocation.currentWeight;
        }
        allocation.riskParityWeight = diversification;
        totalWeight += diversification;
    }
    
    // Normalize weights
    for (auto& [_, allocation] : portfolio_.assetAllocation) {
        allocation.riskParityWeight /= totalWeight;
    }
}

double PortfolioManager::calculateRiskContribution(const std::string& symbol) const {
    double contribution = 0.0;
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    for (const auto& [otherSymbol, otherPosition] : portfolio_.positions) {
        double correlation = calculateCorrelation(symbol, otherSymbol);
        double covariance = correlation * position.volatility * otherPosition.volatility;
        contribution += position.quantity * otherPosition.quantity * covariance;
    }
    
    return contribution;
}

std::vector<std::pair<std::string, double>> PortfolioManager::calculateRiskParityAdjustments() const {
    std::vector<std::pair<std::string, double>> adjustments;
    
    for (const auto& [symbol, allocation] : portfolio_.assetAllocation) {
        double currentRisk = calculateRiskContribution(symbol);
        double targetRisk = portfolio_.riskParityTarget / portfolio_.positions.size();
        double difference = targetRisk - currentRisk;
        
        if (std::abs(difference) > 0.0) {
            auto it = portfolio_.positions.find(symbol);
            if (it != portfolio_.positions.end()) {
                double price = it->second.currentPrice;
                double adjustment = difference / price;
                adjustments.emplace_back(symbol, adjustment);
            }
        }
    }
    
    return adjustments;
}

double PortfolioManager::calculateRollingCorrelation(const std::string& symbol1, 
                                                   const std::string& symbol2,
                                                   size_t window) const {
    auto it1 = portfolio_.rollingReturns.find(symbol1);
    auto it2 = portfolio_.rollingReturns.find(symbol2);
    
    if (it1 == portfolio_.rollingReturns.end() || 
        it2 == portfolio_.rollingReturns.end()) {
        return 0.0;
    }
    
    const auto& returns1 = it1->second;
    const auto& returns2 = it2->second;
    
    if (returns1.size() < window || returns2.size() < window) {
        return 0.0;
    }
    
    double mean1 = 0.0, mean2 = 0.0;
    for (size_t i = 0; i < window; ++i) {
        mean1 += returns1[i];
        mean2 += returns2[i];
    }
    mean1 /= window;
    mean2 /= window;
    
    double covariance = 0.0;
    double variance1 = 0.0;
    double variance2 = 0.0;
    
    for (size_t i = 0; i < window; ++i) {
        double diff1 = returns1[i] - mean1;
        double diff2 = returns2[i] - mean2;
        covariance += diff1 * diff2;
        variance1 += diff1 * diff1;
        variance2 += diff2 * diff2;
    }
    
    covariance /= window;
    variance1 /= window;
    variance2 /= window;
    
    if (variance1 <= 0.0 || variance2 <= 0.0) {
        return 0.0;
    }
    
    return covariance / (std::sqrt(variance1) * std::sqrt(variance2));
}

double PortfolioManager::calculateRegimeAdjustedCorrelation(const std::string& symbol1, 
                                                          const std::string& symbol2) const {
    auto regimes = calculateRegimeCorrelations(symbol1, symbol2);
    double weightedCorrelation = 0.0;
    double totalWeight = 0.0;
    
    for (const auto& [regime, correlation] : regimes) {
        double probability = calculateRegimeProbability(regime);
        weightedCorrelation += correlation * probability;
        totalWeight += probability;
    }
    
    return totalWeight > 0.0 ? weightedCorrelation / totalWeight : 0.0;
}

std::map<MarketRegime, double> PortfolioManager::calculateRegimeCorrelations(
    const std::string& symbol1, const std::string& symbol2) const {
    std::map<MarketRegime, double> regimeCorrelations;
    
    // Filter returns by regime
    std::map<MarketRegime, std::vector<double>> regimeReturns1;
    std::map<MarketRegime, std::vector<double>> regimeReturns2;
    
    auto it1 = portfolio_.historicalReturns.find(symbol1);
    auto it2 = portfolio_.historicalReturns.find(symbol2);
    
    if (it1 == portfolio_.historicalReturns.end() || 
        it2 == portfolio_.historicalReturns.end()) {
        return regimeCorrelations;
    }
    
    const auto& returns1 = it1->second;
    const auto& returns2 = it2->second;
    
    size_t minSize = std::min(returns1.size(), returns2.size());
    for (size_t i = 0; i < minSize; ++i) {
        MarketRegime regime = detectRegimeFromMetrics(returns1[i], 0.0, 0.0);
        regimeReturns1[regime].push_back(returns1[i]);
        regimeReturns2[regime].push_back(returns2[i]);
    }
    
    // Calculate correlation for each regime
    for (const auto& [regime, returns] : regimeReturns1) {
        if (returns.size() >= 2) {
            double mean1 = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
            double mean2 = std::accumulate(regimeReturns2[regime].begin(), 
                                         regimeReturns2[regime].end(), 0.0) 
                         / regimeReturns2[regime].size();
            
            double covariance = 0.0;
            double variance1 = 0.0;
            double variance2 = 0.0;
            
            for (size_t i = 0; i < returns.size(); ++i) {
                double diff1 = returns[i] - mean1;
                double diff2 = regimeReturns2[regime][i] - mean2;
                covariance += diff1 * diff2;
                variance1 += diff1 * diff1;
                variance2 += diff2 * diff2;
            }
            
            covariance /= returns.size();
            variance1 /= returns.size();
            variance2 /= returns.size();
            
            if (variance1 > 0.0 && variance2 > 0.0) {
                regimeCorrelations[regime] = covariance / (std::sqrt(variance1) * std::sqrt(variance2));
            }
        }
    }
    
    return regimeCorrelations;
}

MarketRegime PortfolioManager::detectMarketRegime(const std::string& symbol) const {
    auto it = portfolio_.historicalReturns.find(symbol);
    if (it == portfolio_.historicalReturns.end() || it->second.size() < 20) {
        return MarketRegime::UNKNOWN;
    }
    
    const auto& returns = it->second;
    size_t lookback = std::min(returns.size(), static_cast<size_t>(20));
    auto start = returns.end() - lookback;
    
    double mean = std::accumulate(start, returns.end(), 0.0) / lookback;
    double variance = 0.0;
    for (auto it = start; it != returns.end(); ++it) {
        variance += std::pow(*it - mean, 2);
    }
    variance /= lookback;
    double volatility = std::sqrt(variance);
    
    // Simple regime detection based on returns and volatility
    if (volatility > 0.02) { // 2% daily volatility threshold
        return MarketRegime::HIGH_VOLATILITY;
    } else if (volatility < 0.005) { // 0.5% daily volatility threshold
        return MarketRegime::LOW_VOLATILITY;
    } else if (mean > 0.001) { // 0.1% daily return threshold
        return MarketRegime::TRENDING_UP;
    } else if (mean < -0.001) {
        return MarketRegime::TRENDING_DOWN;
    } else {
        return MarketRegime::RANGING;
    }
}

void PortfolioManager::updateMarketRegimes() {
    for (auto& [symbol, position] : portfolio_.positions) {
        position.currentRegime = detectMarketRegime(symbol);
    }
    
    // Update portfolio regime based on majority of positions
    std::map<MarketRegime, int> regimeCounts;
    for (const auto& [_, position] : portfolio_.positions) {
        regimeCounts[position.currentRegime]++;
    }
    
    auto maxRegime = std::max_element(regimeCounts.begin(), regimeCounts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    if (maxRegime != regimeCounts.end()) {
        portfolio_.currentRegime = maxRegime->first;
    }
}

double PortfolioManager::calculateRegimeProbability(MarketRegime regime) const {
    int totalPositions = portfolio_.positions.size();
    if (totalPositions == 0) {
        return 0.0;
    }
    
    int regimeCount = 0;
    for (const auto& [_, position] : portfolio_.positions) {
        if (position.currentRegime == regime) {
            regimeCount++;
        }
    }
    
    return static_cast<double>(regimeCount) / totalPositions;
}

std::map<MarketRegime, double> PortfolioManager::getRegimeProbabilities() const {
    std::map<MarketRegime, double> probabilities;
    for (int i = 0; i < static_cast<int>(MarketRegime::UNKNOWN); ++i) {
        MarketRegime regime = static_cast<MarketRegime>(i);
        probabilities[regime] = calculateRegimeProbability(regime);
    }
    return probabilities;
}

double PortfolioManager::calculateCalmarRatio() const {
    if (portfolio_.maxDrawdown <= 0.0) {
        return 0.0;
    }
    
    double annualizedReturn = std::accumulate(dailyReturns_.begin(), dailyReturns_.end(), 0.0)
                            * std::sqrt(252.0);
    return annualizedReturn / portfolio_.maxDrawdown;
}

double PortfolioManager::calculateOmegaRatio() const {
    if (dailyReturns_.empty()) {
        return 0.0;
    }
    
    double threshold = portfolio_.riskFreeRate / 252.0; // Daily risk-free rate
    double gains = 0.0;
    double losses = 0.0;
    
    for (double ret : dailyReturns_) {
        if (ret > threshold) {
            gains += ret - threshold;
        } else {
            losses += threshold - ret;
        }
    }
    
    return losses > 0.0 ? gains / losses : 0.0;
}

double PortfolioManager::calculateTailRatio() const {
    if (dailyReturns_.empty()) {
        return 0.0;
    }
    
    std::vector<double> sortedReturns = dailyReturns_;
    std::sort(sortedReturns.begin(), sortedReturns.end());
    
    size_t n = sortedReturns.size();
    size_t tailSize = n / 10; // 10% tail
    
    double rightTail = 0.0;
    double leftTail = 0.0;
    
    for (size_t i = 0; i < tailSize; ++i) {
        rightTail += sortedReturns[n - 1 - i];
        leftTail += sortedReturns[i];
    }
    
    rightTail /= tailSize;
    leftTail /= tailSize;
    
    return std::abs(leftTail) > 0.0 ? rightTail / std::abs(leftTail) : 0.0;
}

void PortfolioManager::updateRollingMetrics(const std::string& symbol, double returnValue) {
    // Update rolling returns
    auto& returns = portfolio_.rollingReturns[symbol];
    returns.push_back(returnValue);
    if (returns.size() > 252) { // Keep 1 year of rolling returns
        returns.pop_front();
    }
    
    // Update rolling correlations
    for (const auto& [otherSymbol, _] : portfolio_.positions) {
        if (symbol != otherSymbol) {
            auto& correlations = portfolio_.rollingCorrelations[{symbol, otherSymbol}];
            double correlation = calculateRollingCorrelation(symbol, otherSymbol);
            correlations.push_back(correlation);
            if (correlations.size() > 20) { // Keep 20 periods of rolling correlations
                correlations.pop_front();
            }
        }
    }
}

MarketRegime PortfolioManager::detectRegimeFromMetrics(double returns, double volatility,
                                                     double correlation) const {
    if (volatility > 0.02) {
        return MarketRegime::HIGH_VOLATILITY;
    } else if (volatility < 0.005) {
        return MarketRegime::LOW_VOLATILITY;
    } else if (returns > 0.001) {
        return MarketRegime::TRENDING_UP;
    } else if (returns < -0.001) {
        return MarketRegime::TRENDING_DOWN;
    } else {
        return MarketRegime::RANGING;
    }
}

double PortfolioManager::calculateRegimeTransitionProbability(MarketRegime from, 
                                                           MarketRegime to) const {
    // Simple implementation - in practice, you'd use a Markov chain model
    static const std::map<std::pair<MarketRegime, MarketRegime>, double> transitionProbs = {
        {{MarketRegime::TRENDING_UP, MarketRegime::TRENDING_UP}, 0.7},
        {{MarketRegime::TRENDING_UP, MarketRegime::TRENDING_DOWN}, 0.1},
        {{MarketRegime::TRENDING_UP, MarketRegime::RANGING}, 0.2},
        // ... add more transition probabilities
    };
    
    auto it = transitionProbs.find({from, to});
    return it != transitionProbs.end() ? it->second : 0.1; // Default probability
}

void PortfolioManager::updateRiskContributions() {
    for (auto& [symbol, position] : portfolio_.positions) {
        position.riskContribution = calculateRiskContribution(symbol);
    }
}

// Factor-based allocation implementation
void PortfolioManager::addFactor(FactorType type, double weight) {
    Factor factor;
    factor.type = type;
    factor.weight = weight;
    factor.score = 0.0;
    factor.lastUpdate = std::chrono::system_clock::now();
    portfolio_.factors[type] = factor;
}

void PortfolioManager::removeFactor(FactorType type) {
    portfolio_.factors.erase(type);
}

void PortfolioManager::updateFactorScores() {
    for (auto& [type, factor] : portfolio_.factors) {
        for (const auto& [symbol, _] : portfolio_.positions) {
            factor.assetScores[symbol] = calculateFactorScore(symbol, type);
        }
        updateFactorHistoricalScores(factor);
        normalizeFactorScores(factor);
    }
}

double PortfolioManager::calculateFactorScore(const std::string& symbol, FactorType factor) const {
    switch (factor) {
        case FactorType::VALUE:
            return calculateValueFactor(symbol);
        case FactorType::MOMENTUM:
            return calculateMomentumFactor(symbol);
        case FactorType::VOLATILITY:
            return calculateVolatilityFactor(symbol);
        case FactorType::QUALITY:
            return calculateQualityFactor(symbol);
        case FactorType::SIZE:
            return calculateSizeFactor(symbol);
        case FactorType::LIQUIDITY:
            return calculateLiquidityFactor(symbol);
        default:
            return 0.0;
    }
}

double PortfolioManager::calculateValueFactor(const std::string& symbol) const {
    // Implement value factor calculation (e.g., P/E, P/B, etc.)
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    // Example: Use inverse of price-to-book ratio
    double bookValue = position.currentPrice * 0.8; // Simplified
    return bookValue > 0.0 ? 1.0 / (position.currentPrice / bookValue) : 0.0;
}

double PortfolioManager::calculateMomentumFactor(const std::string& symbol) const {
    // Implement momentum factor calculation
    auto it = portfolio_.historicalReturns.find(symbol);
    if (it == portfolio_.historicalReturns.end() || it->second.size() < 20) {
        return 0.0;
    }
    
    const auto& returns = it->second;
    size_t lookback = std::min(returns.size(), static_cast<size_t>(20));
    auto start = returns.end() - lookback;
    
    return std::accumulate(start, returns.end(), 0.0) / lookback;
}

double PortfolioManager::calculateVolatilityFactor(const std::string& symbol) const {
    // Implement volatility factor calculation
    return calculateHistoricalVolatility(symbol);
}

double PortfolioManager::calculateQualityFactor(const std::string& symbol) const {
    // Implement quality factor calculation
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    // Example: Combine multiple quality metrics
    double returnOnEquity = 0.15; // Simplified
    double debtToEquity = 0.5;    // Simplified
    return returnOnEquity * (1.0 - debtToEquity);
}

double PortfolioManager::calculateSizeFactor(const std::string& symbol) const {
    // Implement size factor calculation
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    // Example: Use market capitalization
    double marketCap = position.quantity * position.currentPrice;
    return std::log(marketCap); // Log transform for better scaling
}

double PortfolioManager::calculateLiquidityFactor(const std::string& symbol) const {
    // Implement liquidity factor calculation
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    // Example: Use trading volume
    double volume = position.quantity * 1000; // Simplified
    return std::log(volume + 1.0); // Log transform for better scaling
}

// Machine learning-based regime detection implementation
void PortfolioManager::initializeMLModel(MLModelType type, 
                                       const std::map<std::string, double>& hyperparameters) {
    portfolio_.mlConfig.type = type;
    portfolio_.mlConfig.hyperparameters = hyperparameters;
    portfolio_.mlConfig.lookbackPeriod = 20;
    portfolio_.mlConfig.numRegimes = 5;
    portfolio_.mlConfig.isTrained = false;
    portfolio_.mlConfig.lastTrainingTime = std::chrono::system_clock::now();
}

void PortfolioManager::trainMLModel() {
    updateMLFeatures();
    
    switch (portfolio_.mlConfig.type) {
        case MLModelType::KMEANS:
            trainKMeans();
            break;
        case MLModelType::GMM:
            trainGMM();
            break;
        case MLModelType::HMM:
            trainHMM();
            break;
        case MLModelType::RANDOM_FOREST:
            trainRandomForest();
            break;
        case MLModelType::SVM:
            trainSVM();
            break;
        case MLModelType::NEURAL_NETWORK:
            trainNeuralNetwork();
            break;
    }
    
    portfolio_.mlConfig.isTrained = true;
    portfolio_.mlConfig.lastTrainingTime = std::chrono::system_clock::now();
    updateRegimeCentroids();
    calculateRegimeProbabilities();
}

void PortfolioManager::updateMLFeatures() {
    portfolio_.featureMatrix.clear();
    for (const auto& [symbol, position] : portfolio_.positions) {
        portfolio_.featureMatrix.push_back(extractFeatures(symbol));
    }
}

std::vector<double> PortfolioManager::extractFeatures(const std::string& symbol) const {
    std::vector<double> features;
    
    // Add technical indicators
    auto it = portfolio_.historicalReturns.find(symbol);
    if (it != portfolio_.historicalReturns.end()) {
        const auto& returns = it->second;
        if (returns.size() >= 20) {
            // Calculate mean return
            double mean = std::accumulate(returns.end() - 20, returns.end(), 0.0) / 20.0;
            features.push_back(mean);
            
            // Calculate volatility
            double variance = 0.0;
            for (auto rit = returns.end() - 20; rit != returns.end(); ++rit) {
                variance += std::pow(*rit - mean, 2);
            }
            features.push_back(std::sqrt(variance / 20.0));
            
            // Calculate skewness
            double skewness = 0.0;
            for (auto rit = returns.end() - 20; rit != returns.end(); ++rit) {
                skewness += std::pow(*rit - mean, 3);
            }
            features.push_back(skewness / (20.0 * std::pow(std::sqrt(variance / 20.0), 3)));
        }
    }
    
    // Add factor exposures
    for (const auto& [type, factor] : portfolio_.factors) {
        features.push_back(factor.assetScores.at(symbol));
    }
    
    return preprocessFeatures(features);
}

std::vector<double> PortfolioManager::preprocessFeatures(const std::vector<double>& features) const {
    std::vector<double> processed = features;
    
    // Standardize features
    double mean = std::accumulate(processed.begin(), processed.end(), 0.0) / processed.size();
    double variance = 0.0;
    for (double value : processed) {
        variance += std::pow(value - mean, 2);
    }
    double stdDev = std::sqrt(variance / processed.size());
    
    if (stdDev > 0.0) {
        for (double& value : processed) {
            value = (value - mean) / stdDev;
        }
    }
    
    return processed;
}

void PortfolioManager::trainKMeans() {
    // Convert feature matrix to Eigen matrix
    size_t numSamples = portfolio_.featureMatrix.size();
    size_t numFeatures = portfolio_.featureMatrix[0].size();
    Eigen::MatrixXd X(numSamples, numFeatures);
    
    for (size_t i = 0; i < numSamples; ++i) {
        for (size_t j = 0; j < numFeatures; ++j) {
            X(i, j) = portfolio_.featureMatrix[i][j];
        }
    }
    
    // Perform k-means clustering
    size_t k = portfolio_.mlConfig.numRegimes;
    Eigen::MatrixXd centroids = X.topRows(k);
    Eigen::VectorXi labels = Eigen::VectorXi::Zero(numSamples);
    
    // Simple k-means implementation
    bool changed = true;
    while (changed) {
        changed = false;
        
        // Assign points to nearest centroid
        for (size_t i = 0; i < numSamples; ++i) {
            double minDist = std::numeric_limits<double>::max();
            int bestCluster = 0;
            
            for (size_t j = 0; j < k; ++j) {
                double dist = (X.row(i) - centroids.row(j)).squaredNorm();
                if (dist < minDist) {
                    minDist = dist;
                    bestCluster = j;
                }
            }
            
            if (labels(i) != bestCluster) {
                labels(i) = bestCluster;
                changed = true;
            }
        }
        
        // Update centroids
        for (size_t j = 0; j < k; ++j) {
            Eigen::MatrixXd clusterPoints = Eigen::MatrixXd::Zero(1, numFeatures);
            int count = 0;
            
            for (size_t i = 0; i < numSamples; ++i) {
                if (labels(i) == j) {
                    clusterPoints += X.row(i);
                    ++count;
                }
            }
            
            if (count > 0) {
                centroids.row(j) = clusterPoints / count;
            }
        }
    }
    
    // Store results
    portfolio_.regimeLabels.resize(numSamples);
    for (size_t i = 0; i < numSamples; ++i) {
        portfolio_.regimeLabels[i] = labels(i);
    }
    
    portfolio_.regimeCentroids.clear();
    for (size_t j = 0; j < k; ++j) {
        std::vector<double> centroid(numFeatures);
        for (size_t f = 0; f < numFeatures; ++f) {
            centroid[f] = centroids(j, f);
        }
        portfolio_.regimeCentroids[static_cast<MarketRegime>(j)] = centroid;
    }
}

MarketRegime PortfolioManager::predictRegime(const std::string& symbol) const {
    if (!portfolio_.mlConfig.isTrained) {
        return MarketRegime::UNKNOWN;
    }
    
    std::vector<double> features = extractFeatures(symbol);
    double minDist = std::numeric_limits<double>::max();
    MarketRegime bestRegime = MarketRegime::UNKNOWN;
    
    for (const auto& [regime, centroid] : portfolio_.regimeCentroids) {
        double dist = 0.0;
        for (size_t i = 0; i < features.size(); ++i) {
            dist += std::pow(features[i] - centroid[i], 2);
        }
        dist = std::sqrt(dist);
        
        if (dist < minDist) {
            minDist = dist;
            bestRegime = regime;
        }
    }
    
    return bestRegime;
}

std::map<MarketRegime, double> PortfolioManager::getRegimeProbabilities(const std::string& symbol) const {
    std::map<MarketRegime, double> probabilities;
    if (!portfolio_.mlConfig.isTrained) {
        return probabilities;
    }
    
    std::vector<double> features = extractFeatures(symbol);
    double totalDist = 0.0;
    
    for (const auto& [regime, centroid] : portfolio_.regimeCentroids) {
        double dist = 0.0;
        for (size_t i = 0; i < features.size(); ++i) {
            dist += std::pow(features[i] - centroid[i], 2);
        }
        dist = std::sqrt(dist);
        probabilities[regime] = dist;
        totalDist += dist;
    }
    
    // Convert distances to probabilities
    if (totalDist > 0.0) {
        for (auto& [regime, prob] : probabilities) {
            prob = 1.0 - (prob / totalDist);
        }
    }
    
    return probabilities;
}

// Dynamic risk budgeting implementation
void PortfolioManager::updateRiskBudget() {
    riskBudgets_.clear();
    double totalRisk = 0.0;
    
    for (const auto& [symbol, position] : portfolio_.positions) {
        double risk = calculateRiskBudget(symbol);
        riskBudgets_[symbol] = risk;
        totalRisk += risk;
    }
    
    // Normalize risk budgets
    if (totalRisk > 0.0) {
        for (auto& [symbol, risk] : riskBudgets_) {
            risk /= totalRisk;
        }
    }
}

double PortfolioManager::calculateRiskBudget(const std::string& symbol) const {
    if (riskBudgetMethod_ == "equal") {
        return calculateEqualRiskContribution(symbol);
    } else if (riskBudgetMethod_ == "parity") {
        return calculateRiskParity(symbol);
    } else if (riskBudgetMethod_ == "minvar") {
        return calculateMinimumVariance(symbol);
    } else if (riskBudgetMethod_ == "maxdiv") {
        return calculateMaximumDiversification(symbol);
    }
    return 0.0;
}

double PortfolioManager::calculateEqualRiskContribution(const std::string& symbol) const {
    return 1.0 / portfolio_.positions.size();
}

double PortfolioManager::calculateRiskParity(const std::string& symbol) const {
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    double volatility = calculateHistoricalVolatility(symbol);
    double correlation = calculatePortfolioCorrelation(symbol);
    
    // Risk parity with correlation adjustment
    return 1.0 / (volatility * (1.0 + correlation));
}

double PortfolioManager::calculateMinimumVariance(const std::string& symbol) const {
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    double variance = std::pow(calculateHistoricalVolatility(symbol), 2);
    double correlation = calculatePortfolioCorrelation(symbol);
    
    // Minimum variance with correlation adjustment
    return 1.0 / (variance * (1.0 + correlation));
}

double PortfolioManager::calculateMaximumDiversification(const std::string& symbol) const {
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    double diversification = 0.0;
    double totalCorrelation = 0.0;
    
    for (const auto& [otherSymbol, otherPosition] : portfolio_.positions) {
        if (symbol != otherSymbol) {
            double correlation = calculateCorrelation(symbol, otherSymbol);
            diversification += 1.0 - std::abs(correlation);
            totalCorrelation += std::abs(correlation);
        }
    }
    
    // Maximum diversification with correlation penalty
    return diversification / (1.0 + totalCorrelation);
}

double PortfolioManager::calculateRegimeAdjustedRisk(const std::string& symbol) const {
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    double baseRisk = calculateRiskParity(symbol);
    double regimeAdjustment = 1.0;
    
    // Adjust risk based on current regime
    switch (position.currentRegime) {
        case MarketRegime::TRENDING_UP:
            regimeAdjustment = 1.2; // Increase risk in trending up
            break;
        case MarketRegime::TRENDING_DOWN:
            regimeAdjustment = 0.8; // Decrease risk in trending down
            break;
        case MarketRegime::HIGH_VOLATILITY:
            regimeAdjustment = 0.7; // Decrease risk in high volatility
            break;
        case MarketRegime::LOW_VOLATILITY:
            regimeAdjustment = 1.3; // Increase risk in low volatility
            break;
        default:
            break;
    }
    
    return baseRisk * regimeAdjustment;
}

double PortfolioManager::calculateFactorAdjustedRisk(const std::string& symbol) const {
    auto it = portfolio_.positions.find(symbol);
    if (it == portfolio_.positions.end()) {
        return 0.0;
    }
    
    const auto& position = it->second;
    double baseRisk = calculateRiskParity(symbol);
    double factorAdjustment = 1.0;
    
    // Adjust risk based on factor scores
    for (const auto& [type, factor] : portfolio_.factors) {
        double score = factor.assetScores.at(symbol);
        factorAdjustment *= (1.0 + score * factor.weight);
    }
    
    return baseRisk * factorAdjustment;
}

void PortfolioManager::trainGMM() {
    // Convert feature matrix to Eigen matrix
    size_t numSamples = portfolio_.featureMatrix.size();
    size_t numFeatures = portfolio_.featureMatrix[0].size();
    Eigen::MatrixXd X(numSamples, numFeatures);
    
    for (size_t i = 0; i < numSamples; ++i) {
        for (size_t j = 0; j < numFeatures; ++j) {
            X(i, j) = portfolio_.featureMatrix[i][j];
        }
    }
    
    // Initialize GMM parameters
    size_t k = portfolio_.mlConfig.numRegimes;
    std::vector<Eigen::VectorXd> means(k);
    std::vector<Eigen::MatrixXd> covariances(k);
    std::vector<double> weights(k, 1.0 / k);
    
    // Initialize means using k-means++
    means[0] = X.row(0);
    for (size_t i = 1; i < k; ++i) {
        Eigen::VectorXd distances = Eigen::VectorXd::Zero(numSamples);
        for (size_t j = 0; j < numSamples; ++j) {
            double minDist = std::numeric_limits<double>::max();
            for (size_t m = 0; m < i; ++m) {
                double dist = (X.row(j) - means[m]).squaredNorm();
                minDist = std::min(minDist, dist);
            }
            distances(j) = minDist;
        }
        double sumDist = distances.sum();
        double r = static_cast<double>(rand()) / RAND_MAX * sumDist;
        double cumsum = 0.0;
        for (size_t j = 0; j < numSamples; ++j) {
            cumsum += distances(j);
            if (cumsum >= r) {
                means[i] = X.row(j);
                break;
            }
        }
    }
    
    // Initialize covariances
    for (size_t i = 0; i < k; ++i) {
        covariances[i] = Eigen::MatrixXd::Identity(numFeatures, numFeatures);
    }
    
    // EM algorithm
    const size_t maxIterations = 100;
    const double tolerance = 1e-6;
    double prevLogLikelihood = -std::numeric_limits<double>::max();
    
    for (size_t iter = 0; iter < maxIterations; ++iter) {
        // E-step: Calculate responsibilities
        Eigen::MatrixXd responsibilities(numSamples, k);
        for (size_t i = 0; i < numSamples; ++i) {
            double sum = 0.0;
            for (size_t j = 0; j < k; ++j) {
                Eigen::VectorXd diff = X.row(i) - means[j];
                double exponent = -0.5 * diff.transpose() * covariances[j].inverse() * diff;
                double det = covariances[j].determinant();
                responsibilities(i, j) = weights[j] * std::exp(exponent) / std::sqrt(det);
                sum += responsibilities(i, j);
            }
            responsibilities.row(i) /= sum;
        }
        
        // M-step: Update parameters
        for (size_t j = 0; j < k; ++j) {
            double sumResponsibility = responsibilities.col(j).sum();
            weights[j] = sumResponsibility / numSamples;
            
            // Update means
            means[j] = Eigen::VectorXd::Zero(numFeatures);
            for (size_t i = 0; i < numSamples; ++i) {
                means[j] += responsibilities(i, j) * X.row(i);
            }
            means[j] /= sumResponsibility;
            
            // Update covariances
            covariances[j] = Eigen::MatrixXd::Zero(numFeatures, numFeatures);
            for (size_t i = 0; i < numSamples; ++i) {
                Eigen::VectorXd diff = X.row(i) - means[j];
                covariances[j] += responsibilities(i, j) * diff * diff.transpose();
            }
            covariances[j] /= sumResponsibility;
        }
        
        // Calculate log-likelihood
        double logLikelihood = 0.0;
        for (size_t i = 0; i < numSamples; ++i) {
            double sum = 0.0;
            for (size_t j = 0; j < k; ++j) {
                Eigen::VectorXd diff = X.row(i) - means[j];
                double exponent = -0.5 * diff.transpose() * covariances[j].inverse() * diff;
                double det = covariances[j].determinant();
                sum += weights[j] * std::exp(exponent) / std::sqrt(det);
            }
            logLikelihood += std::log(sum);
        }
        
        // Check convergence
        if (std::abs(logLikelihood - prevLogLikelihood) < tolerance) {
            break;
        }
        prevLogLikelihood = logLikelihood;
    }
    
    // Store results
    portfolio_.regimeLabels.resize(numSamples);
    for (size_t i = 0; i < numSamples; ++i) {
        Eigen::VectorXd::Index maxIndex;
        responsibilities.row(i).maxCoeff(&maxIndex);
        portfolio_.regimeLabels[i] = maxIndex;
    }
    
    portfolio_.regimeCentroids.clear();
    for (size_t j = 0; j < k; ++j) {
        std::vector<double> centroid(numFeatures);
        for (size_t f = 0; f < numFeatures; ++f) {
            centroid[f] = means[j](f);
        }
        portfolio_.regimeCentroids[static_cast<MarketRegime>(j)] = centroid;
    }
}

void PortfolioManager::trainHMM() {
    // Convert feature matrix to Eigen matrix
    size_t numSamples = portfolio_.featureMatrix.size();
    size_t numFeatures = portfolio_.featureMatrix[0].size();
    Eigen::MatrixXd X(numSamples, numFeatures);
    
    for (size_t i = 0; i < numSamples; ++i) {
        for (size_t j = 0; j < numFeatures; ++j) {
            X(i, j) = portfolio_.featureMatrix[i][j];
        }
    }
    
    // Initialize HMM parameters
    size_t k = portfolio_.mlConfig.numRegimes;
    Eigen::MatrixXd transition = Eigen::MatrixXd::Constant(k, k, 1.0 / k);
    std::vector<Eigen::VectorXd> means(k);
    std::vector<Eigen::MatrixXd> covariances(k);
    Eigen::VectorXd initial(k);
    initial.setConstant(1.0 / k);
    
    // Initialize means and covariances using k-means
    trainKMeans(); // Reuse k-means initialization
    
    // Baum-Welch algorithm
    const size_t maxIterations = 100;
    const double tolerance = 1e-6;
    double prevLogLikelihood = -std::numeric_limits<double>::max();
    
    for (size_t iter = 0; iter < maxIterations; ++iter) {
        // Forward pass
        Eigen::MatrixXd alpha(numSamples, k);
        for (size_t i = 0; i < k; ++i) {
            Eigen::VectorXd diff = X.row(0) - means[i];
            double exponent = -0.5 * diff.transpose() * covariances[i].inverse() * diff;
            double det = covariances[i].determinant();
            alpha(0, i) = initial(i) * std::exp(exponent) / std::sqrt(det);
        }
        
        for (size_t t = 1; t < numSamples; ++t) {
            for (size_t i = 0; i < k; ++i) {
                double sum = 0.0;
                for (size_t j = 0; j < k; ++j) {
                    sum += alpha(t-1, j) * transition(j, i);
                }
                Eigen::VectorXd diff = X.row(t) - means[i];
                double exponent = -0.5 * diff.transpose() * covariances[i].inverse() * diff;
                double det = covariances[i].determinant();
                alpha(t, i) = sum * std::exp(exponent) / std::sqrt(det);
            }
        }
        
        // Backward pass
        Eigen::MatrixXd beta(numSamples, k);
        beta.row(numSamples-1).setConstant(1.0);
        
        for (size_t t = numSamples-2; t >= 0; --t) {
            for (size_t i = 0; i < k; ++i) {
                double sum = 0.0;
                for (size_t j = 0; j < k; ++j) {
                    Eigen::VectorXd diff = X.row(t+1) - means[j];
                    double exponent = -0.5 * diff.transpose() * covariances[j].inverse() * diff;
                    double det = covariances[j].determinant();
                    sum += transition(i, j) * beta(t+1, j) * 
                          std::exp(exponent) / std::sqrt(det);
                }
                beta(t, i) = sum;
            }
        }
        
        // Update parameters
        for (size_t i = 0; i < k; ++i) {
            // Update means
            means[i] = Eigen::VectorXd::Zero(numFeatures);
            double sumGamma = 0.0;
            for (size_t t = 0; t < numSamples; ++t) {
                double gamma = alpha(t, i) * beta(t, i);
                means[i] += gamma * X.row(t);
                sumGamma += gamma;
            }
            means[i] /= sumGamma;
            
            // Update covariances
            covariances[i] = Eigen::MatrixXd::Zero(numFeatures, numFeatures);
            for (size_t t = 0; t < numSamples; ++t) {
                double gamma = alpha(t, i) * beta(t, i);
                Eigen::VectorXd diff = X.row(t) - means[i];
                covariances[i] += gamma * diff * diff.transpose();
            }
            covariances[i] /= sumGamma;
            
            // Update transition probabilities
            for (size_t j = 0; j < k; ++j) {
                double sum = 0.0;
                for (size_t t = 0; t < numSamples-1; ++t) {
                    Eigen::VectorXd diff = X.row(t+1) - means[j];
                    double exponent = -0.5 * diff.transpose() * covariances[j].inverse() * diff;
                    double det = covariances[j].determinant();
                    sum += alpha(t, i) * transition(i, j) * beta(t+1, j) * 
                          std::exp(exponent) / std::sqrt(det);
                }
                transition(i, j) = sum;
            }
            transition.row(i) /= transition.row(i).sum();
        }
        
        // Calculate log-likelihood
        double logLikelihood = 0.0;
        for (size_t t = 0; t < numSamples; ++t) {
            double sum = 0.0;
            for (size_t i = 0; i < k; ++i) {
                sum += alpha(t, i) * beta(t, i);
            }
            logLikelihood += std::log(sum);
        }
        
        // Check convergence
        if (std::abs(logLikelihood - prevLogLikelihood) < tolerance) {
            break;
        }
        prevLogLikelihood = logLikelihood;
    }
    
    // Store results
    portfolio_.regimeLabels.resize(numSamples);
    for (size_t t = 0; t < numSamples; ++t) {
        Eigen::VectorXd::Index maxIndex;
        alpha.row(t).maxCoeff(&maxIndex);
        portfolio_.regimeLabels[t] = maxIndex;
    }
    
    portfolio_.regimeCentroids.clear();
    for (size_t j = 0; j < k; ++j) {
        std::vector<double> centroid(numFeatures);
        for (size_t f = 0; f < numFeatures; ++f) {
            centroid[f] = means[j](f);
        }
        portfolio_.regimeCentroids[static_cast<MarketRegime>(j)] = centroid;
    }
}

void PortfolioManager::trainRandomForest() {
    // Convert feature matrix to Eigen matrix
    size_t numSamples = portfolio_.featureMatrix.size();
    size_t numFeatures = portfolio_.featureMatrix[0].size();
    Eigen::MatrixXd X(numSamples, numFeatures);
    Eigen::VectorXi y(numSamples);
    
    for (size_t i = 0; i < numSamples; ++i) {
        for (size_t j = 0; j < numFeatures; ++j) {
            X(i, j) = portfolio_.featureMatrix[i][j];
        }
        y(i) = portfolio_.regimeLabels[i];
    }
    
    // Initialize random forest parameters
    size_t numTrees = 100;
    size_t maxDepth = 10;
    size_t minSamplesSplit = 2;
    size_t minSamplesLeaf = 1;
    
    // Train random forest
    std::vector<std::vector<std::vector<double>>> trees(numTrees);
    std::vector<std::vector<int>> featureIndices(numTrees);
    
    for (size_t tree = 0; tree < numTrees; ++tree) {
        // Bootstrap sample
        Eigen::MatrixXd X_bootstrap(numSamples, numFeatures);
        Eigen::VectorXi y_bootstrap(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            size_t idx = rand() % numSamples;
            X_bootstrap.row(i) = X.row(idx);
            y_bootstrap(i) = y(idx);
        }
        
        // Random feature selection
        std::vector<int> features(numFeatures);
        std::iota(features.begin(), features.end(), 0);
        std::random_shuffle(features.begin(), features.end());
        size_t numSelectedFeatures = std::sqrt(numFeatures);
        features.resize(numSelectedFeatures);
        featureIndices[tree] = features;
        
        // Train decision tree
        std::vector<std::vector<double>> tree;
        trainDecisionTree(X_bootstrap, y_bootstrap, features, maxDepth, 
                         minSamplesSplit, minSamplesLeaf, tree);
        trees[tree] = tree;
    }
    
    // Store results
    portfolio_.mlConfig.hyperparameters["num_trees"] = numTrees;
    portfolio_.mlConfig.hyperparameters["max_depth"] = maxDepth;
    portfolio_.mlConfig.hyperparameters["min_samples_split"] = minSamplesSplit;
    portfolio_.mlConfig.hyperparameters["min_samples_leaf"] = minSamplesLeaf;
}

void PortfolioManager::trainSVM() {
    // Convert feature matrix to Eigen matrix
    size_t numSamples = portfolio_.featureMatrix.size();
    size_t numFeatures = portfolio_.featureMatrix[0].size();
    Eigen::MatrixXd X(numSamples, numFeatures);
    Eigen::VectorXi y(numSamples);
    
    for (size_t i = 0; i < numSamples; ++i) {
        for (size_t j = 0; j < numFeatures; ++j) {
            X(i, j) = portfolio_.featureMatrix[i][j];
        }
        y(i) = portfolio_.regimeLabels[i];
    }
    
    // Initialize SVM parameters
    double C = 1.0;
    double gamma = 1.0 / numFeatures;
    
    // Train SVM using SMO algorithm
    Eigen::VectorXd alpha = Eigen::VectorXd::Zero(numSamples);
    Eigen::VectorXd w = Eigen::VectorXd::Zero(numFeatures);
    double b = 0.0;
    
    const size_t maxIterations = 1000;
    const double tolerance = 1e-6;
    
    for (size_t iter = 0; iter < maxIterations; ++iter) {
        bool changed = false;
        
        for (size_t i = 0; i < numSamples; ++i) {
            double Ei = (w.transpose() * X.row(i).transpose() + b) - y(i);
            
            if ((y(i) * Ei < -tolerance && alpha(i) < C) ||
                (y(i) * Ei > tolerance && alpha(i) > 0)) {
                
                size_t j = rand() % numSamples;
                while (j == i) j = rand() % numSamples;
                
                double Ej = (w.transpose() * X.row(j).transpose() + b) - y(j);
                
                double old_alpha_i = alpha(i);
                double old_alpha_j = alpha(j);
                
                double L, H;
                if (y(i) != y(j)) {
                    L = std::max(0.0, alpha(j) - alpha(i));
                    H = std::min(C, C + alpha(j) - alpha(i));
                } else {
                    L = std::max(0.0, alpha(i) + alpha(j) - C);
                    H = std::min(C, alpha(i) + alpha(j));
                }
                
                if (L == H) continue;
                
                double eta = 2.0 * X.row(i) * X.row(j).transpose() -
                            X.row(i) * X.row(i).transpose() -
                            X.row(j) * X.row(j).transpose();
                
                if (eta >= 0) continue;
                
                alpha(j) -= y(j) * (Ei - Ej) / eta;
                alpha(j) = std::min(H, std::max(L, alpha(j)));
                
                if (std::abs(alpha(j) - old_alpha_j) < tolerance) continue;
                
                alpha(i) += y(i) * y(j) * (old_alpha_j - alpha(j));
                
                double b1 = b - Ei - y(i) * (alpha(i) - old_alpha_i) * 
                           X.row(i) * X.row(i).transpose() -
                           y(j) * (alpha(j) - old_alpha_j) * 
                           X.row(i) * X.row(j).transpose();
                
                double b2 = b - Ej - y(i) * (alpha(i) - old_alpha_i) * 
                           X.row(i) * X.row(j).transpose() -
                           y(j) * (alpha(j) - old_alpha_j) * 
                           X.row(j) * X.row(j).transpose();
                
                if (0 < alpha(i) && alpha(i) < C) b = b1;
                else if (0 < alpha(j) && alpha(j) < C) b = b2;
                else b = (b1 + b2) / 2.0;
                
                changed = true;
            }
        }
        
        if (!changed) break;
    }
    
    // Store results
    w = Eigen::VectorXd::Zero(numFeatures);
    for (size_t i = 0; i < numSamples; ++i) {
        w += alpha(i) * y(i) * X.row(i).transpose();
    }
    
    portfolio_.regimeCentroids.clear();
    for (size_t i = 0; i < portfolio_.mlConfig.numRegimes; ++i) {
        std::vector<double> centroid(numFeatures);
        for (size_t f = 0; f < numFeatures; ++f) {
            centroid[f] = w(f);
        }
        portfolio_.regimeCentroids[static_cast<MarketRegime>(i)] = centroid;
    }
}

void PortfolioManager::trainNeuralNetwork() {
    // Convert feature matrix to Eigen matrix
    size_t numSamples = portfolio_.featureMatrix.size();
    size_t numFeatures = portfolio_.featureMatrix[0].size();
    Eigen::MatrixXd X(numSamples, numFeatures);
    Eigen::MatrixXd y(numSamples, portfolio_.mlConfig.numRegimes);
    y.setZero();
    
    for (size_t i = 0; i < numSamples; ++i) {
        for (size_t j = 0; j < numFeatures; ++j) {
            X(i, j) = portfolio_.featureMatrix[i][j];
        }
        y(i, portfolio_.regimeLabels[i]) = 1.0;
    }
    
    // Initialize neural network parameters
    size_t hiddenSize = 64;
    double learningRate = 0.01;
    size_t numEpochs = 100;
    
    // Initialize weights
    Eigen::MatrixXd W1 = Eigen::MatrixXd::Random(numFeatures, hiddenSize) * 0.01;
    Eigen::VectorXd b1 = Eigen::VectorXd::Zero(hiddenSize);
    Eigen::MatrixXd W2 = Eigen::MatrixXd::Random(hiddenSize, portfolio_.mlConfig.numRegimes) * 0.01;
    Eigen::VectorXd b2 = Eigen::VectorXd::Zero(portfolio_.mlConfig.numRegimes);
    
    // Training loop
    for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
        // Forward pass
        Eigen::MatrixXd hidden = (X * W1).rowwise() + b1.transpose();
        hidden = hidden.unaryExpr([](double x) { return 1.0 / (1.0 + std::exp(-x)); });
        Eigen::MatrixXd output = (hidden * W2).rowwise() + b2.transpose();
        output = output.unaryExpr([](double x) { return std::exp(x); });
        Eigen::VectorXd rowSums = output.rowwise().sum();
        for (size_t i = 0; i < numSamples; ++i) {
            output.row(i) /= rowSums(i);
        }
        
        // Backward pass
        Eigen::MatrixXd dOutput = output - y;
        Eigen::MatrixXd dHidden = (dOutput * W2.transpose()).cwiseProduct(
            hidden.cwiseProduct(1.0 - hidden));
        
        // Update weights
        W2 -= learningRate * hidden.transpose() * dOutput / numSamples;
        b2 -= learningRate * dOutput.colwise().sum().transpose() / numSamples;
        W1 -= learningRate * X.transpose() * dHidden / numSamples;
        b1 -= learningRate * dHidden.colwise().sum().transpose() / numSamples;
    }
    
    // Store results
    portfolio_.regimeCentroids.clear();
    for (size_t i = 0; i < portfolio_.mlConfig.numRegimes; ++i) {
        std::vector<double> centroid(numFeatures);
        for (size_t f = 0; f < numFeatures; ++f) {
            centroid[f] = W1(f, i);
        }
        portfolio_.regimeCentroids[static_cast<MarketRegime>(i)] = centroid;
    }
}

} // namespace portfolio
} // namespace tradingbot 