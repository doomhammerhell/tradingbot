#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace tradingbot {
namespace core {

struct MarketData {
    std::string symbol;
    double price;
    double volume;
    std::chrono::system_clock::time_point timestamp;
    double bid;
    double ask;
    double high;
    double low;
    double open;
    double close;
    double vwap; // Volume Weighted Average Price

    MarketData()
        : price(0.0)
        , volume(0.0)
        , bid(0.0)
        , ask(0.0)
        , high(0.0)
        , low(0.0)
        , open(0.0)
        , close(0.0)
        , vwap(0.0)
    {}
};

} // namespace core
} // namespace tradingbot 