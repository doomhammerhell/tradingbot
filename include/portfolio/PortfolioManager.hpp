#pragma once

#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <vector>
#include <deque>
#include <nlohmann/json.hpp>
#include <Eigen/Dense>

namespace tradingbot {
namespace portfolio {

// Market regime types
enum class MarketRegime {
    TRENDING_UP,
    TRENDING_DOWN,
    RANGING,
    HIGH_VOLATILITY,
    LOW_VOLATILITY,
    UNKNOWN
};

// Risk parity allocation method
enum class RiskParityMethod {
    EQUAL_RISK_CONTRIBUTION,
    INVERSE_VOLATILITY,
    MINIMUM_VARIANCE,
    MAXIMUM_DIVERSIFICATION
};

// Factor types for factor-based allocation
enum class FactorType {
    VALUE,
    MOMENTUM,
    VOLATILITY,
    QUALITY,
    SIZE,
    LIQUIDITY,
    CUSTOM
};

// ML model types for regime detection
enum class MLModelType {
    KMEANS,
    GMM,
    HMM,
    RANDOM_FOREST,
    SVM,
    NEURAL_NETWORK
};

struct Factor {
    FactorType type;
    double weight;
    double score;
    std::vector<double> historicalScores;
    std::map<std::string, double> assetScores;
    std::chrono::system_clock::time_point lastUpdate;
};

struct MLModelConfig {
    MLModelType type;
    std::map<std::string, double> hyperparameters;
    size_t lookbackPeriod;
    size_t numRegimes;
    bool isTrained;
    std::chrono::system_clock::time_point lastTrainingTime;
};

struct FactorAllocation {
    double targetWeight;
    double currentWeight;
    double factorScore;
    double factorContribution;
    std::map<FactorType, double> factorExposures;
    std::map<MarketRegime, double> regimeWeights;
};

struct Position {
    std::string symbol;
    double quantity;
    double averagePrice;
    double currentPrice;
    double unrealizedPnL;
    double realizedPnL;
    std::chrono::system_clock::time_point entryTime;
    std::chrono::system_clock::time_point lastUpdateTime;
    double stopLoss;
    double takeProfit;
    double volatility;  // 20-day historical volatility
    double correlation; // Correlation with portfolio
    double riskContribution; // Contribution to portfolio risk
    double drawdown; // Current drawdown from peak
    double peakValue; // Peak value of the position
    MarketRegime currentRegime; // Current market regime
    std::map<FactorType, double> factorExposures;
    std::map<FactorType, double> factorScores;
    double regimeProbability;
    std::vector<double> featureVector;
};

struct AssetAllocation {
    double targetWeight;
    double currentWeight;
    double maxDeviation;
    double minWeight;
    double maxWeight;
    double targetVolatility;  // Target volatility for this asset
    double correlationFactor; // Correlation adjustment factor
    double riskParityWeight; // Risk parity based weight
    double regimeAdjustedWeight; // Weight adjusted for current regime
    std::map<MarketRegime, double> regimeWeights; // Weights for different regimes
};

struct Portfolio {
    double totalBalance;
    double availableBalance;
    double totalPnL;
    double dailyPnL;
    double maxDrawdown;
    double currentDrawdown;
    double riskFreeRate;
    double targetVolatility;  // Portfolio-wide target volatility
    double currentVolatility; // Current portfolio volatility
    double correlationThreshold; // Maximum allowed correlation
    double maxAllowedDrawdown; // Maximum allowed drawdown
    double riskParityTarget; // Target risk contribution
    MarketRegime currentRegime; // Current market regime
    RiskParityMethod riskParityMethod; // Method for risk parity allocation
    std::map<std::string, Position> positions;
    std::map<std::string, AssetAllocation> assetAllocation;
    std::chrono::system_clock::time_point lastRebalanceTime;
    std::map<std::string, std::vector<double>> historicalReturns; // For correlation calculation
    std::map<std::string, std::deque<double>> rollingReturns; // For rolling calculations
    std::map<std::pair<std::string, std::string>, std::deque<double>> rollingCorrelations; // Rolling correlations
    std::map<MarketRegime, std::chrono::system_clock::time_point> regimeStartTimes; // Regime tracking
    std::map<FactorType, Factor> factors;
    MLModelConfig mlConfig;
    std::map<std::string, FactorAllocation> factorAllocation;
    std::vector<std::vector<double>> featureMatrix;
    std::vector<int> regimeLabels;
    std::map<MarketRegime, double> regimeProbabilities;
    std::map<MarketRegime, std::vector<double>> regimeCentroids;
};

class PortfolioManager {
public:
    PortfolioManager();
    
    // Initialize with configuration
    void initialize(const std::string& config);
    
    // Position management
    void updatePosition(const std::string& symbol, double quantity, double price, 
                       double stopLoss = 0.0, double takeProfit = 0.0);
    void closePosition(const std::string& symbol, double price);
    Position getPosition(const std::string& symbol) const;
    
    // Risk management
    double calculatePositionSize(const std::string& symbol, double price, 
                               double stopLoss, double riskAmount) const;
    double calculateRiskPerTrade(double accountRisk) const;
    bool checkPositionLimits(const std::string& symbol, double quantity) const;
    
    // Advanced position sizing
    double calculateCorrelationAdjustedSize(const std::string& symbol, double price,
                                         double stopLoss, double riskAmount) const;
    double calculateVolatilityAdjustedSize(const std::string& symbol, double price,
                                         double riskAmount) const;
    double calculateOptimalPositionSize(const std::string& symbol, double price,
                                      double stopLoss, double riskAmount) const;
    
    // Volatility targeting
    void setTargetVolatility(double targetVol);
    void updateVolatilityMetrics();
    bool isVolatilityWithinTarget() const;
    std::vector<std::pair<std::string, double>> calculateVolatilityAdjustments() const;
    
    // Correlation management
    void updateCorrelationMetrics();
    double calculatePortfolioCorrelation(const std::string& symbol) const;
    bool isCorrelationAcceptable(const std::string& symbol) const;
    
    // Advanced correlation calculations
    double calculateRollingCorrelation(const std::string& symbol1, const std::string& symbol2,
                                     size_t window = 20) const;
    double calculateRegimeAdjustedCorrelation(const std::string& symbol1, 
                                            const std::string& symbol2) const;
    std::map<MarketRegime, double> calculateRegimeCorrelations(const std::string& symbol1,
                                                             const std::string& symbol2) const;
    
    // Market regime detection
    MarketRegime detectMarketRegime(const std::string& symbol) const;
    void updateMarketRegimes();
    double calculateRegimeProbability(MarketRegime regime) const;
    std::map<MarketRegime, double> getRegimeProbabilities() const;
    
    // Drawdown control
    void setMaxDrawdown(double maxDrawdown);
    bool isDrawdownWithinLimits() const;
    std::vector<std::pair<std::string, double>> calculateDrawdownAdjustments() const;
    void updateDrawdownMetrics();
    
    // Risk parity
    void setRiskParityMethod(RiskParityMethod method);
    void calculateRiskParityWeights();
    double calculateRiskContribution(const std::string& symbol) const;
    std::vector<std::pair<std::string, double>> calculateRiskParityAdjustments() const;
    
    // Asset allocation
    void setAssetAllocation(const std::string& symbol, double targetWeight, 
                           double maxDeviation = 0.1, double minWeight = 0.0, 
                           double maxWeight = 1.0);
    bool needsRebalancing() const;
    std::vector<std::pair<std::string, double>> calculateRebalanceOrders() const;
    
    // Performance metrics
    double calculateSharpeRatio() const;
    double calculateSortinoRatio() const;
    double calculateMaxDrawdown() const;
    double calculateWinRate() const;
    double calculateProfitFactor() const;
    double calculateCalmarRatio() const;
    double calculateOmegaRatio() const;
    double calculateTailRatio() const;
    
    // Portfolio state
    Portfolio getPortfolio() const;
    void updatePortfolioMetrics();
    std::string getMetrics() const;
    
    // Factor-based allocation
    void addFactor(FactorType type, double weight);
    void removeFactor(FactorType type);
    void updateFactorScores();
    void calculateFactorExposures();
    std::vector<std::pair<std::string, double>> calculateFactorBasedAdjustments() const;
    double calculateFactorScore(const std::string& symbol, FactorType factor) const;
    void setFactorWeights(const std::map<FactorType, double>& weights);
    std::map<FactorType, double> getFactorWeights() const;
    std::map<std::string, double> getFactorScores(FactorType factor) const;

    // Machine learning-based regime detection
    void initializeMLModel(MLModelType type, const std::map<std::string, double>& hyperparameters);
    void trainMLModel();
    void updateMLFeatures();
    MarketRegime predictRegime(const std::string& symbol) const;
    std::map<MarketRegime, double> getRegimeProbabilities(const std::string& symbol) const;
    void updateRegimeCentroids();
    std::vector<double> extractFeatures(const std::string& symbol) const;
    double calculateRegimeDistance(const std::string& symbol, MarketRegime regime) const;
    void setMLHyperparameters(const std::map<std::string, double>& hyperparameters);
    std::map<std::string, double> getMLHyperparameters() const;

    // Dynamic risk budgeting
    void updateRiskBudget();
    double calculateRiskBudget(const std::string& symbol) const;
    std::vector<std::pair<std::string, double>> calculateRiskBudgetAdjustments() const;
    void setRiskBudgetMethod(const std::string& method);
    std::string getRiskBudgetMethod() const;

private:
    void updateAssetAllocation();
    void updatePositionMetrics(Position& position);
    double calculatePositionValue(const Position& position) const;
    double calculatePortfolioValue() const;
    double calculateDailyReturns() const;
    double calculateHistoricalVolatility(const std::string& symbol) const;
    double calculateCorrelation(const std::string& symbol1, const std::string& symbol2) const;
    void updateHistoricalReturns(const std::string& symbol, double returnValue);
    void updateRollingMetrics(const std::string& symbol, double returnValue);
    MarketRegime detectRegimeFromMetrics(double returns, double volatility, 
                                       double correlation) const;
    double calculateRegimeTransitionProbability(MarketRegime from, MarketRegime to) const;
    void updateRiskContributions();
    
    // Factor-based allocation helpers
    double calculateValueFactor(const std::string& symbol) const;
    double calculateMomentumFactor(const std::string& symbol) const;
    double calculateVolatilityFactor(const std::string& symbol) const;
    double calculateQualityFactor(const std::string& symbol) const;
    double calculateSizeFactor(const std::string& symbol) const;
    double calculateLiquidityFactor(const std::string& symbol) const;
    void updateFactorHistoricalScores(Factor& factor);
    void normalizeFactorScores(Factor& factor);

    // ML helpers
    void trainKMeans();
    void trainGMM();
    void trainHMM();
    void trainRandomForest();
    void trainSVM();
    void trainNeuralNetwork();
    std::vector<double> preprocessFeatures(const std::vector<double>& features) const;
    void updateFeatureMatrix();
    void updateRegimeLabels();
    void calculateRegimeProbabilities();
    void updateModelPerformance();

    // Risk budgeting helpers
    double calculateEqualRiskContribution(const std::string& symbol) const;
    double calculateRiskParity(const std::string& symbol) const;
    double calculateMinimumVariance(const std::string& symbol) const;
    double calculateMaximumDiversification(const std::string& symbol) const;
    void normalizeRiskBudgets();

    Portfolio portfolio_;
    std::vector<double> dailyReturns_;
    std::vector<double> tradeResults_;
    nlohmann::json config_;
    std::string riskBudgetMethod_;
    std::map<std::string, double> riskBudgets_;
}; 