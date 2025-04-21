# Core library
add_library(tradingbot_core
    core/IStrategy.cpp
    core/IExecution.cpp
    core/IRiskManager.cpp
    core/IDataFeed.cpp
)

target_include_directories(tradingbot_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Technical analysis library
add_library(tradingbot_technical
    technical/TechnicalAnalysis.cpp
    technical/indicators/EMA.cpp
    technical/indicators/SMA.cpp
    technical/indicators/RSI.cpp
    technical/indicators/MACD.cpp
    technical/indicators/StochRSI.cpp
)

target_include_directories(tradingbot_technical
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Data library
add_library(tradingbot_data
    data/IEX.cpp
    data/JSONdata.cpp
)

target_include_directories(tradingbot_data
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Execution library
add_library(tradingbot_execution
    execution/MockExecution.cpp
)

target_include_directories(tradingbot_execution
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Risk management library
add_library(tradingbot_risk
    risk/RiskManager.cpp
)

target_include_directories(tradingbot_risk
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Strategies library
add_library(tradingbot_strategies
    strategies/BaseStrategy.cpp
    strategies/TechnicalStrategy.cpp
)

target_include_directories(tradingbot_strategies
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(tradingbot_technical
    PRIVATE
        tradingbot_core
        tradingbot_data
)

target_link_libraries(tradingbot_execution
    PRIVATE
        tradingbot_core
)

target_link_libraries(tradingbot_risk
    PRIVATE
        tradingbot_core
        tradingbot_execution
)

target_link_libraries(tradingbot_strategies
    PRIVATE
        tradingbot_core
        tradingbot_technical
        tradingbot_risk
)

# Add tradingbot library
add_library(tradingbot
    core/TradingBot.cpp
    data/BinanceDataProvider.cpp
    execution/BinanceOrderExecutor.cpp
    risk/BasicRiskManager.cpp
    strategies/MACDStrategy.cpp
    strategies/MovingAverageStrategy.cpp
    backtesting/AdvancedBacktester.cpp
    logging/AdvancedLogger.cpp
    utils/Logger.cpp
)

# Link libraries
target_link_libraries(tradingbot
    PRIVATE
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${GSL_LIBRARIES}
    spdlog::spdlog
)

# Add main executable
add_executable(tradingbot_main main.cpp)
target_link_libraries(tradingbot_main PRIVATE tradingbot) 