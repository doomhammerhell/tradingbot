#include "core/Logger.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace tradingbot {
namespace core {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& logDir, 
                       const std::string& logLevel,
                       size_t maxFileSize,
                       size_t maxFiles) {
    logDir_ = logDir;
    
    // Create log directory if it doesn't exist
    std::filesystem::create_directories(logDir_);
    
    // Create rotating file sink
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logDir_ + "/tradingbot.log", maxFileSize, maxFiles);
    
    // Create console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    
    // Create logger with both sinks
    logger_ = std::make_shared<spdlog::logger>("tradingbot", 
        spdlog::sinks_init_list{file_sink, console_sink});
    
    // Set log level
    logger_->set_level(getLogLevel(logLevel));
    
    // Set pattern
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    
    // Flush on every log
    logger_->flush_on(spdlog::level::err);
    
    info("Logger initialized", {
        {"log_dir", logDir_},
        {"log_level", logLevel},
        {"max_file_size", maxFileSize},
        {"max_files", maxFiles}
    });
}

void Logger::info(const std::string& message, const nlohmann::json& data) {
    logger_->info(createStructuredMessage(message, data));
}

void Logger::debug(const std::string& message, const nlohmann::json& data) {
    logger_->debug(createStructuredMessage(message, data));
}

void Logger::warn(const std::string& message, const nlohmann::json& data) {
    logger_->warn(createStructuredMessage(message, data));
}

void Logger::error(const std::string& message, const nlohmann::json& data) {
    logger_->error(createStructuredMessage(message, data));
}

void Logger::logTrade(const std::string& symbol, 
                     double price, 
                     double quantity, 
                     const std::string& side,
                     const nlohmann::json& metadata) {
    nlohmann::json tradeData = {
        {"symbol", symbol},
        {"price", price},
        {"quantity", quantity},
        {"side", side},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
    };
    
    if (!metadata.empty()) {
        tradeData["metadata"] = metadata;
    }
    
    info("Trade executed", tradeData);
}

void Logger::logMetrics(const nlohmann::json& metrics) {
    info("Performance metrics", metrics);
}

void Logger::flush() {
    logger_->flush();
}

Logger::~Logger() {
    if (logger_) {
        flush();
    }
}

spdlog::level::level_enum Logger::getLogLevel(const std::string& level) const {
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "info") return spdlog::level::info;
    if (level == "warn") return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    return spdlog::level::info; // Default level
}

std::string Logger::createStructuredMessage(const std::string& message, 
                                          const nlohmann::json& data) const {
    nlohmann::json logEntry = {
        {"message", message},
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
    };
    
    if (!data.empty()) {
        logEntry["data"] = data;
    }
    
    return logEntry.dump();
}

} // namespace core
} // namespace tradingbot 