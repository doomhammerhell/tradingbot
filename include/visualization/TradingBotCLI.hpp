#pragma once

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <ncurses.h>
#include "../core/TradingEngine.hpp"

namespace tradingbot {
namespace visualization {

class TradingBotCLI {
public:
    TradingBotCLI(std::shared_ptr<core::TradingEngine> engine);
    ~TradingBotCLI();

    // Start the CLI interface
    void start();

    // Stop the CLI interface
    void stop();

private:
    std::shared_ptr<core::TradingEngine> engine_;
    std::thread cliThread_;
    std::atomic<bool> running_;
    std::mutex mutex_;

    // NCurses windows
    WINDOW* mainWin_;
    WINDOW* statusWin_;
    WINDOW* commandWin_;

    // Helper methods
    void initializeNcurses();
    void cleanupNcurses();
    void drawInterface();
    void updateStatus();
    void processInput();
    void handleCommand(const std::string& command);
    void showHelp();
    void showError(const std::string& message);
};

} // namespace visualization
} // namespace tradingbot 