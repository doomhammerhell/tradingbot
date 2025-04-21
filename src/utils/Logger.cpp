#include "../../include/utils/Logger.hpp"
#include <filesystem>

namespace tradingbot {
namespace utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Create logs directory if it doesn't exist
    std::filesystem::create_directories("logs");
    
    // Create console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    
    // Create file sink
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/bot.log", 1024 * 1024 * 5, 3);
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    
    // Create logger with both sinks
    logger_ = std::make_shared<spdlog::logger>("tradingbot", 
        spdlog::sinks_init_list{console_sink, file_sink});
    
    // Set default log level
    logger_->set_level(spdlog::level::info);
    
    // Set flush level
    logger_->flush_on(spdlog::level::warn);
}

} // namespace utils
} // namespace tradingbot 