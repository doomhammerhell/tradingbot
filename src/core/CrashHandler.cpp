#include "core/CrashHandler.hpp"
#include <fstream>
#include <filesystem>
#include <signal.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

namespace tradingbot {
namespace core {

std::atomic<bool> CrashHandler::isInitialized_{false};

CrashHandler& CrashHandler::getInstance() {
    static CrashHandler instance;
    return instance;
}

void CrashHandler::initialize(const std::string& stateFilePath) {
    if (isInitialized_) {
        return;
    }
    
    stateFilePath_ = stateFilePath;
    
    // Register signal handlers
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    
    // Check for existing state file
    if (std::filesystem::exists(stateFilePath_)) {
        isRecovering_ = true;
        spdlog::info("Recovering from previous state");
    }
    
    isInitialized_ = true;
}

void CrashHandler::registerCleanup(std::function<void()> cleanup) {
    cleanupFunctions_.push_back(cleanup);
}

void CrashHandler::saveState(const nlohmann::json& state) {
    saveStateToFile(state);
}

nlohmann::json CrashHandler::loadState() const {
    return loadStateFromFile();
}

bool CrashHandler::isRecovering() const {
    return isRecovering_;
}

int CrashHandler::getLastSignal() const {
    return lastSignal_;
}

void CrashHandler::signalHandler(int signal) {
    auto& instance = getInstance();
    instance.lastSignal_ = signal;
    
    spdlog::error("Received signal: {}", signal);
    instance.cleanup();
    
    // Restore default handler and re-raise signal
    signal(signal, SIG_DFL);
    raise(signal);
}

void CrashHandler::saveStateToFile(const nlohmann::json& state) const {
    try {
        std::ofstream file(stateFilePath_);
        file << state.dump(4);
        spdlog::debug("State saved to {}", stateFilePath_);
    } catch (const std::exception& e) {
        spdlog::error("Failed to save state: {}", e.what());
    }
}

nlohmann::json CrashHandler::loadStateFromFile() const {
    try {
        if (!std::filesystem::exists(stateFilePath_)) {
            return nlohmann::json();
        }
        
        std::ifstream file(stateFilePath_);
        nlohmann::json state;
        file >> state;
        
        spdlog::info("State loaded from {}", stateFilePath_);
        return state;
    } catch (const std::exception& e) {
        spdlog::error("Failed to load state: {}", e.what());
        return nlohmann::json();
    }
}

void CrashHandler::cleanup() {
    spdlog::info("Cleaning up resources...");
    
    // Execute all registered cleanup functions
    for (const auto& cleanup : cleanupFunctions_) {
        try {
            cleanup();
        } catch (const std::exception& e) {
            spdlog::error("Cleanup function failed: {}", e.what());
        }
    }
    
    // Flush logs
    Logger::getInstance().flush();
}

CrashHandler::~CrashHandler() {
    if (isInitialized_) {
        cleanup();
    }
}

} // namespace core
} // namespace tradingbot 