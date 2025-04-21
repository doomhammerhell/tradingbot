#include <iostream>
#include "utils/Logger.hpp"

int main(int argc, char* argv[]) {
    // Initialize logger
    auto& logger = tradingbot::utils::Logger::getInstance();
    logger.info("Trading bot started");

    // TODO: Initialize trading bot components
    // TODO: Start main loop

    logger.info("Trading bot stopped");
    return 0;
} 