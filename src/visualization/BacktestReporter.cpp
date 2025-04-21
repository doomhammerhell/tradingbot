#include "visualization/BacktestReporter.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <matplotlibcpp.h>
#include <algorithm>
#include <numeric>

namespace plt = matplotlibcpp;

namespace tradingbot {
namespace visualization {

BacktestReporter::BacktestReporter(const std::string& configPath)
    : configPath_(configPath), enableCSV_(true), enablePlot_(true) {
}

BacktestReporter::~BacktestReporter() {
}

void BacktestReporter::initialize() {
    // Load configuration from JSON file
    std::ifstream configFile(configPath_);
    if (!configFile.is_open()) {
        throw std::runtime_error("Failed to open configuration file: " + configPath_);
    }

    nlohmann::json config;
    configFile >> config;

    // Parse configuration
    if (config.contains("reporting")) {
        const auto& reporting = config["reporting"];
        enableCSV_ = reporting.value("enable_csv", true);
        enablePlot_ = reporting.value("enable_plot", true);
        outputFolder_ = reporting.value("output_folder", "reports/");
    }

    validateConfiguration();
    createOutputDirectory();
}

void BacktestReporter::generateReports(const core::TradingEngine& engine,
                                     const performance::PerformanceMetrics& metrics) {
    // Convert engine data to trade records
    std::vector<TradeRecord> trades;
    // TODO: Convert engine trades to TradeRecord format

    // Calculate daily statistics
    std::vector<DailyStats> dailyStats;
    // TODO: Calculate daily statistics from trades

    // Generate reports based on configuration
    if (enableCSV_) {
        exportTradesToCSV(trades);
        exportDailyStatsToCSV(dailyStats);
        exportMetricsToJSON(metrics);
    }

    if (enablePlot_) {
        generateEquityCurvePlot(dailyStats);
        generateTradeMarkersPlot(trades);
        generateDrawdownPlot(dailyStats);
        generateTradeDistributionPlots(trades);
        generateMonthlyPerformanceHeatmap(dailyStats);
        generateCorrelationMatrix(trades);
    }
}

void BacktestReporter::exportTradesToCSV(const std::vector<TradeRecord>& trades) {
    std::string filename = outputFolder_ + getCurrentTimestamp() + "/trades.csv";
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    // Write CSV header
    file << "Symbol,EntryTime,ExitTime,EntryPrice,ExitPrice,Quantity,PNL,IsWin\n";

    // Write trade records
    for (const auto& trade : trades) {
        file << trade.symbol << ","
             << trade.entryTime << ","
             << trade.exitTime << ","
             << trade.entryPrice << ","
             << trade.exitPrice << ","
             << trade.quantity << ","
             << trade.pnl << ","
             << (trade.isWin ? "true" : "false") << "\n";
    }
}

void BacktestReporter::exportDailyStatsToCSV(const std::vector<DailyStats>& stats) {
    std::string filename = outputFolder_ + getCurrentTimestamp() + "/daily_stats.csv";
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    // Write CSV header
    file << "Date,PNL,Equity,Trades,WinningTrades,WinRate\n";

    // Write daily statistics
    for (const auto& stat : stats) {
        file << stat.date << ","
             << stat.pnl << ","
             << stat.equity << ","
             << stat.trades << ","
             << stat.winningTrades << ","
             << stat.winRate << "\n";
    }
}

void BacktestReporter::exportMetricsToJSON(const performance::PerformanceMetrics& metrics) {
    std::string filename = outputFolder_ + getCurrentTimestamp() + "/metrics.json";
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    nlohmann::json jsonMetrics = {
        {"totalTrades", metrics.totalTrades},
        {"winningTrades", metrics.winningTrades},
        {"losingTrades", metrics.losingTrades},
        {"winRatio", metrics.winRatio},
        {"totalPnL", metrics.totalPnL},
        {"maxDrawdown", metrics.maxDrawdown},
        {"sharpeRatio", metrics.sharpeRatio},
        {"sortinoRatio", metrics.sortinoRatio},
        {"profitFactor", metrics.profitFactor},
        {"averageTrade", metrics.averageTrade},
        {"averageWin", metrics.averageWin},
        {"averageLoss", metrics.averageLoss},
        {"largestWin", metrics.largestWin},
        {"largestLoss", metrics.largestLoss},
        {"averageTradeDuration", metrics.averageTradeDuration}
    };

    file << jsonMetrics.dump(4);
}

void BacktestReporter::generateEquityCurvePlot(const std::vector<DailyStats>& stats) {
    std::vector<double> dates;
    std::vector<double> equity;
    
    for (const auto& stat : stats) {
        // Convert date string to numeric value for plotting
        dates.push_back(std::stod(stat.date));
        equity.push_back(stat.equity);
    }

    plt::figure();
    plt::plot(dates, equity, "b-");
    plt::title("Equity Curve");
    plt::xlabel("Date");
    plt::ylabel("Equity");
    plt::grid(true);
    
    std::string filename = outputFolder_ + getCurrentTimestamp() + "/equity_curve.png";
    plt::save(filename);
    plt::close();
}

void BacktestReporter::generateTradeMarkersPlot(const std::vector<TradeRecord>& trades) {
    std::vector<double> entryDates;
    std::vector<double> entryPrices;
    std::vector<double> exitDates;
    std::vector<double> exitPrices;
    std::vector<std::string> colors;

    for (const auto& trade : trades) {
        // Convert date strings to numeric values
        entryDates.push_back(std::stod(trade.entryTime));
        entryPrices.push_back(trade.entryPrice);
        exitDates.push_back(std::stod(trade.exitTime));
        exitPrices.push_back(trade.exitPrice);
        colors.push_back(trade.isWin ? "g" : "r");
    }

    plt::figure();
    plt::scatter(entryDates, entryPrices, 50, colors, "o");
    plt::scatter(exitDates, exitPrices, 50, colors, "x");
    plt::title("Trade Markers");
    plt::xlabel("Date");
    plt::ylabel("Price");
    plt::grid(true);
    
    std::string filename = outputFolder_ + getCurrentTimestamp() + "/trade_markers.png";
    plt::save(filename);
    plt::close();
}

void BacktestReporter::generateDrawdownPlot(const std::vector<DailyStats>& stats) {
    std::vector<double> dates;
    std::vector<double> equity;
    std::vector<double> drawdown;
    
    // Extract dates and equity values
    for (const auto& stat : stats) {
        dates.push_back(std::stod(stat.date));
        equity.push_back(stat.equity);
    }
    
    // Calculate drawdown
    drawdown = calculateDrawdown(equity);
    
    // Create figure with two subplots
    plt::figure_size(1200, 800);
    plt::subplot(2, 1, 1);
    
    // Plot equity curve
    plt::plot(dates, equity, "b-");
    plt::title("Equity Curve");
    plt::grid(true);
    
    // Plot drawdown
    plt::subplot(2, 1, 2);
    plt::plot(dates, drawdown, "r-");
    plt::title("Drawdown");
    plt::xlabel("Date");
    plt::ylabel("Drawdown (%)");
    plt::grid(true);
    
    // Save plot
    savePlot("drawdown_analysis.png");
}

void BacktestReporter::generateTradeDistributionPlots(const std::vector<TradeRecord>& trades) {
    // Extract PnL values
    std::vector<double> pnls;
    for (const auto& trade : trades) {
        pnls.push_back(trade.pnl);
    }
    
    // Create figure with multiple subplots
    plt::figure_size(1200, 800);
    
    // PnL Distribution
    plt::subplot(2, 2, 1);
    plt::hist(pnls, 50);
    plt::title("PnL Distribution");
    plt::xlabel("PnL");
    plt::ylabel("Frequency");
    plt::grid(true);
    
    // Cumulative PnL Distribution
    plt::subplot(2, 2, 2);
    std::sort(pnls.begin(), pnls.end());
    std::vector<double> cumsum(pnls.size());
    std::partial_sum(pnls.begin(), pnls.end(), cumsum.begin());
    plt::plot(pnls, cumsum, "b-");
    plt::title("Cumulative PnL Distribution");
    plt::xlabel("PnL");
    plt::ylabel("Cumulative PnL");
    plt::grid(true);
    
    // Trade Duration Distribution
    plt::subplot(2, 2, 3);
    std::vector<double> durations;
    for (const auto& trade : trades) {
        double entry = std::stod(trade.entryTime);
        double exit = std::stod(trade.exitTime);
        durations.push_back(exit - entry);
    }
    plt::hist(durations, 50);
    plt::title("Trade Duration Distribution");
    plt::xlabel("Duration (days)");
    plt::ylabel("Frequency");
    plt::grid(true);
    
    // Save plot
    savePlot("trade_distributions.png");
}

void BacktestReporter::generateMonthlyPerformanceHeatmap(const std::vector<DailyStats>& stats) {
    // Group stats by month
    std::map<std::string, double> monthlyPnL;
    for (const auto& stat : stats) {
        std::string month = stat.date.substr(0, 7); // YYYY-MM
        monthlyPnL[month] += stat.pnl;
    }
    
    // Create heatmap data
    std::vector<std::string> months;
    std::vector<double> pnls;
    for (const auto& [month, pnl] : monthlyPnL) {
        months.push_back(month);
        pnls.push_back(pnl);
    }
    
    // Create heatmap
    plt::figure_size(1200, 400);
    plt::bar(months, pnls);
    plt::title("Monthly Performance");
    plt::xlabel("Month");
    plt::ylabel("PnL");
    plt::grid(true);
    
    // Save plot
    savePlot("monthly_performance.png");
}

void BacktestReporter::generateCorrelationMatrix(const std::vector<TradeRecord>& trades) {
    // Group trades by symbol
    std::map<std::string, std::vector<double>> symbolPnls;
    for (const auto& trade : trades) {
        symbolPnls[trade.symbol].push_back(trade.pnl);
    }
    
    // Create correlation matrix
    std::vector<std::string> symbols;
    std::vector<std::vector<double>> correlations;
    
    for (const auto& [symbol1, pnls1] : symbolPnls) {
        symbols.push_back(symbol1);
        std::vector<double> row;
        
        for (const auto& [symbol2, pnls2] : symbolPnls) {
            // Calculate correlation coefficient
            double corr = 0.0;
            if (pnls1.size() == pnls2.size()) {
                // Simple correlation calculation
                double sum1 = std::accumulate(pnls1.begin(), pnls1.end(), 0.0);
                double sum2 = std::accumulate(pnls2.begin(), pnls2.end(), 0.0);
                double mean1 = sum1 / pnls1.size();
                double mean2 = sum2 / pnls2.size();
                
                double numerator = 0.0;
                double denom1 = 0.0;
                double denom2 = 0.0;
                
                for (size_t i = 0; i < pnls1.size(); ++i) {
                    double diff1 = pnls1[i] - mean1;
                    double diff2 = pnls2[i] - mean2;
                    numerator += diff1 * diff2;
                    denom1 += diff1 * diff1;
                    denom2 += diff2 * diff2;
                }
                
                corr = numerator / std::sqrt(denom1 * denom2);
            }
            row.push_back(corr);
        }
        correlations.push_back(row);
    }
    
    // Create heatmap
    plt::figure_size(800, 800);
    plt::imshow(correlations);
    plt::colorbar();
    plt::title("Symbol Correlation Matrix");
    plt::xticks(range(symbols.size()), symbols);
    plt::yticks(range(symbols.size()), symbols);
    
    // Save plot
    savePlot("correlation_matrix.png");
}

void BacktestReporter::setupPlotStyle() {
    plt::style("seaborn");
    plt::rc("font", "size", 10);
    plt::rc("axes", "titlesize", 12);
    plt::rc("axes", "labelsize", 10);
    plt::rc("xtick", "labelsize", 8);
    plt::rc("ytick", "labelsize", 8);
    plt::rc("legend", "fontsize", 10);
    plt::rc("figure", "titlesize", 14);
}

void BacktestReporter::savePlot(const std::string& filename) {
    std::string fullPath = outputFolder_ + getCurrentTimestamp() + "/" + filename;
    plt::save(fullPath);
    plt::close();
}

std::vector<double> BacktestReporter::calculateDrawdown(const std::vector<double>& equity) {
    std::vector<double> drawdown(equity.size());
    double peak = equity[0];
    
    for (size_t i = 0; i < equity.size(); ++i) {
        peak = std::max(peak, equity[i]);
        drawdown[i] = (peak - equity[i]) / peak * 100.0;
    }
    
    return drawdown;
}

void BacktestReporter::createOutputDirectory() {
    std::string timestamp = getCurrentTimestamp();
    std::string fullPath = outputFolder_ + timestamp;
    
    if (!std::filesystem::exists(fullPath)) {
        std::filesystem::create_directories(fullPath);
    }
}

std::string BacktestReporter::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&time);
    
    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%d_%H-%M-%S");
    return ss.str();
}

std::string BacktestReporter::formatDateTime(const std::string& timestamp) {
    // TODO: Implement date/time formatting
    return timestamp;
}

void BacktestReporter::validateConfiguration() {
    if (outputFolder_.empty()) {
        throw std::runtime_error("Output folder path cannot be empty");
    }
    
    if (outputFolder_.back() != '/') {
        outputFolder_ += '/';
    }
}

} // namespace visualization
} // namespace tradingbot 