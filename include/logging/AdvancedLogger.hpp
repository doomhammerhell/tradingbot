#pragma once

#include "ILogger.hpp"
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>
#include <elasticsearch/elasticsearch.hpp>
#include <nlohmann/json.hpp>

namespace tradingbot {
namespace logging {

class AdvancedLogger : public ILogger {
public:
    AdvancedLogger(
        const std::string& elasticsearchUrl,
        const std::string& prometheusEndpoint,
        const std::string& grafanaUrl);
    ~AdvancedLogger() override;

    // Logging methods
    void log(LogLevel level, const std::string& message, const LogContext& context) override;
    void trace(const std::string& message, const LogContext& context) override;
    void debug(const std::string& message, const LogContext& context) override;
    void info(const std::string& message, const LogContext& context) override;
    void warning(const std::string& message, const LogContext& context) override;
    void error(const std::string& message, const LogContext& context) override;
    void critical(const std::string& message, const LogContext& context) override;

    // Metrics methods
    void recordMetric(const Metric& metric) override;
    void incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels = {}) override;
    void recordGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels = {}) override;
    void recordHistogram(const std::string& name, double value, const std::map<std::string, std::string>& labels = {}) override;

    // Alert methods
    void raiseAlert(const Alert& alert) override;
    void subscribeToAlerts(LogLevel level, std::function<void(const Alert&)> callback) override;

    // Query methods
    std::vector<LogContext> queryLogs(
        LogLevel minLevel,
        const std::chrono::system_clock::time_point& startTime,
        const std::chrono::system_clock::time_point& endTime,
        const std::map<std::string, std::string>& filters = {}) override;

    std::vector<Metric> queryMetrics(
        const std::string& name,
        const std::chrono::system_clock::time_point& startTime,
        const std::chrono::system_clock::time_point& endTime,
        const std::map<std::string, std::string>& labels = {}) override;

    // Configuration methods
    void setLogLevel(LogLevel level) override;
    void setOutputFile(const std::string& path) override;
    void setMaxFileSize(size_t size) override;
    void setMaxFiles(size_t count) override;
    void setRetentionPeriod(const std::chrono::hours& hours) override;

private:
    struct LogEntry {
        LogLevel level;
        std::string message;
        LogContext context;
        std::chrono::system_clock::time_point timestamp;
    };

    struct MetricEntry {
        Metric metric;
        std::chrono::system_clock::time_point timestamp;
    };

    struct AlertEntry {
        Alert alert;
        std::chrono::system_clock::time_point timestamp;
    };

    // Elasticsearch client
    std::unique_ptr<elasticsearch::Client> elasticsearch_;
    std::string elasticsearchIndex_;

    // Prometheus metrics
    std::shared_ptr<prometheus::Registry> registry_;
    std::map<std::string, prometheus::Counter*> counters_;
    std::map<std::string, prometheus::Gauge*> gauges_;
    std::map<std::string, prometheus::Histogram*> histograms_;

    // Alert subscriptions
    std::map<LogLevel, std::vector<std::function<void(const Alert&)>>> alertSubscriptions_;

    // Configuration
    LogLevel currentLogLevel_;
    std::string outputFile_;
    size_t maxFileSize_;
    size_t maxFiles_;
    std::chrono::hours retentionPeriod_;

    // Queues and processing
    std::queue<LogEntry> logQueue_;
    std::queue<MetricEntry> metricQueue_;
    std::queue<AlertEntry> alertQueue_;
    std::mutex logMutex_;
    std::mutex metricMutex_;
    std::mutex alertMutex_;
    std::condition_variable logCondition_;
    std::condition_variable metricCondition_;
    std::condition_variable alertCondition_;
    std::atomic<bool> running_;
    std::thread logProcessor_;
    std::thread metricProcessor_;
    std::thread alertProcessor_;

    // Processing methods
    void processLogs();
    void processMetrics();
    void processAlerts();
    void indexLog(const LogEntry& entry);
    void indexMetric(const MetricEntry& entry);
    void indexAlert(const AlertEntry& entry);
    void createElasticsearchIndex();
    void setupPrometheusMetrics();
    void setupGrafanaDashboard();
    nlohmann::json createLogDocument(const LogEntry& entry);
    nlohmann::json createMetricDocument(const MetricEntry& entry);
    nlohmann::json createAlertDocument(const AlertEntry& entry);
};

} // namespace logging
} // namespace tradingbot 