#include "exchange/BinanceConnector.hpp"
#include <catch2/catch.hpp>
#include <thread>

using namespace tradingbot::exchange;

TEST_CASE("BinanceConnector initialization", "[exchange][binance]") {
    BinanceConnector connector;
    
    SECTION("Initial state") {
        REQUIRE_FALSE(connector.isConnected());
        REQUIRE(connector.getExchangeName() == "Binance");
    }
    
    SECTION("Initialize with credentials") {
        REQUIRE_NOTHROW(connector.initialize("test_key", "test_secret"));
        REQUIRE(connector.isConnected());
    }
    
    SECTION("Initialize with empty credentials") {
        REQUIRE_THROWS_AS(
            connector.initialize("", ""),
            std::runtime_error
        );
    }
}

TEST_CASE("BinanceConnector market data", "[exchange][binance]") {
    BinanceConnector connector;
    connector.initialize("test_key", "test_secret");
    
    SECTION("Get available symbols") {
        auto symbols = connector.getAvailableSymbols();
        REQUIRE_FALSE(symbols.empty());
        REQUIRE(std::find(symbols.begin(), symbols.end(), "BTCUSDT") != symbols.end());
    }
    
    SECTION("Get current price") {
        auto price = connector.getCurrentPrice("BTCUSDT");
        REQUIRE(price > 0.0);
        
        // Test caching
        auto cachedPrice = connector.getCurrentPrice("BTCUSDT");
        REQUIRE(price == cachedPrice);
        
        // Wait for cache to expire
        std::this_thread::sleep_for(std::chrono::seconds(6));
        auto newPrice = connector.getCurrentPrice("BTCUSDT");
        REQUIRE(newPrice != price);
    }
    
    SECTION("Get invalid symbol price") {
        REQUIRE_THROWS_AS(
            connector.getCurrentPrice("INVALID"),
            std::runtime_error
        );
    }
}

TEST_CASE("BinanceConnector order management", "[exchange][binance]") {
    BinanceConnector connector;
    connector.initialize("test_key", "test_secret");
    
    SECTION("Place market order") {
        Order order;
        order.symbol = "BTCUSDT";
        order.side = OrderSide::BUY;
        order.type = OrderType::MARKET;
        order.quantity = 0.001;
        
        auto orderId = connector.placeOrder(order);
        REQUIRE_FALSE(orderId.empty());
        
        auto status = connector.getOrderStatus(orderId);
        REQUIRE(status == OrderStatus::FILLED);
    }
    
    SECTION("Place limit order") {
        Order order;
        order.symbol = "BTCUSDT";
        order.side = OrderSide::SELL;
        order.type = OrderType::LIMIT;
        order.quantity = 0.001;
        order.price = 50000.0;
        
        auto orderId = connector.placeOrder(order);
        REQUIRE_FALSE(orderId.empty());
        
        auto status = connector.getOrderStatus(orderId);
        REQUIRE(status == OrderStatus::NEW);
    }
    
    SECTION("Cancel order") {
        Order order;
        order.symbol = "BTCUSDT";
        order.side = OrderSide::SELL;
        order.type = OrderType::LIMIT;
        order.quantity = 0.001;
        order.price = 50000.0;
        
        auto orderId = connector.placeOrder(order);
        REQUIRE_NOTHROW(connector.cancelOrder(orderId));
        
        auto status = connector.getOrderStatus(orderId);
        REQUIRE(status == OrderStatus::CANCELED);
    }
    
    SECTION("Get open orders") {
        auto orders = connector.getOpenOrders();
        REQUIRE(orders.empty()); // No open orders initially
        
        Order order;
        order.symbol = "BTCUSDT";
        order.side = OrderSide::SELL;
        order.type = OrderType::LIMIT;
        order.quantity = 0.001;
        order.price = 50000.0;
        
        connector.placeOrder(order);
        orders = connector.getOpenOrders();
        REQUIRE_FALSE(orders.empty());
    }
}

TEST_CASE("BinanceConnector balance management", "[exchange][binance]") {
    BinanceConnector connector;
    connector.initialize("test_key", "test_secret");
    
    SECTION("Get all balances") {
        auto balances = connector.getBalances();
        REQUIRE_FALSE(balances.empty());
        
        auto btcBalance = std::find_if(balances.begin(), balances.end(),
            [](const Balance& b) { return b.asset == "BTC"; });
        REQUIRE(btcBalance != balances.end());
    }
    
    SECTION("Get specific balance") {
        auto balance = connector.getBalance("BTC");
        REQUIRE(balance.asset == "BTC");
        REQUIRE(balance.free >= 0.0);
        REQUIRE(balance.locked >= 0.0);
    }
    
    SECTION("Get invalid asset balance") {
        REQUIRE_THROWS_AS(
            connector.getBalance("INVALID"),
            std::runtime_error
        );
    }
}

TEST_CASE("BinanceConnector error handling", "[exchange][binance]") {
    BinanceConnector connector;
    
    SECTION("Operations without initialization") {
        REQUIRE_THROWS_AS(
            connector.getCurrentPrice("BTCUSDT"),
            std::runtime_error
        );
        
        Order order;
        REQUIRE_THROWS_AS(
            connector.placeOrder(order),
            std::runtime_error
        );
        
        REQUIRE_THROWS_AS(
            connector.getBalances(),
            std::runtime_error
        );
    }
    
    SECTION("Invalid order parameters") {
        connector.initialize("test_key", "test_secret");
        
        Order order;
        order.symbol = "INVALID";
        REQUIRE_THROWS_AS(
            connector.placeOrder(order),
            std::runtime_error
        );
        
        order.symbol = "BTCUSDT";
        order.quantity = 0.0;
        REQUIRE_THROWS_AS(
            connector.placeOrder(order),
            std::runtime_error
        );
    }
} 