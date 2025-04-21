#pragma once

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <nlohmann/json.hpp>

namespace tradingbot {
namespace core {

class Logger {
public:
    static Logger& getInstance();
    
    // Initialize logger with configuration
    void initialize(const std::string& logDir = "logs", 
                   const std::string& logLevel = "info",
                   size_t maxFileSize = 1048576 * 5,  // 5MB
                   size_t maxFiles = 3);
    
    // Log methods with structured data
    void info(const std::string& message, const nlohmann::json& data = {});
    void debug(const std::string& message, const nlohmann::json& data = {});
    void warn(const std::string& message, const nlohmann::json& data = {});
    void error(const std::string& message, const nlohmann::json& data = {});
    
    // Log trade-specific events
    void logTrade(const std::string& symbol, 
                 double price, 
                 double quantity, 
                 const std::string& side,
                 const nlohmann::json& metadata = {});
    
    // Log performance metrics
    void logMetrics(const nlohmann::json& metrics);
    
    // Flush all logs
    void flush();

private:
    Logger() = default;
    ~Logger();
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Convert log level string to spdlog level
    spdlog::level::level_enum getLogLevel(const std::string& level) const;
    
    // Create structured log message
    std::string createStructuredMessage(const std::string& message, 
                                      const nlohmann::json& data) const;
    
    std::shared_ptr<spdlog::logger> logger_;
    std::string logDir_;
};

} // namespace core
} // namespace tradingbot 