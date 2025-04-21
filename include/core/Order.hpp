#pragma once

#include <string>
#include <chrono>

namespace tradingbot {
namespace core {

enum class OrderType {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED,
    EXPIRED
};

struct Order {
    std::string id;
    std::string symbol;
    OrderType type;
    OrderSide side;
    double quantity;
    double price;
    double stopPrice;
    double executedQuantity;
    double averagePrice;
    OrderStatus status;
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point updateTime;

    Order()
        : quantity(0.0)
        , price(0.0)
        , stopPrice(0.0)
        , executedQuantity(0.0)
        , averagePrice(0.0)
        , status(OrderStatus::NEW)
    {}
};

} // namespace core
} // namespace tradingbot 