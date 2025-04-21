#pragma once

#include <string>
#include <memory>
#include <CLI/CLI.hpp>
#include "core/Logger.hpp"
#include "core/SecureConfigLoader.hpp"
#include "core/CrashHandler.hpp"
#include "core/SecurityManager.hpp"

namespace tradingbot {
namespace core {

class CLIInterface {
public:
    CLIInterface();
    ~CLIInterface() = default;
    
    // Parse command line arguments
    int parse(int argc, char** argv);
    
    // Get the parsed configuration
    nlohmann::json getConfig() const;
    
    // Check if help was requested
    bool helpRequested() const;
    
    // Check if version was requested
    bool versionRequested() const;
    
    // Start configuration hot-reloading
    void startConfigReloading();
    
    // Stop configuration hot-reloading
    void stopConfigReloading();

private:
    void setupOptions();
    void validateConfig();
    void setupLogging();
    void setupCommands();
    void reloadConfig();
    
    std::unique_ptr<CLI::App> app_;
    nlohmann::json config_;
    bool helpRequested_{false};
    bool versionRequested_{false};
    std::atomic<bool> configReloading_{false};
    std::thread configReloadThread_;
    
    // Command line options
    struct {
        std::string configFile;
        std::string logLevel;
        std::string strategy;
        bool simulationMode;
        bool resetState;
        bool version;
        bool showStatus;
        bool showMetrics;
        bool showSecurity;
        bool showAuditLogs;
        int auditLogLimit;
        bool clearAuditLogs;
        bool reloadConfig;
        int reloadInterval;
    } options_;
};

} // namespace core
} // namespace tradingbot 