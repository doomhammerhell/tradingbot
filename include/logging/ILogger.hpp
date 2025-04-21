#pragma once

#include <string>
#include <memory>
#include <map>
#include <chrono>
#include <vector>
#include <functional>

namespace tradingbot {
namespace logging {

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

struct LogContext {
    std::string component;
    std::string function;
    int line;
    std::string file;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;
};

struct Metric {
    std::string name;
    double value;
    std::map<std::string, std::string> labels;
    std::chrono::system_clock::time_point timestamp;
};

struct Alert {
    std::string id;
    std::string message;
    LogLevel level;
    std::map<std::string, std::string> context;
    std::chrono::system_clock::time_point timestamp;
};

class ILogger {
public:
    virtual ~ILogger() = default;

    // Logging methods
    virtual void log(LogLevel level, const std::string& message, const LogContext& context) = 0;
    virtual void trace(const std::string& message, const LogContext& context) = 0;
    virtual void debug(const std::string& message, const LogContext& context) = 0;
    virtual void info(const std::string& message, const LogContext& context) = 0;
    virtual void warning(const std::string& message, const LogContext& context) = 0;
    virtual void error(const std::string& message, const LogContext& context) = 0;
    virtual void critical(const std::string& message, const LogContext& context) = 0;

    // Metrics methods
    virtual void recordMetric(const Metric& metric) = 0;
    virtual void incrementCounter(const std::string& name, const std::map<std::string, std::string>& labels = {}) = 0;
    virtual void recordGauge(const std::string& name, double value, const std::map<std::string, std::string>& labels = {}) = 0;
    virtual void recordHistogram(const std::string& name, double value, const std::map<std::string, std::string>& labels = {}) = 0;

    // Alert methods
    virtual void raiseAlert(const Alert& alert) = 0;
    virtual void subscribeToAlerts(LogLevel level, std::function<void(const Alert&)> callback) = 0;

    // Query methods
    virtual std::vector<LogContext> queryLogs(
        LogLevel minLevel,
        const std::chrono::system_clock::time_point& startTime,
        const std::chrono::system_clock::time_point& endTime,
        const std::map<std::string, std::string>& filters = {}) = 0;

    virtual std::vector<Metric> queryMetrics(
        const std::string& name,
        const std::chrono::system_clock::time_point& startTime,
        const std::chrono::system_clock::time_point& endTime,
        const std::map<std::string, std::string>& labels = {}) = 0;

    // Configuration methods
    virtual void setLogLevel(LogLevel level) = 0;
    virtual void setOutputFile(const std::string& path) = 0;
    virtual void setMaxFileSize(size_t size) = 0;
    virtual void setMaxFiles(size_t count) = 0;
    virtual void setRetentionPeriod(const std::chrono::hours& hours) = 0;
};

} // namespace logging
} // namespace tradingbot 