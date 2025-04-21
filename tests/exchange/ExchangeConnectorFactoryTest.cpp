#include "exchange/ExchangeConnectorFactory.hpp"
#include "exchange/MockBinanceConnector.hpp"
#include <catch2/catch.hpp>

using namespace tradingbot::exchange;

TEST_CASE("ExchangeConnectorFactory singleton", "[exchange][factory]") {
    SECTION("Get instance") {
        auto& factory1 = ExchangeConnectorFactory::getInstance();
        auto& factory2 = ExchangeConnectorFactory::getInstance();
        REQUIRE(&factory1 == &factory2);
    }
}

TEST_CASE("ExchangeConnectorFactory registration", "[exchange][factory]") {
    auto& factory = ExchangeConnectorFactory::getInstance();
    
    SECTION("Get available types") {
        auto types = factory.getAvailableTypes();
        REQUIRE(types.size() >= 2); // mock_binance and binance
        REQUIRE(std::find(types.begin(), types.end(), "mock_binance") != types.end());
        REQUIRE(std::find(types.begin(), types.end(), "binance") != types.end());
    }
}

TEST_CASE("ExchangeConnectorFactory creation", "[exchange][factory]") {
    auto& factory = ExchangeConnectorFactory::getInstance();
    
    SECTION("Create mock connector") {
        std::string config = R"({
            "apiKey": "test_key",
            "apiSecret": "test_secret"
        })";
        
        auto connector = factory.createConnector("mock_binance", config);
        REQUIRE(connector != nullptr);
        REQUIRE(connector->getExchangeName() == "MockBinance");
        REQUIRE(connector->isConnected());
    }
    
    SECTION("Create from config JSON") {
        std::string configJson = R"({
            "type": "mock_binance",
            "config": {
                "apiKey": "test_key",
                "apiSecret": "test_secret"
            }
        })";
        
        auto connector = factory.createFromConfig(configJson);
        REQUIRE(connector != nullptr);
        REQUIRE(connector->getExchangeName() == "MockBinance");
        REQUIRE(connector->isConnected());
    }
    
    SECTION("Invalid connector type") {
        REQUIRE_THROWS_AS(
            factory.createConnector("invalid_type", "{}"),
            std::runtime_error
        );
    }
    
    SECTION("Invalid config JSON") {
        REQUIRE_THROWS_AS(
            factory.createFromConfig("invalid json"),
            std::runtime_error
        );
    }
    
    SECTION("Missing config fields") {
        std::string configJson = R"({
            "type": "mock_binance",
            "config": {
                "apiKey": "test_key"
            }
        })";
        
        REQUIRE_THROWS_AS(
            factory.createFromConfig(configJson),
            std::runtime_error
        );
    }
} 