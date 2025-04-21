#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "../core/TradingEngine.hpp"
#include "../performance/PerformanceTracker.hpp"

namespace tradingbot {
namespace visualization {

struct TradeRecord {
    std::string symbol;
    std::string entryTime;
    std::string exitTime;
    double entryPrice;
    double exitPrice;
    double quantity;
    double pnl;
    bool isWin;
};

struct DailyStats {
    std::string date;
    double pnl;
    double equity;
    int trades;
    int winningTrades;
    double winRate;
    double drawdown;  // Added for drawdown visualization
};

class BacktestReporter {
public:
    BacktestReporter(const std::string& configPath);
    ~BacktestReporter();

    // Initialize reporter with configuration
    void initialize();

    // Generate reports for a completed backtest
    void generateReports(const core::TradingEngine& engine, 
                        const performance::PerformanceMetrics& metrics);

    // Export trade records to CSV
    void exportTradesToCSV(const std::vector<TradeRecord>& trades);

    // Export daily statistics to CSV
    void exportDailyStatsToCSV(const std::vector<DailyStats>& stats);

    // Export performance metrics to JSON
    void exportMetricsToJSON(const performance::PerformanceMetrics& metrics);

    // Generate equity curve plot
    void generateEquityCurvePlot(const std::vector<DailyStats>& stats);

    // Generate trade markers plot
    void generateTradeMarkersPlot(const std::vector<TradeRecord>& trades);

    // Generate drawdown visualization
    void generateDrawdownPlot(const std::vector<DailyStats>& stats);

    // Generate trade distribution plots
    void generateTradeDistributionPlots(const std::vector<TradeRecord>& trades);

    // Generate monthly performance heatmap
    void generateMonthlyPerformanceHeatmap(const std::vector<DailyStats>& stats);

    // Generate correlation matrix
    void generateCorrelationMatrix(const std::vector<TradeRecord>& trades);

private:
    std::string configPath_;
    std::string outputFolder_;
    bool enableCSV_;
    bool enablePlot_;
    
    // Helper methods
    void createOutputDirectory();
    std::string getCurrentTimestamp();
    std::string formatDateTime(const std::string& timestamp);
    void validateConfiguration();
    
    // Plot helper methods
    void setupPlotStyle();
    void savePlot(const std::string& filename);
    std::vector<double> calculateDrawdown(const std::vector<double>& equity);
};

} // namespace visualization
} // namespace tradingbot 