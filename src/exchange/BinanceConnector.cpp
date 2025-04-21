#include "exchange/BinanceConnector.hpp"
#include "utils/Logger.hpp"
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace tradingbot {
namespace exchange {

BinanceConnector::BinanceConnector()
    : curl_(nullptr)
    , connected_(false) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

BinanceConnector::~BinanceConnector() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
    curl_global_cleanup();
}

void BinanceConnector::initialize(const std::string& apiKey, const std::string& apiSecret) {
    std::lock_guard<std::mutex> lock(mutex_);
    apiKey_ = apiKey;
    apiSecret_ = apiSecret;
    connected_ = true;
    LOG_INFO("BinanceConnector initialized with API key: " + apiKey);
}

Order BinanceConnector::placeOrder(const Order& order) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_) {
        throw std::runtime_error("Not connected to exchange");
    }

    std::stringstream query;
    query << "symbol=" << order.symbol
          << "&side=" << (order.side == Order::Side::BUY ? "BUY" : "SELL")
          << "&type=" << (order.type == Order::Type::MARKET ? "MARKET" : "LIMIT")
          << "&quantity=" << std::fixed << std::setprecision(8) << order.quantity;

    if (order.type == Order::Type::LIMIT) {
        query << "&price=" << std::fixed << std::setprecision(8) << order.price
              << "&timeInForce=GTC";
    }

    std::string response = makeRequest(ORDER + std::string("?") + query.str(), true);
    auto json = nlohmann::json::parse(response);

    Order result = order;
    result.orderId = json["orderId"].get<std::string>();
    result.timestamp = std::chrono::system_clock::now();
    return result;
}

bool BinanceConnector::cancelOrder(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string query = "orderId=" + orderId;
    try {
        makeRequest(ORDER + std::string("?") + query, true);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to cancel order: " + std::string(e.what()));
        return false;
    }
}

std::vector<Order> BinanceConnector::getOpenOrders() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string response = makeRequest(OPEN_ORDERS, true);
    auto json = nlohmann::json::parse(response);

    std::vector<Order> orders;
    for (const auto& orderJson : json) {
        Order order;
        order.symbol = orderJson["symbol"].get<std::string>();
        order.type = orderJson["type"].get<std::string>() == "MARKET" ? 
            Order::Type::MARKET : Order::Type::LIMIT;
        order.side = orderJson["side"].get<std::string>() == "BUY" ? 
            Order::Side::BUY : Order::Side::SELL;
        order.quantity = std::stod(orderJson["origQty"].get<std::string>());
        order.price = std::stod(orderJson["price"].get<std::string>());
        order.orderId = orderJson["orderId"].get<std::string>();
        order.timestamp = std::chrono::system_clock::from_time_t(
            orderJson["time"].get<long>() / 1000);
        orders.push_back(order);
    }
    return orders;
}

Order BinanceConnector::getOrderStatus(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string query = "orderId=" + orderId;
    std::string response = makeRequest(ORDER + std::string("?") + query, true);
    auto json = nlohmann::json::parse(response);

    Order order;
    order.symbol = json["symbol"].get<std::string>();
    order.type = json["type"].get<std::string>() == "MARKET" ? 
        Order::Type::MARKET : Order::Type::LIMIT;
    order.side = json["side"].get<std::string>() == "BUY" ? 
        Order::Side::BUY : Order::Side::SELL;
    order.quantity = std::stod(json["origQty"].get<std::string>());
    order.price = std::stod(json["price"].get<std::string>());
    order.orderId = json["orderId"].get<std::string>();
    order.timestamp = std::chrono::system_clock::from_time_t(
        json["time"].get<long>() / 1000);
    return order;
}

std::vector<Balance> BinanceConnector::getBalances() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string response = makeRequest(ACCOUNT_INFO, true);
    auto json = nlohmann::json::parse(response);

    std::vector<Balance> balances;
    for (const auto& balanceJson : json["balances"]) {
        Balance balance;
        balance.asset = balanceJson["asset"].get<std::string>();
        balance.free = std::stod(balanceJson["free"].get<std::string>());
        balance.locked = std::stod(balanceJson["locked"].get<std::string>());
        balance.total = balance.free + balance.locked;
        balances.push_back(balance);
    }
    return balances;
}

Balance BinanceConnector::getBalance(const std::string& asset) {
    auto balances = getBalances();
    for (const auto& balance : balances) {
        if (balance.asset == asset) {
            return balance;
        }
    }
    throw std::runtime_error("Asset not found: " + asset);
}

double BinanceConnector::getCurrentPrice(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isPriceCacheValid()) {
        updatePriceCache();
    }

    auto it = priceCache_.find(symbol);
    if (it != priceCache_.end()) {
        return it->second;
    }

    std::string query = "symbol=" + symbol;
    std::string response = makeRequest(TICKER_PRICE + std::string("?") + query);
    auto json = nlohmann::json::parse(response);
    double price = std::stod(json["price"].get<std::string>());
    priceCache_[symbol] = price;
    return price;
}

std::vector<std::string> BinanceConnector::getAvailableSymbols() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string response = makeRequest(EXCHANGE_INFO);
    auto json = nlohmann::json::parse(response);

    std::vector<std::string> symbols;
    for (const auto& symbolInfo : json["symbols"]) {
        symbols.push_back(symbolInfo["symbol"].get<std::string>());
    }
    return symbols;
}

std::string BinanceConnector::getExchangeName() const {
    return "Binance";
}

bool BinanceConnector::isConnected() const {
    return connected_;
}

std::string BinanceConnector::makeRequest(const std::string& endpoint, bool useAuth) {
    if (!curl_) {
        throw std::runtime_error("CURL not initialized");
    }

    std::string url = BASE_URL + endpoint;
    std::string response;

    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);

    if (useAuth) {
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

        size_t pos = endpoint.find('?');
        if (pos != std::string::npos) {
            std::string query = endpoint.substr(pos + 1);
            std::string signature = signRequest(query);
            url += "&signature=" + signature;
            curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        }
    }

    CURLcode res = curl_easy_perform(curl_);
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    return response;
}

std::string BinanceConnector::signRequest(const std::string& query) {
    unsigned char* digest = HMAC(EVP_sha256(), 
        apiSecret_.c_str(), apiSecret_.length(),
        (unsigned char*)query.c_str(), query.length(),
        nullptr, nullptr);

    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return ss.str();
}

size_t BinanceConnector::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t realsize = size * nmemb;
    userp->append((char*)contents, realsize);
    return realsize;
}

void BinanceConnector::updatePriceCache() {
    std::string response = makeRequest(TICKER_PRICE);
    auto json = nlohmann::json::parse(response);
    for (const auto& item : json) {
        std::string symbol = item["symbol"].get<std::string>();
        double price = std::stod(item["price"].get<std::string>());
        priceCache_[symbol] = price;
    }
    lastPriceUpdate_ = std::chrono::system_clock::now();
}

bool BinanceConnector::isPriceCacheValid() const {
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - lastPriceUpdate_);
    return diff.count() < 60; // Cache valid for 1 minute
}

} // namespace exchange
} // namespace tradingbot 