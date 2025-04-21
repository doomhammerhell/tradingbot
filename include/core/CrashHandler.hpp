#pragma once

#include <string>
#include <functional>
#include <csignal>
#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>
#include "core/Logger.hpp"

namespace tradingbot {
namespace core {

class CrashHandler {
public:
    static CrashHandler& getInstance();

    // Initialize crash handler with state file path
    void initialize(const std::string& stateFilePath = "state.json");
    
    // Register cleanup function
    void registerCleanup(std::function<void()> cleanup);
    
    // Save current state
    void saveState(const nlohmann::json& state);
    
    // Load last saved state
    nlohmann::json loadState() const;
    
    // Check if we're recovering from a crash
    bool isRecovering() const;
    
    // Get the signal that caused the crash
    int getLastSignal() const;

private:
    CrashHandler() = default;
    ~CrashHandler();
    
    // Prevent copying
    CrashHandler(const CrashHandler&) = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;
    
    // Signal handler
    static void signalHandler(int signal);
    
    // Save state to file
    void saveStateToFile(const nlohmann::json& state) const;
    
    // Load state from file
    nlohmann::json loadStateFromFile() const;
    
    // Clean up resources
    void cleanup();

    std::string stateFilePath_;
    std::atomic<bool> isRecovering_{false};
    std::atomic<int> lastSignal_{0};
    std::vector<std::function<void()>> cleanupFunctions_;
    static std::atomic<bool> isInitialized_;
};

} // namespace core
} // namespace tradingbot 