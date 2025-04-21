# Enable testing
enable_testing()

# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Technical analysis tests
add_executable(test_technical
    technical/test_ema.cpp
    technical/test_sma.cpp
    technical/test_rsi.cpp
    technical/test_macd.cpp
    technical/test_stochrsi.cpp
)

target_link_libraries(test_technical
    PRIVATE
        tradingbot_technical
        GTest::gtest_main
)

# Core tests
add_executable(test_core
    core/test_strategy.cpp
    core/test_execution.cpp
    core/test_risk.cpp
)

target_link_libraries(test_core
    PRIVATE
        tradingbot_core
        GTest::gtest_main
)

# Add tests to CTest
add_test(NAME test_technical COMMAND test_technical)
add_test(NAME test_core COMMAND test_core)

# Add test executable
add_executable(tradingbot_tests
    strategies/strategy_moving_average_test.cpp
    data/historical_data_feed_test.cpp
)

# Link test executable with GoogleTest and tradingbot library
target_link_libraries(tradingbot_tests
    PRIVATE
    GTest::GTest
    GTest::Main
    tradingbot
)

# Add test
add_test(NAME tradingbot_tests COMMAND tradingbot_tests) 