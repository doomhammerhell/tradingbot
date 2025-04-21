#pragma once

#include "IExchangeConnector.hpp"
#include <curl/curl.h>
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

namespace tradingbot {
namespace exchange {

class BinanceConnector : public IExchangeConnector {
public:
    BinanceConnector();
    ~BinanceConnector();

    // IExchangeConnector implementation
    void initialize(const std::string& apiKey, const std::string& apiSecret) override;
    Order placeOrder(const Order& order) override;
    bool cancelOrder(const std::string& orderId) override;
    std::vector<Order> getOpenOrders() override;
    Order getOrderStatus(const std::string& orderId) override;
    std::vector<Balance> getBalances() override;
    Balance getBalance(const std::string& asset) override;
    double getCurrentPrice(const std::string& symbol) override;
    std::vector<std::string> getAvailableSymbols() override;
    std::string getExchangeName() const override;
    bool isConnected() const override;

private:
    CURL* curl_;
    std::string apiKey_;
    std::string apiSecret_;
    bool connected_;
    std::mutex mutex_;
    std::map<std::string, double> priceCache_;
    std::chrono::system_clock::time_point lastPriceUpdate_;

    // HTTP request helpers
    std::string makeRequest(const std::string& endpoint, bool useAuth = false);
    std::string signRequest(const std::string& query);
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

    // API endpoints
    static constexpr const char* BASE_URL = "https://api.binance.com";
    static constexpr const char* TICKER_PRICE = "/api/v3/ticker/price";
    static constexpr const char* EXCHANGE_INFO = "/api/v3/exchangeInfo";
    static constexpr const char* ACCOUNT_INFO = "/api/v3/account";
    static constexpr const char* ORDER = "/api/v3/order";
    static constexpr const char* OPEN_ORDERS = "/api/v3/openOrders";

    // Cache management
    void updatePriceCache();
    bool isPriceCacheValid() const;
};

} // namespace exchange
} // namespace tradingbot 