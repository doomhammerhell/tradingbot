#include "exchange/MockBinanceConnector.hpp"
#include <catch2/catch.hpp>
#include <chrono>

using namespace tradingbot::exchange;

TEST_CASE("MockBinanceConnector initialization", "[exchange][mock]") {
    MockBinanceConnector connector;
    
    SECTION("Initial state") {
        REQUIRE_FALSE(connector.isConnected());
        REQUIRE(connector.getExchangeName() == "MockBinance");
    }
    
    SECTION("Initialize with credentials") {
        connector.initialize("test_key", "test_secret");
        REQUIRE(connector.isConnected());
    }
}

TEST_CASE("MockBinanceConnector market data", "[exchange][mock]") {
    MockBinanceConnector connector;
    connector.initialize("test_key", "test_secret");
    
    SECTION("Get available symbols") {
        auto symbols = connector.getAvailableSymbols();
        REQUIRE(symbols.size() > 0);
        REQUIRE(std::find(symbols.begin(), symbols.end(), "BTCUSDT") != symbols.end());
        REQUIRE(std::find(symbols.begin(), symbols.end(), "ETHUSDT") != symbols.end());
    }
    
    SECTION("Get current price") {
        double btcPrice = connector.getCurrentPrice("BTCUSDT");
        double ethPrice = connector.getCurrentPrice("ETHUSDT");
        
        REQUIRE(btcPrice > 0);
        REQUIRE(ethPrice > 0);
        REQUIRE(btcPrice != ethPrice);
    }
}

TEST_CASE("MockBinanceConnector order management", "[exchange][mock]") {
    MockBinanceConnector connector;
    connector.initialize("test_key", "test_secret");
    
    SECTION("Place market order") {
        Order order;
        order.symbol = "BTCUSDT";
        order.type = Order::Type::MARKET;
        order.side = Order::Side::BUY;
        order.quantity = 0.1;
        
        auto result = connector.placeOrder(order);
        REQUIRE(result.orderId.length() > 0);
        REQUIRE(result.symbol == order.symbol);
        REQUIRE(result.type == order.type);
        REQUIRE(result.side == order.side);
        REQUIRE(result.quantity == order.quantity);
    }
    
    SECTION("Place limit order") {
        Order order;
        order.symbol = "BTCUSDT";
        order.type = Order::Type::LIMIT;
        order.side = Order::Side::SELL;
        order.quantity = 0.1;
        order.price = 50000.0;
        
        auto result = connector.placeOrder(order);
        REQUIRE(result.orderId.length() > 0);
        REQUIRE(result.price == order.price);
    }
    
    SECTION("Get open orders") {
        Order order;
        order.symbol = "BTCUSDT";
        order.type = Order::Type::LIMIT;
        order.side = Order::Side::BUY;
        order.quantity = 0.1;
        order.price = 50000.0;
        
        connector.placeOrder(order);
        auto openOrders = connector.getOpenOrders();
        REQUIRE(openOrders.size() == 1);
        REQUIRE(openOrders[0].orderId.length() > 0);
    }
    
    SECTION("Cancel order") {
        Order order;
        order.symbol = "BTCUSDT";
        order.type = Order::Type::LIMIT;
        order.side = Order::Side::BUY;
        order.quantity = 0.1;
        order.price = 50000.0;
        
        auto result = connector.placeOrder(order);
        REQUIRE(connector.cancelOrder(result.orderId));
        auto openOrders = connector.getOpenOrders();
        REQUIRE(openOrders.empty());
    }
}

TEST_CASE("MockBinanceConnector balance management", "[exchange][mock]") {
    MockBinanceConnector connector;
    connector.initialize("test_key", "test_secret");
    
    SECTION("Get all balances") {
        auto balances = connector.getBalances();
        REQUIRE(balances.size() >= 3); // BTC, ETH, USDT
        REQUIRE(std::any_of(balances.begin(), balances.end(),
            [](const Balance& b) { return b.asset == "BTC"; }));
    }
    
    SECTION("Get specific balance") {
        auto btcBalance = connector.getBalance("BTC");
        REQUIRE(btcBalance.asset == "BTC");
        REQUIRE(btcBalance.free > 0);
        REQUIRE(btcBalance.total == btcBalance.free + btcBalance.locked);
    }
    
    SECTION("Balance updates after order") {
        auto initialBalance = connector.getBalance("USDT");
        
        Order order;
        order.symbol = "BTCUSDT";
        order.type = Order::Type::MARKET;
        order.side = Order::Side::BUY;
        order.quantity = 0.1;
        
        connector.placeOrder(order);
        auto newBalance = connector.getBalance("USDT");
        REQUIRE(newBalance.free < initialBalance.free);
    }
}

TEST_CASE("MockBinanceConnector error handling", "[exchange][mock]") {
    MockBinanceConnector connector;
    
    SECTION("Operations without initialization") {
        REQUIRE_THROWS_AS(connector.placeOrder(Order{}), std::runtime_error);
        REQUIRE_THROWS_AS(connector.getBalances(), std::runtime_error);
    }
    
    SECTION("Invalid symbol") {
        connector.initialize("test_key", "test_secret");
        REQUIRE_THROWS_AS(connector.getCurrentPrice("INVALID"), std::runtime_error);
    }
    
    SECTION("Invalid asset") {
        connector.initialize("test_key", "test_secret");
        REQUIRE_THROWS_AS(connector.getBalance("INVALID"), std::runtime_error);
    }
} 