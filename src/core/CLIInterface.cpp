#include "core/CLIInterface.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <chrono>
#include <thread>

namespace tradingbot {
namespace core {

CLIInterface::CLIInterface() : app_(std::make_unique<CLI::App>("Trading Bot")) {
    setupOptions();
    setupCommands();
}

void CLIInterface::setupOptions() {
    // Basic options
    app_->add_option("-c,--config", options_.configFile, "Configuration file path")
        ->check(CLI::ExistingFile);
    
    app_->add_option("-l,--log-level", options_.logLevel, "Log level (trace, debug, info, warn, error)")
        ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error"}));
    
    app_->add_option("-s,--strategy", options_.strategy, "Trading strategy to use")
        ->required();
    
    // Flags
    app_->add_flag("--simulation", options_.simulationMode, "Run in simulation mode");
    app_->add_flag("--reset", options_.resetState, "Reset saved state");
    app_->add_flag("--version", options_.version, "Show version information");
    
    // Set version
    app_->set_version_flag("--version", "1.0.0");
}

void CLIInterface::setupCommands() {
    // Status commands
    auto status = app_->add_subcommand("status", "Show bot status");
    status->add_flag("--show", options_.showStatus, "Show detailed status");
    
    // Metrics commands
    auto metrics = app_->add_subcommand("metrics", "Show bot metrics");
    metrics->add_flag("--show", options_.showMetrics, "Show detailed metrics");
    
    // Security commands
    auto security = app_->add_subcommand("security", "Security management");
    security->add_flag("--show", options_.showSecurity, "Show security status");
    security->add_flag("--audit-logs", options_.showAuditLogs, "Show audit logs");
    security->add_option("--limit", options_.auditLogLimit, "Limit number of audit log entries")
        ->default_val(100);
    security->add_flag("--clear", options_.clearAuditLogs, "Clear audit logs");
    
    // Configuration commands
    auto config = app_->add_subcommand("config", "Configuration management");
    config->add_flag("--reload", options_.reloadConfig, "Enable configuration hot-reloading");
    config->add_option("--interval", options_.reloadInterval, "Configuration reload interval (seconds)")
        ->default_val(60);
}

int CLIInterface::parse(int argc, char** argv) {
    try {
        app_->parse(argc, argv);
        
        // Check for help request
        helpRequested_ = app_->get_help_ptr()->as<bool>();
        if (helpRequested_) {
            return 0;
        }
        
        // Check for version request
        versionRequested_ = options_.version;
        if (versionRequested_) {
            return 0;
        }
        
        // Validate configuration
        validateConfig();
        
        // Setup logging
        setupLogging();
        
        // Handle security commands
        if (options_.showSecurity) {
            auto status = SecurityManager::getInstance().getSecurityStatus();
            spdlog::info("Security Status:\n{}", status.dump(2));
        }
        
        if (options_.showAuditLogs) {
            auto logs = SecurityManager::getInstance().getAuditLogs(options_.auditLogLimit);
            spdlog::info("Audit Logs (last {} entries):", options_.auditLogLimit);
            for (const auto& log : logs) {
                spdlog::info("{}", log.dump(2));
            }
        }
        
        if (options_.clearAuditLogs) {
            SecurityManager::getInstance().clearAuditLogs();
        }
        
        // Start configuration hot-reloading if requested
        if (options_.reloadConfig) {
            startConfigReloading();
        }
        
        return 0;
    } catch (const CLI::ParseError& e) {
        return app_->exit(e);
    }
}

void CLIInterface::validateConfig() {
    // Load configuration from file if specified
    if (!options_.configFile.empty()) {
        SecureConfigLoader::getInstance().loadConfig(options_.configFile);
    }
    
    // Update configuration with command line options
    config_["strategy"] = options_.strategy;
    config_["simulation_mode"] = options_.simulationMode;
    config_["log_level"] = options_.logLevel;
    
    // Validate required fields
    if (options_.strategy.empty()) {
        throw CLI::ValidationError("Strategy must be specified");
    }
    
    // Check if state file exists and handle reset
    if (options_.resetState) {
        std::filesystem::remove("state.json");
        spdlog::info("State file reset");
    }
}

void CLIInterface::setupLogging() {
    // Initialize logger with command line options
    Logger::getInstance().initialize(
        "logs",
        options_.logLevel.empty() ? "info" : options_.logLevel
    );
    
    spdlog::info("CLI interface initialized", {
        {"config_file", options_.configFile},
        {"log_level", options_.logLevel},
        {"strategy", options_.strategy},
        {"simulation_mode", options_.simulationMode}
    });
}

void CLIInterface::startConfigReloading() {
    if (configReloading_) {
        return;
    }
    
    configReloading_ = true;
    configReloadThread_ = std::thread([this]() {
        while (configReloading_) {
            std::this_thread::sleep_for(std::chrono::seconds(options_.reloadInterval));
            reloadConfig();
        }
    });
    
    spdlog::info("Configuration hot-reloading started with interval: {} seconds", 
                 options_.reloadInterval);
}

void CLIInterface::stopConfigReloading() {
    if (!configReloading_) {
        return;
    }
    
    configReloading_ = false;
    if (configReloadThread_.joinable()) {
        configReloadThread_.join();
    }
    
    spdlog::info("Configuration hot-reloading stopped");
}

void CLIInterface::reloadConfig() {
    try {
        if (!options_.configFile.empty()) {
            SecureConfigLoader::getInstance().loadConfig(options_.configFile);
            spdlog::info("Configuration reloaded successfully");
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to reload configuration: {}", e.what());
    }
}

nlohmann::json CLIInterface::getConfig() const {
    return config_;
}

bool CLIInterface::helpRequested() const {
    return helpRequested_;
}

bool CLIInterface::versionRequested() const {
    return versionRequested_;
}

} // namespace core
} // namespace tradingbot 