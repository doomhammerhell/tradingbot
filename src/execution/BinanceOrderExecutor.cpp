#include "../../include/execution/BinanceOrderExecutor.hpp"
#include <fstream>
#include <sstream>
#include <json/json.h>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <chrono>

namespace tradingbot {
namespace execution {

namespace {
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
}

BinanceOrderExecutor::BinanceOrderExecutor()
    : isConnected_(false)
{}

BinanceOrderExecutor::~BinanceOrderExecutor() {
    disconnect();
}

void BinanceOrderExecutor::initialize(const std::string& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Failed to open config file: " + configPath);
    }

    Json::Value root;
    configFile >> root;

    apiKey_ = root["api_key"].asString();
    apiSecret_ = root["api_secret"].asString();
    baseUrl_ = root["base_url"].asString();
}

void BinanceOrderExecutor::connect() {
    if (isConnected_) {
        return;
    }

    isConnected_ = true;
    startWebsocket();
    updateBalances();
    updatePositions();
}

void BinanceOrderExecutor::disconnect() {
    if (!isConnected_) {
        return;
    }

    isConnected_ = false;
    stopWebsocket();
}

core::Order BinanceOrderExecutor::placeOrder(const core::Order& order) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::stringstream query;
    query << "symbol=" << order.symbol
          << "&side=" << (order.side == core::OrderSide::BUY ? "BUY" : "SELL")
          << "&type=" << (order.type == core::OrderType::MARKET ? "MARKET" : "LIMIT")
          << "&quantity=" << order.quantity;

    if (order.type != core::OrderType::MARKET) {
        query << "&price=" << order.price;
    }

    std::string timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    query << "&timestamp=" << timestamp;

    std::string signature = generateSignature(query.str());
    query << "&signature=" << signature;

    std::string url = baseUrl_ + "/api/v3/order?" + query.str();

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to place order: " + 
                               std::string(curl_easy_strerror(res)));
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse order response");
    }

    core::Order result = order;
    result.id = root["orderId"].asString();
    result.status = core::OrderStatus::NEW;
    result.creationTime = std::chrono::system_clock::now();
    result.updateTime = result.creationTime;

    return result;
}

bool BinanceOrderExecutor::cancelOrder(const std::string& orderId) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::string query = "orderId=" + orderId + "&timestamp=" + timestamp;
    std::string signature = generateSignature(query);
    query += "&signature=" + signature;

    std::string url = baseUrl_ + "/api/v3/order?" + query;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to cancel order: " + 
                               std::string(curl_easy_strerror(res)));
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse cancel order response");
    }

    return root["status"].asString() == "CANCELED";
}

core::Order BinanceOrderExecutor::getOrderStatus(const std::string& orderId) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::string query = "orderId=" + orderId + "&timestamp=" + timestamp;
    std::string signature = generateSignature(query);
    query += "&signature=" + signature;

    std::string url = baseUrl_ + "/api/v3/order?" + query;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to get order status: " + 
                               std::string(curl_easy_strerror(res)));
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse order status response");
    }

    core::Order order;
    order.id = orderId;
    order.symbol = root["symbol"].asString();
    order.type = root["type"].asString() == "MARKET" ? 
                 core::OrderType::MARKET : core::OrderType::LIMIT;
    order.side = root["side"].asString() == "BUY" ? 
                 core::OrderSide::BUY : core::OrderSide::SELL;
    order.quantity = std::stod(root["origQty"].asString());
    order.price = std::stod(root["price"].asString());
    order.executedQuantity = std::stod(root["executedQty"].asString());
    order.averagePrice = std::stod(root["avgPrice"].asString());
    
    std::string status = root["status"].asString();
    if (status == "NEW") {
        order.status = core::OrderStatus::NEW;
    } else if (status == "PARTIALLY_FILLED") {
        order.status = core::OrderStatus::PARTIALLY_FILLED;
    } else if (status == "FILLED") {
        order.status = core::OrderStatus::FILLED;
    } else if (status == "CANCELED") {
        order.status = core::OrderStatus::CANCELED;
    } else if (status == "REJECTED") {
        order.status = core::OrderStatus::REJECTED;
    } else if (status == "EXPIRED") {
        order.status = core::OrderStatus::EXPIRED;
    }

    return order;
}

std::vector<core::Order> BinanceOrderExecutor::getOpenOrders() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::string query = "timestamp=" + timestamp;
    std::string signature = generateSignature(query);
    query += "&signature=" + signature;

    std::string url = baseUrl_ + "/api/v3/openOrders?" + query;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to get open orders: " + 
                               std::string(curl_easy_strerror(res)));
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse open orders response");
    }

    std::vector<core::Order> orders;
    for (const auto& order : root) {
        core::Order o;
        o.id = order["orderId"].asString();
        o.symbol = order["symbol"].asString();
        o.type = order["type"].asString() == "MARKET" ? 
                 core::OrderType::MARKET : core::OrderType::LIMIT;
        o.side = order["side"].asString() == "BUY" ? 
                 core::OrderSide::BUY : core::OrderSide::SELL;
        o.quantity = std::stod(order["origQty"].asString());
        o.price = std::stod(order["price"].asString());
        o.executedQuantity = std::stod(order["executedQty"].asString());
        o.averagePrice = std::stod(order["avgPrice"].asString());
        o.status = core::OrderStatus::NEW;
        orders.push_back(o);
    }

    return orders;
}

double BinanceOrderExecutor::getBalance(const std::string& asset) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto it = balances_.find(asset);
    return it != balances_.end() ? it->second : 0.0;
}

double BinanceOrderExecutor::getPosition(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto it = positions_.find(symbol);
    return it != positions_.end() ? it->second : 0.0;
}

void BinanceOrderExecutor::subscribeOrderUpdates(
    std::function<void(const core::Order&)> callback) {
    
    std::lock_guard<std::mutex> lock(callbackMutex_);
    orderCallback_ = callback;
}

void BinanceOrderExecutor::startWebsocket() {
    // Implementation would use a WebSocket library like libwebsockets
    // This is a placeholder for the actual implementation
    websocketThread_ = std::thread([this]() {
        while (isConnected_) {
            // Process WebSocket messages
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void BinanceOrderExecutor::stopWebsocket() {
    if (websocketThread_.joinable()) {
        websocketThread_.join();
    }
}

void BinanceOrderExecutor::processWebsocketMessage(const std::string& message) {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(message, root)) {
        return;
    }

    core::Order order;
    order.id = root["o"]["i"].asString();
    order.symbol = root["o"]["s"].asString();
    order.type = root["o"]["o"].asString() == "MARKET" ? 
                 core::OrderType::MARKET : core::OrderType::LIMIT;
    order.side = root["o"]["S"].asString() == "BUY" ? 
                 core::OrderSide::BUY : core::OrderSide::SELL;
    order.quantity = std::stod(root["o"]["q"].asString());
    order.price = std::stod(root["o"]["p"].asString());
    order.executedQuantity = std::stod(root["o"]["z"].asString());
    order.averagePrice = std::stod(root["o"]["ap"].asString());
    
    std::string status = root["o"]["X"].asString();
    if (status == "NEW") {
        order.status = core::OrderStatus::NEW;
    } else if (status == "PARTIALLY_FILLED") {
        order.status = core::OrderStatus::PARTIALLY_FILLED;
    } else if (status == "FILLED") {
        order.status = core::OrderStatus::FILLED;
    } else if (status == "CANCELED") {
        order.status = core::OrderStatus::CANCELED;
    } else if (status == "REJECTED") {
        order.status = core::OrderStatus::REJECTED;
    } else if (status == "EXPIRED") {
        order.status = core::OrderStatus::EXPIRED;
    }

    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (orderCallback_) {
        orderCallback_(order);
    }
}

std::string BinanceOrderExecutor::generateSignature(const std::string& queryString) const {
    unsigned char* digest = HMAC(EVP_sha256(),
                               apiSecret_.c_str(),
                               apiSecret_.length(),
                               (unsigned char*)queryString.c_str(),
                               queryString.length(),
                               NULL,
                               NULL);

    char mdString[65];
    for (int i = 0; i < 32; i++) {
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
    }
    mdString[64] = '\0';

    return std::string(mdString);
}

void BinanceOrderExecutor::updateBalances() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::string query = "timestamp=" + timestamp;
    std::string signature = generateSignature(query);
    query += "&signature=" + signature;

    std::string url = baseUrl_ + "/api/v3/account?" + query;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to get account info: " + 
                               std::string(curl_easy_strerror(res)));
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse account info response");
    }

    std::lock_guard<std::mutex> lock(dataMutex_);
    balances_.clear();
    for (const auto& balance : root["balances"]) {
        std::string asset = balance["asset"].asString();
        double amount = std::stod(balance["free"].asString());
        balances_[asset] = amount;
    }
}

void BinanceOrderExecutor::updatePositions() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    std::string query = "timestamp=" + timestamp;
    std::string signature = generateSignature(query);
    query += "&signature=" + signature;

    std::string url = baseUrl_ + "/api/v3/account?" + query;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("X-MBX-APIKEY: " + apiKey_).c_str());

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to get account info: " + 
                               std::string(curl_easy_strerror(res)));
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) {
        throw std::runtime_error("Failed to parse account info response");
    }

    std::lock_guard<std::mutex> lock(dataMutex_);
    positions_.clear();
    for (const auto& position : root["positions"]) {
        std::string symbol = position["symbol"].asString();
        double amount = std::stod(position["positionAmt"].asString());
        positions_[symbol] = amount;
    }
}

} // namespace execution
} // namespace tradingbot 