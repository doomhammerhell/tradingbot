#include "visualization/TradingBotCLI.hpp"
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

namespace tradingbot {
namespace visualization {

TradingBotCLI::TradingBotCLI(std::shared_ptr<core::TradingEngine> engine)
    : engine_(engine), running_(false), mainWin_(nullptr),
      statusWin_(nullptr), commandWin_(nullptr) {
}

TradingBotCLI::~TradingBotCLI() {
    stop();
}

void TradingBotCLI::start() {
    if (running_) {
        return;
    }

    running_ = true;
    cliThread_ = std::thread([this]() {
        initializeNcurses();
        
        while (running_) {
            drawInterface();
            updateStatus();
            processInput();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        cleanupNcurses();
    });
}

void TradingBotCLI::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (cliThread_.joinable()) {
        cliThread_.join();
    }
}

void TradingBotCLI::initializeNcurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // Create windows
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    
    mainWin_ = newwin(maxY - 2, maxX, 0, 0);
    statusWin_ = newwin(1, maxX, maxY - 2, 0);
    commandWin_ = newwin(1, maxX, maxY - 1, 0);
    
    // Enable scrolling
    scrollok(mainWin_, TRUE);
    scrollok(statusWin_, TRUE);
    scrollok(commandWin_, TRUE);
}

void TradingBotCLI::cleanupNcurses() {
    delwin(mainWin_);
    delwin(statusWin_);
    delwin(commandWin_);
    endwin();
}

void TradingBotCLI::drawInterface() {
    // Clear windows
    werase(mainWin_);
    werase(statusWin_);
    werase(commandWin_);
    
    // Draw borders
    box(mainWin_, 0, 0);
    box(statusWin_, 0, 0);
    box(commandWin_, 0, 0);
    
    // Draw titles
    mvwprintw(mainWin_, 0, 2, " Trading Bot ");
    mvwprintw(statusWin_, 0, 2, " Status ");
    mvwprintw(commandWin_, 0, 2, " Command ");
    
    // Refresh windows
    wrefresh(mainWin_);
    wrefresh(statusWin_);
    wrefresh(commandWin_);
}

void TradingBotCLI::updateStatus() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get engine status
    auto state = engine_->getState();
    auto metrics = engine_->getPerformanceMetrics();
    
    // Format status string
    std::stringstream ss;
    ss << "Mode: " << (state.isLive ? "LIVE" : "SIMULATION") << " | "
       << "Strategy: " << state.currentStrategy << " | "
       << "Trades: " << metrics.totalTrades << " | "
       << "Win Rate: " << std::fixed << std::setprecision(2) 
       << (metrics.winRatio * 100) << "% | "
       << "PnL: " << metrics.totalPnL;
    
    // Update status window
    mvwprintw(statusWin_, 0, 10, ss.str().c_str());
    wrefresh(statusWin_);
}

void TradingBotCLI::processInput() {
    int ch = wgetch(commandWin_);
    
    if (ch == KEY_ENTER || ch == '\n') {
        // Get command from command window
        char buffer[256];
        wgetnstr(commandWin_, buffer, sizeof(buffer) - 1);
        std::string command(buffer);
        
        // Clear command window
        werase(commandWin_);
        box(commandWin_, 0, 0);
        mvwprintw(commandWin_, 0, 2, " Command ");
        wrefresh(commandWin_);
        
        // Handle command
        handleCommand(command);
    }
}

void TradingBotCLI::handleCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (command == "help") {
        showHelp();
    }
    else if (command == "start") {
        engine_->start();
    }
    else if (command == "stop") {
        engine_->stop();
    }
    else if (command == "pause") {
        engine_->pause();
    }
    else if (command == "resume") {
        engine_->resume();
    }
    else if (command == "status") {
        updateStatus();
    }
    else if (command.substr(0, 8) == "strategy") {
        // Change strategy
        std::string strategyName = command.substr(9);
        try {
            engine_->setStrategy(strategyName);
        }
        catch (const std::exception& e) {
            showError(e.what());
        }
    }
    else {
        showError("Unknown command. Type 'help' for available commands.");
    }
}

void TradingBotCLI::showHelp() {
    std::string helpText = R"(
Available commands:
  help      - Show this help message
  start     - Start the trading bot
  stop      - Stop the trading bot
  pause     - Pause the trading bot
  resume    - Resume the trading bot
  status    - Show current status
  strategy <name> - Change to specified strategy
)";
    
    wclear(mainWin_);
    box(mainWin_, 0, 0);
    mvwprintw(mainWin_, 0, 2, " Trading Bot ");
    
    int y = 1;
    std::istringstream iss(helpText);
    std::string line;
    while (std::getline(iss, line)) {
        mvwprintw(mainWin_, y++, 1, line.c_str());
    }
    
    wrefresh(mainWin_);
}

void TradingBotCLI::showError(const std::string& message) {
    wclear(mainWin_);
    box(mainWin_, 0, 0);
    mvwprintw(mainWin_, 0, 2, " Trading Bot ");
    mvwprintw(mainWin_, 1, 1, "Error: %s", message.c_str());
    wrefresh(mainWin_);
}

} // namespace visualization
} // namespace tradingbot 