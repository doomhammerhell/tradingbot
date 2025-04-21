#include "visualization/WebInterface.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <random>
#include <chrono>
#include <openssl/sha.h>
#include <zlib.h>

namespace tradingbot {
namespace visualization {

WebInterface::WebInterface(std::shared_ptr<core::TradingEngine> engine, int port)
    : engine_(engine), running_(false), port_(port) {
}

WebInterface::~WebInterface() {
    stop();
}

void WebInterface::start() {
    if (running_) {
        return;
    }

    running_ = true;
    server_ = std::make_unique<httplib::Server>();
    ws_server_ = std::make_unique<WebSocketServer>();
    setupRoutes();

    // Start HTTP server
    serverThread_ = std::thread([this]() {
        server_->listen("0.0.0.0", port_);
    });

    // Start WebSocket server
    ws_server_->init_asio();
    ws_server_->set_open_handler([this](WebSocketConnection hdl) {
        onWebSocketOpen(hdl);
    });
    ws_server_->set_close_handler([this](WebSocketConnection hdl) {
        onWebSocketClose(hdl);
    });
    ws_server_->set_message_handler([this](WebSocketConnection hdl, WebSocketMessage msg) {
        onWebSocketMessage(hdl, msg);
    });

    wsThread_ = std::thread([this]() {
        ws_server_->listen(port_ + 1);
        ws_server_->start_accept();
        ws_server_->run();
    });
}

void WebInterface::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    server_->stop();
    ws_server_->stop();
    
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    if (wsThread_.joinable()) {
        wsThread_.join();
    }
}

void WebInterface::setAuth(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
}

void WebInterface::setupRoutes() {
    // Authentication endpoints
    server_->Post("/api/login", [this](const httplib::Request& req, httplib::Response& res) {
        handleLogin(req, res);
    });
    
    server_->Post("/api/logout", [this](const httplib::Request& req, httplib::Response& res) {
        handleLogout(req, res);
    });

    // Status endpoints
    server_->Get("/api/status", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleGetStatus(req, res);
    });
    
    server_->Get("/api/metrics", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleGetMetrics(req, res);
    });
    
    server_->Get("/api/trades", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleGetTrades(req, res);
    });
    
    // Control endpoints
    server_->Post("/api/start", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleStart(req, res);
    });
    
    server_->Post("/api/stop", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleStop(req, res);
    });
    
    server_->Post("/api/pause", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handlePause(req, res);
    });
    
    server_->Post("/api/resume", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleResume(req, res);
    });
    
    server_->Post("/api/strategy", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleSetStrategy(req, res);
    });
    
    // Configuration endpoints
    server_->Get("/api/config", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleGetConfig(req, res);
    });
    
    server_->Post("/api/config", [this](const httplib::Request& req, httplib::Response& res) {
        if (!authenticate(req)) {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Unauthorized"})", "application/json");
            return;
        }
        handleUpdateConfig(req, res);
    });
    
    // Serve static files
    server_->set_mount_point("/", "web/");
}

void WebInterface::handleLogin(const httplib::Request& req, httplib::Response& res) {
    try {
        auto json = nlohmann::json::parse(req.body);
        std::string username = json["username"];
        std::string password = json["password"];

        if (username == username_ && password == password_) {
            std::string token = generateToken(username);
            res.set_content(R"({"status": "success", "token": ")" + token + "\"}", "application/json");
        } else {
            res.status = 401;
            res.set_content(R"({"status": "error", "message": "Invalid credentials"})", "application/json");
        }
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

void WebInterface::handleLogout(const httplib::Request& req, httplib::Response& res) {
    try {
        auto json = nlohmann::json::parse(req.body);
        std::string token = json["token"];
        
        // Invalidate token (in a real implementation, you would store tokens in a database)
        res.set_content(R"({"status": "success", "message": "Logged out"})", "application/json");
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

void WebInterface::onWebSocketOpen(WebSocketConnection hdl) {
    std::lock_guard<std::mutex> lock(mutex_);
    ws_connections_[hdl] = "";
}

void WebInterface::onWebSocketClose(WebSocketConnection hdl) {
    std::lock_guard<std::mutex> lock(mutex_);
    ws_connections_.erase(hdl);
}

void WebInterface::onWebSocketMessage(WebSocketConnection hdl, WebSocketMessage msg) {
    try {
        auto json = nlohmann::json::parse(msg->get_payload());
        if (json.contains("type") && json["type"] == "auth") {
            std::string token = json["token"];
            if (validateToken(token)) {
                std::lock_guard<std::mutex> lock(mutex_);
                ws_connections_[hdl] = token;
            } else {
                ws_server_->close(hdl, websocketpp::close::status::policy_violation, "Invalid token");
            }
        }
    } catch (const std::exception& e) {
        ws_server_->close(hdl, websocketpp::close::status::invalid_payload, e.what());
    }
}

void WebInterface::broadcastMessage(const std::string& type, const nlohmann::json& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json message = {
        {"type", type},
        {"payload", payload}
    };
    
    for (const auto& [hdl, token] : ws_connections_) {
        if (!token.empty()) {  // Only send to authenticated connections
            ws_server_->send(hdl, message.dump(), websocketpp::frame::opcode::text);
        }
    }
}

bool WebInterface::authenticate(const httplib::Request& req) {
    if (username_.empty() || password_.empty()) {
        return true;  // No authentication required
    }

    auto auth = req.get_header_value("Authorization");
    if (auth.empty()) {
        return false;
    }

    // Check if it's a Bearer token
    if (auth.find("Bearer ") == 0) {
        std::string token = auth.substr(7);
        return validateToken(token);
    }

    return false;
}

std::string WebInterface::generateToken(const std::string& username) {
    // Generate a random token (in a real implementation, use JWT or similar)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex = "0123456789ABCDEF";
    std::string token;
    for (int i = 0; i < 32; ++i) {
        token += hex[dis(gen)];
    }
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    token += std::to_string(timestamp);
    
    // Hash the token
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, token.c_str(), token.size());
    SHA256_Final(hash, &sha256);
    
    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

bool WebInterface::validateToken(const std::string& token) {
    // In a real implementation, you would:
    // 1. Check if the token is in a database of valid tokens
    // 2. Verify the token hasn't expired
    // 3. Check the token's signature
    return !token.empty();
}

void WebInterface::handleGetStatus(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    res.set_content(getEngineStatus().dump(), "application/json");
}

void WebInterface::handleGetMetrics(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    res.set_content(getPerformanceMetrics().dump(), "application/json");
}

void WebInterface::handleGetTrades(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    int limit = 100;
    if (req.has_param("limit")) {
        limit = std::stoi(req.get_param_value("limit"));
    }
    res.set_content(getRecentTrades(limit).dump(), "application/json");
}

void WebInterface::handleStart(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        engine_->start();
        res.set_content(R"({"status": "success", "message": "Trading bot started"})", "application/json");
    }
    catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

void WebInterface::handleStop(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        engine_->stop();
        res.set_content(R"({"status": "success", "message": "Trading bot stopped"})", "application/json");
    }
    catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

void WebInterface::handlePause(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        engine_->pause();
        res.set_content(R"({"status": "success", "message": "Trading bot paused"})", "application/json");
    }
    catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

void WebInterface::handleResume(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        engine_->resume();
        res.set_content(R"({"status": "success", "message": "Trading bot resumed"})", "application/json");
    }
    catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

void WebInterface::handleSetStrategy(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        auto json = nlohmann::json::parse(req.body);
        std::string strategyName = json["strategy"];
        engine_->setStrategy(strategyName);
        res.set_content(R"({"status": "success", "message": "Strategy changed"})", "application/json");
    }
    catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

void WebInterface::handleGetConfig(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        std::ifstream configFile("config.json");
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to open config file");
        }
        
        nlohmann::json config;
        configFile >> config;
        res.set_content(config.dump(), "application/json");
    }
    catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

void WebInterface::handleUpdateConfig(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        auto config = nlohmann::json::parse(req.body);
        validateConfig(config);
        
        std::ofstream configFile("config.json");
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to open config file for writing");
        }
        
        configFile << config.dump(4);
        res.set_content(R"({"status": "success", "message": "Configuration updated"})", "application/json");
    }
    catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"status": "error", "message": ")" + std::string(e.what()) + "\"}", "application/json");
    }
}

nlohmann::json WebInterface::getEngineStatus() {
    auto state = engine_->getState();
    return {
        {"isLive", state.isLive},
        {"isRunning", state.isRunning},
        {"isPaused", state.isPaused},
        {"currentStrategy", state.currentStrategy},
        {"lastUpdate", state.lastUpdate}
    };
}

nlohmann::json WebInterface::getPerformanceMetrics() {
    auto metrics = engine_->getPerformanceMetrics();
    return {
        {"totalTrades", metrics.totalTrades},
        {"winningTrades", metrics.winningTrades},
        {"losingTrades", metrics.losingTrades},
        {"winRatio", metrics.winRatio},
        {"totalPnL", metrics.totalPnL},
        {"maxDrawdown", metrics.maxDrawdown},
        {"sharpeRatio", metrics.sharpeRatio},
        {"sortinoRatio", metrics.sortinoRatio},
        {"profitFactor", metrics.profitFactor}
    };
}

nlohmann::json WebInterface::getRecentTrades(int limit) {
    auto trades = engine_->getRecentTrades(limit);
    nlohmann::json jsonTrades = nlohmann::json::array();
    
    for (const auto& trade : trades) {
        jsonTrades.push_back({
            {"symbol", trade.symbol},
            {"entryTime", trade.entryTime},
            {"exitTime", trade.exitTime},
            {"entryPrice", trade.entryPrice},
            {"exitPrice", trade.exitPrice},
            {"quantity", trade.quantity},
            {"pnl", trade.pnl},
            {"isWin", trade.isWin}
        });
    }
    
    return jsonTrades;
}

void WebInterface::validateConfig(const nlohmann::json& config) {
    // Validate required fields
    if (!config.contains("trading")) {
        throw std::runtime_error("Missing trading configuration");
    }
    
    if (!config.contains("data")) {
        throw std::runtime_error("Missing data configuration");
    }
    
    if (!config.contains("strategies")) {
        throw std::runtime_error("Missing strategies configuration");
    }
    
    // Validate trading configuration
    const auto& trading = config["trading"];
    if (!trading.contains("initialCapital")) {
        throw std::runtime_error("Missing initialCapital in trading configuration");
    }
    
    // Validate data configuration
    const auto& data = config["data"];
    if (!data.contains("feedType")) {
        throw std::runtime_error("Missing feedType in data configuration");
    }
    
    // Validate strategies configuration
    const auto& strategies = config["strategies"];
    if (strategies.empty()) {
        throw std::runtime_error("No strategies configured");
    }
}

WebInterface::CorrelationData WebInterface::calculateCorrelationMatrix() {
    CorrelationData data;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    // Get all active symbols
    auto symbols = engine_->getActiveSymbols();
    data.assets = symbols;
    
    // Initialize correlation matrix
    size_t n = symbols.size();
    data.matrix.resize(n, std::vector<double>(n, 0.0));
    
    // Calculate correlations
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i; j < n; ++j) {
            if (i == j) {
                data.matrix[i][j] = 1.0;
            } else {
                // Get price data for both symbols
                auto prices1 = engine_->getHistoricalPrices(symbols[i]);
                auto prices2 = engine_->getHistoricalPrices(symbols[j]);
                
                // Calculate correlation
                double correlation = calculateCorrelation(prices1, prices2);
                data.matrix[i][j] = correlation;
                data.matrix[j][i] = correlation;
            }
        }
    }
    
    return data;
}

WebInterface::HeatmapData WebInterface::generateMarketHeatmap() {
    HeatmapData data;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    // Get all active symbols
    auto symbols = engine_->getActiveSymbols();
    
    // Generate random positions and values for demonstration
    // In a real implementation, you would use actual market data
    for (size_t i = 0; i < symbols.size(); ++i) {
        HeatmapData::AssetPoint point;
        point.x = static_cast<double>(i) / symbols.size();
        point.y = static_cast<double>(i) / symbols.size();
        point.value = engine_->getSymbolPerformance(symbols[i]);
        data.assets.push_back(point);
    }
    
    return data;
}

WebInterface::TradeDistributionData WebInterface::calculateTradeDistribution() {
    TradeDistributionData data;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    // Initialize hourly distribution
    data.hours.resize(24, 0);
    
    // Get all trades
    auto trades = engine_->getTradeHistory();
    
    // Count trades by hour
    for (const auto& trade : trades) {
        std::time_t time = std::chrono::system_clock::to_time_t(trade.entryTime);
        std::tm* tm = std::localtime(&time);
        data.hours[tm->tm_hour]++;
    }
    
    return data;
}

WebInterface::AssetPerformanceData WebInterface::calculateAssetPerformance() {
    AssetPerformanceData data;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    // Get all active symbols
    auto symbols = engine_->getActiveSymbols();
    
    // Calculate win rates for each symbol
    for (const auto& symbol : symbols) {
        data.assets.push_back(symbol);
        data.winRates.push_back(engine_->getSymbolWinRate(symbol));
    }
    
    return data;
}

void WebInterface::sendCorrelationData(const httplib::Request& req, httplib::Response& res) {
    auto data = calculateCorrelationMatrix();
    nlohmann::json response;
    response["matrix"] = data.matrix;
    response["assets"] = data.assets;
    res.set_content(response.dump(), "application/json");
}

void WebInterface::sendHeatmapData(const httplib::Request& req, httplib::Response& res) {
    auto data = generateMarketHeatmap();
    nlohmann::json response;
    nlohmann::json assets = nlohmann::json::array();
    
    for (const auto& asset : data.assets) {
        nlohmann::json point;
        point["x"] = asset.x;
        point["y"] = asset.y;
        point["value"] = asset.value;
        assets.push_back(point);
    }
    
    response["assets"] = assets;
    res.set_content(response.dump(), "application/json");
}

void WebInterface::sendTradeDistributionData(const httplib::Request& req, httplib::Response& res) {
    auto data = calculateTradeDistribution();
    nlohmann::json response;
    response["hours"] = data.hours;
    res.set_content(response.dump(), "application/json");
}

void WebInterface::sendAssetPerformanceData(const httplib::Request& req, httplib::Response& res) {
    auto data = calculateAssetPerformance();
    nlohmann::json response;
    response["assets"] = data.assets;
    response["winRates"] = data.winRates;
    res.set_content(response.dump(), "application/json");
}

TechnicalIndicators::MovingAverage WebInterface::calculateMovingAverages(const std::string& symbol, int period) {
    TechnicalIndicators::MovingAverage ma;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto prices = engine_->getHistoricalPrices(symbol);
    if (prices.size() < period) return ma;
    
    // Calculate Simple Moving Average (SMA)
    for (size_t i = period - 1; i < prices.size(); ++i) {
        double sum = 0.0;
        for (int j = 0; j < period; ++j) {
            sum += prices[i - j];
        }
        ma.sma.push_back(sum / period);
        ma.timestamps.push_back(std::chrono::system_clock::now() - std::chrono::hours(i));
    }
    
    // Calculate Exponential Moving Average (EMA)
    double multiplier = 2.0 / (period + 1);
    ma.ema.push_back(ma.sma[0]); // First EMA is same as SMA
    
    for (size_t i = 1; i < ma.sma.size(); ++i) {
        double ema = (prices[i + period - 1] - ma.ema[i-1]) * multiplier + ma.ema[i-1];
        ma.ema.push_back(ema);
    }
    
    return ma;
}

TechnicalIndicators::Oscillator WebInterface::calculateOscillators(const std::string& symbol, int period) {
    TechnicalIndicators::Oscillator osc;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto prices = engine_->getHistoricalPrices(symbol);
    if (prices.size() < period + 1) return osc;
    
    // Calculate RSI
    std::vector<double> gains, losses;
    for (size_t i = 1; i < prices.size(); ++i) {
        double change = prices[i] - prices[i-1];
        gains.push_back(change > 0 ? change : 0);
        losses.push_back(change < 0 ? -change : 0);
    }
    
    for (size_t i = period; i < gains.size(); ++i) {
        double avgGain = 0.0, avgLoss = 0.0;
        for (int j = 0; j < period; ++j) {
            avgGain += gains[i-j];
            avgLoss += losses[i-j];
        }
        avgGain /= period;
        avgLoss /= period;
        
        double rs = avgGain / (avgLoss + 1e-10);
        double rsi = 100 - (100 / (1 + rs));
        osc.rsi.push_back(rsi);
        osc.timestamps.push_back(std::chrono::system_clock::now() - std::chrono::hours(i));
    }
    
    // Calculate MACD
    auto ema12 = calculateMovingAverages(symbol, 12).ema;
    auto ema26 = calculateMovingAverages(symbol, 26).ema;
    
    for (size_t i = 0; i < ema12.size(); ++i) {
        osc.macd.push_back(ema12[i] - ema26[i]);
    }
    
    // Calculate MACD Signal Line (9-day EMA of MACD)
    double signalMultiplier = 2.0 / (9 + 1);
    osc.signal.push_back(osc.macd[0]);
    
    for (size_t i = 1; i < osc.macd.size(); ++i) {
        double signal = (osc.macd[i] - osc.signal[i-1]) * signalMultiplier + osc.signal[i-1];
        osc.signal.push_back(signal);
    }
    
    return osc;
}

TechnicalIndicators::VolumeAnalysis WebInterface::analyzeVolume(const std::string& symbol) {
    TechnicalIndicators::VolumeAnalysis vol;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto trades = engine_->getTradeHistory(symbol);
    if (trades.empty()) return vol;
    
    double obv = 0.0;
    double vwapNumerator = 0.0;
    double vwapDenominator = 0.0;
    
    for (const auto& trade : trades) {
        vol.volume.push_back(trade.quantity);
        vol.timestamps.push_back(trade.entryTime);
        
        // Calculate OBV
        if (trade.exitPrice > trade.entryPrice) {
            obv += trade.quantity;
        } else if (trade.exitPrice < trade.entryPrice) {
            obv -= trade.quantity;
        }
        vol.obv.push_back(obv);
        
        // Calculate VWAP
        vwapNumerator += trade.quantity * trade.entryPrice;
        vwapDenominator += trade.quantity;
        vol.vwap.push_back(vwapNumerator / vwapDenominator);
    }
    
    return vol;
}

TechnicalIndicators::SupportResistance WebInterface::calculateSupportResistance(const std::string& symbol) {
    TechnicalIndicators::SupportResistance sr;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto prices = engine_->getHistoricalPrices(symbol);
    if (prices.size() < 20) return sr;
    
    // Calculate pivot points
    double high = *std::max_element(prices.begin(), prices.end());
    double low = *std::min_element(prices.begin(), prices.end());
    double close = prices.back();
    
    double pivot = (high + low + close) / 3;
    sr.pivotPoints.push_back(pivot);
    
    // Calculate support and resistance levels
    double r1 = 2 * pivot - low;
    double s1 = 2 * pivot - high;
    double r2 = pivot + (high - low);
    double s2 = pivot - (high - low);
    
    sr.resistanceLevels.push_back(r1);
    sr.resistanceLevels.push_back(r2);
    sr.supportLevels.push_back(s1);
    sr.supportLevels.push_back(s2);
    
    return sr;
}

bool WebInterface::authenticateWebSocket(const std::string& token) {
    // Validate token (implement your token validation logic)
    return validateToken(token);
}

bool WebInterface::checkRateLimit(const WebSocketConnection& connection) {
    auto now = std::chrono::system_clock::now();
    auto windowStart = now - rateLimitConfig_.windowSize;
    
    if (connection.lastMessage < windowStart) {
        return true; // Window has reset
    }
    
    return connection.messageCount < rateLimitConfig_.maxMessagesPerMinute;
}

void WebInterface::updateRateLimit(WebSocketConnection& connection) {
    auto now = std::chrono::system_clock::now();
    auto windowStart = now - rateLimitConfig_.windowSize;
    
    if (connection.lastMessage < windowStart) {
        connection.messageCount = 0;
    }
    
    connection.messageCount++;
    connection.lastMessage = now;
}

void WebInterface::disconnectExcessiveConnections() {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    
    // Count connections per IP
    std::unordered_map<std::string, int> ipConnections;
    for (const auto& [hdl, conn] : connections_) {
        ipConnections[conn.clientId]++;
    }
    
    // Disconnect excess connections
    for (const auto& [ip, count] : ipConnections) {
        if (count > rateLimitConfig_.maxConnectionsPerIP) {
            for (auto it = connections_.begin(); it != connections_.end();) {
                if (it->second.clientId == ip) {
                    ws_server_->close(it->first, websocketpp::close::status::policy_violation, "Too many connections");
                    it = connections_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

std::string WebInterface::compressMessage(const nlohmann::json& message) {
    std::string jsonStr = message.dump();
    compressionBuffer_.resize(jsonStr.size() * 2); // Reserve enough space
    
    // Use zlib for compression (you'll need to link against zlib)
    uLongf compressedSize = compressionBuffer_.size();
    compress2(reinterpret_cast<Bytef*>(compressionBuffer_.data()), &compressedSize,
              reinterpret_cast<const Bytef*>(jsonStr.data()), jsonStr.size(),
              Z_BEST_COMPRESSION);
    
    return std::string(compressionBuffer_.data(), compressedSize);
}

nlohmann::json WebInterface::decompressMessage(const std::string& compressed) {
    std::string decompressed;
    decompressed.resize(compressed.size() * 10); // Reserve enough space
    
    uLongf decompressedSize = decompressed.size();
    uncompress(reinterpret_cast<Bytef*>(decompressed.data()), &decompressedSize,
               reinterpret_cast<const Bytef*>(compressed.data()), compressed.size());
    
    decompressed.resize(decompressedSize);
    return nlohmann::json::parse(decompressed);
}

void WebInterface::sendMovingAverages(const httplib::Request& req, httplib::Response& res) {
    std::string symbol = req.get_param_value("symbol");
    int period = std::stoi(req.get_param_value("period", "20"));
    
    const std::string cacheKey = "moving_averages_" + symbol + "_" + std::to_string(period);
    if (isCacheValid(cacheKey, std::chrono::seconds(60))) {
        res.set_content(getCachedData(cacheKey).dump(), "application/json");
        return;
    }
    
    auto data = calculateMovingAverages(symbol, period);
    nlohmann::json response;
    response["sma"] = data.sma;
    response["ema"] = data.ema;
    response["timestamps"] = data.timestamps;
    
    updateCache(cacheKey, response);
    res.set_content(response.dump(), "application/json");
}

void WebInterface::sendOscillators(const httplib::Request& req, httplib::Response& res) {
    std::string symbol = req.get_param_value("symbol");
    int period = std::stoi(req.get_param_value("period", "14"));
    
    const std::string cacheKey = "oscillators_" + symbol + "_" + std::to_string(period);
    if (isCacheValid(cacheKey, std::chrono::seconds(60))) {
        res.set_content(getCachedData(cacheKey).dump(), "application/json");
        return;
    }
    
    auto data = calculateOscillators(symbol, period);
    nlohmann::json response;
    response["rsi"] = data.rsi;
    response["macd"] = data.macd;
    response["signal"] = data.signal;
    response["timestamps"] = data.timestamps;
    
    updateCache(cacheKey, response);
    res.set_content(response.dump(), "application/json");
}

void WebInterface::sendVolumeAnalysis(const httplib::Request& req, httplib::Response& res) {
    std::string symbol = req.get_param_value("symbol");
    
    const std::string cacheKey = "volume_analysis_" + symbol;
    if (isCacheValid(cacheKey, std::chrono::seconds(30))) {
        res.set_content(getCachedData(cacheKey).dump(), "application/json");
        return;
    }
    
    auto data = analyzeVolume(symbol);
    nlohmann::json response;
    response["volume"] = data.volume;
    response["obv"] = data.obv;
    response["vwap"] = data.vwap;
    response["timestamps"] = data.timestamps;
    
    updateCache(cacheKey, response);
    res.set_content(response.dump(), "application/json");
}

void WebInterface::sendSupportResistance(const httplib::Request& req, httplib::Response& res) {
    std::string symbol = req.get_param_value("symbol");
    
    const std::string cacheKey = "support_resistance_" + symbol;
    if (isCacheValid(cacheKey, std::chrono::seconds(300))) {
        res.set_content(getCachedData(cacheKey).dump(), "application/json");
        return;
    }
    
    auto data = calculateSupportResistance(symbol);
    nlohmann::json response;
    response["supportLevels"] = data.supportLevels;
    response["resistanceLevels"] = data.resistanceLevels;
    response["pivotPoints"] = data.pivotPoints;
    
    updateCache(cacheKey, response);
    res.set_content(response.dump(), "application/json");
}

TechnicalIndicators::FibonacciRetracements WebInterface::calculateFibonacciRetracements(const std::string& symbol, int period) {
    TechnicalIndicators::FibonacciRetracements fib;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto prices = engine_->getHistoricalPrices(symbol);
    if (prices.size() < period) return fib;
    
    // Find high and low in the period
    double high = *std::max_element(prices.end() - period, prices.end());
    double low = *std::min_element(prices.end() - period, prices.end());
    double range = high - low;
    
    // Calculate Fibonacci levels
    fib.levels = {
        high - range * 0.236,
        high - range * 0.382,
        high - range * 0.5,
        high - range * 0.618,
        high - range * 0.786
    };
    
    fib.high = high;
    fib.low = low;
    fib.timestamp = std::chrono::system_clock::now();
    
    return fib;
}

TechnicalIndicators::ParabolicSAR WebInterface::calculateParabolicSAR(const std::string& symbol, double acceleration, double maximum) {
    TechnicalIndicators::ParabolicSAR sar;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto prices = engine_->getHistoricalPrices(symbol);
    if (prices.size() < 2) return sar;
    
    bool uptrend = prices[1] > prices[0];
    double ep = uptrend ? prices[0] : prices[0];  // Extreme point
    double sarValue = uptrend ? prices[0] : prices[0];
    double af = acceleration;  // Acceleration factor
    
    for (size_t i = 1; i < prices.size(); ++i) {
        // Update extreme point
        if (uptrend) {
            ep = std::max(ep, prices[i]);
        } else {
            ep = std::min(ep, prices[i]);
        }
        
        // Calculate SAR
        sarValue = sarValue + af * (ep - sarValue);
        
        // Check for trend reversal
        if ((uptrend && prices[i] < sarValue) || (!uptrend && prices[i] > sarValue)) {
            uptrend = !uptrend;
            sarValue = ep;
            ep = prices[i];
            af = acceleration;
        } else {
            // Increase acceleration factor
            af = std::min(af + acceleration, maximum);
        }
        
        sar.sar.push_back(sarValue);
        sar.trend.push_back(uptrend);
        sar.timestamps.push_back(std::chrono::system_clock::now() - std::chrono::hours(i));
    }
    
    return sar;
}

TechnicalIndicators::ATR WebInterface::calculateATR(const std::string& symbol, int period) {
    TechnicalIndicators::ATR atr;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto prices = engine_->getHistoricalPrices(symbol);
    if (prices.size() < period + 1) return atr;
    
    // Calculate True Range
    std::vector<double> tr;
    for (size_t i = 1; i < prices.size(); ++i) {
        double high = std::max(prices[i], prices[i-1]);
        double low = std::min(prices[i], prices[i-1]);
        tr.push_back(high - low);
    }
    
    // Calculate ATR
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += tr[i];
    }
    atr.atr.push_back(sum / period);
    
    for (size_t i = period; i < tr.size(); ++i) {
        atr.atr.push_back((atr.atr.back() * (period - 1) + tr[i]) / period);
    }
    
    // Add timestamps
    for (size_t i = 0; i < atr.atr.size(); ++i) {
        atr.timestamps.push_back(std::chrono::system_clock::now() - std::chrono::hours(i));
    }
    
    return atr;
}

TechnicalIndicators::ADX WebInterface::calculateADX(const std::string& symbol, int period) {
    TechnicalIndicators::ADX adx;
    std::lock_guard<std::mutex> lock(engine_mutex_);
    
    auto prices = engine_->getHistoricalPrices(symbol);
    if (prices.size() < period + 1) return adx;
    
    // Calculate +DM and -DM
    std::vector<double> plusDM, minusDM;
    for (size_t i = 1; i < prices.size(); ++i) {
        double upMove = prices[i] - prices[i-1];
        double downMove = prices[i-1] - prices[i];
        
        if (upMove > downMove && upMove > 0) {
            plusDM.push_back(upMove);
            minusDM.push_back(0);
        } else if (downMove > upMove && downMove > 0) {
            plusDM.push_back(0);
            minusDM.push_back(downMove);
        } else {
            plusDM.push_back(0);
            minusDM.push_back(0);
        }
    }
    
    // Calculate +DI and -DI
    std::vector<double> plusDI, minusDI;
    double sumPlusDM = 0.0, sumMinusDM = 0.0;
    double sumTR = 0.0;
    
    for (int i = 0; i < period; ++i) {
        sumPlusDM += plusDM[i];
        sumMinusDM += minusDM[i];
        sumTR += std::abs(prices[i+1] - prices[i]);
    }
    
    plusDI.push_back(100 * sumPlusDM / sumTR);
    minusDI.push_back(100 * sumMinusDM / sumTR);
    
    for (size_t i = period; i < plusDM.size(); ++i) {
        sumPlusDM = (sumPlusDM * (period - 1) + plusDM[i]) / period;
        sumMinusDM = (sumMinusDM * (period - 1) + minusDM[i]) / period;
        sumTR = (sumTR * (period - 1) + std::abs(prices[i+1] - prices[i])) / period;
        
        plusDI.push_back(100 * sumPlusDM / sumTR);
        minusDI.push_back(100 * sumMinusDM / sumTR);
    }
    
    // Calculate ADX
    for (size_t i = 0; i < plusDI.size(); ++i) {
        double dx = 100 * std::abs(plusDI[i] - minusDI[i]) / (plusDI[i] + minusDI[i]);
        if (i < period) {
            adx.adx.push_back(dx);
        } else {
            adx.adx.push_back((adx.adx.back() * (period - 1) + dx) / period);
        }
    }
    
    adx.plusDI = plusDI;
    adx.minusDI = minusDI;
    
    // Add timestamps
    for (size_t i = 0; i < adx.adx.size(); ++i) {
        adx.timestamps.push_back(std::chrono::system_clock::now() - std::chrono::hours(i));
    }
    
    return adx;
}

void WebInterface::addToBatch(const std::string& type, const nlohmann::json& data, MessagePriority priority) {
    std::lock_guard<std::mutex> lock(batchMutex_);
    messageBatch_.push_back({type, data, std::chrono::system_clock::now(), priority, 0});
}

void WebInterface::processBatchWithPriority() {
    while (batchProcessing_) {
        std::this_thread::sleep_for(batchInterval_);
        
        std::lock_guard<std::mutex> lock(batchMutex_);
        if (messageBatch_.empty()) continue;
        
        // Sort messages by priority and timestamp
        std::sort(messageBatch_.begin(), messageBatch_.end(),
            [](const BatchedMessage& a, const BatchedMessage& b) {
                if (a.priority != b.priority) {
                    return a.priority < b.priority;  // HIGH < MEDIUM < LOW
                }
                return a.timestamp < b.timestamp;
            });
        
        // Group messages by type and priority
        std::map<std::pair<std::string, MessagePriority>, nlohmann::json> groupedMessages;
        for (const auto& msg : messageBatch_) {
            auto key = std::make_pair(msg.type, msg.priority);
            if (!groupedMessages.contains(key)) {
                groupedMessages[key] = nlohmann::json::array();
            }
            groupedMessages[key].push_back(msg.data);
        }
        
        // Send batched messages
        for (const auto& [key, data] : groupedMessages) {
            nlohmann::json batchedMessage = {
                {"type", key.first},
                {"priority", static_cast<int>(key.second)},
                {"data", data},
                {"timestamp", std::chrono::system_clock::now()}
            };
            
            std::string compressed = compressMessage(batchedMessage);
            broadcastMessage(key.first, compressed);
        }
        
        clearBatch();
    }
}

void WebInterface::handleWebSocketError(WebSocketConnection hdl, const std::string& error) {
    std::lock_guard<std::mutex> lock(connectionStateMutex_);
    auto& state = connectionStates_[hdl];
    state.lastError = error;
    state.consecutiveFailures++;
    
    if (state.consecutiveFailures >= 3) {
        recoverWebSocketConnection(hdl);
    }
}

void WebInterface::recoverWebSocketConnection(WebSocketConnection hdl) {
    std::lock_guard<std::mutex> lock(connectionStateMutex_);
    auto& state = connectionStates_[hdl];
    
    // Attempt to reconnect
    try {
        ws_server_->close(hdl, websocketpp::close::status::going_away, "Reconnecting...");
        state.isConnected = false;
        state.consecutiveFailures = 0;
        
        // Reconnect after a delay
        std::this_thread::sleep_for(std::chrono::seconds(5));
        ws_server_->connect(hdl);
        
        // Resend pending messages
        for (const auto& msg : state.pendingMessages) {
            if (msg.retryCount < 3) {
                msg.retryCount++;
                addToBatch(msg.type, msg.data, msg.priority);
            }
        }
        state.pendingMessages.clear();
        
    } catch (const std::exception& e) {
        state.lastError = e.what();
        state.consecutiveFailures++;
    }
}

void WebInterface::sendHeartbeat(WebSocketConnection hdl) {
    try {
        nlohmann::json heartbeat = {
            {"type", "heartbeat"},
            {"timestamp", std::chrono::system_clock::now()}
        };
        ws_server_->send(hdl, heartbeat.dump(), websocketpp::frame::opcode::text);
        
        std::lock_guard<std::mutex> lock(connectionStateMutex_);
        connectionStates_[hdl].lastHeartbeat = std::chrono::system_clock::now();
        
    } catch (const std::exception& e) {
        handleWebSocketError(hdl, e.what());
    }
}

void WebInterface::checkConnectionHealth() {
    while (healthCheckRunning_) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        
        std::lock_guard<std::mutex> lock(connectionStateMutex_);
        auto now = std::chrono::system_clock::now();
        
        for (auto& [hdl, state] : connectionStates_) {
            if (state.isConnected) {
                auto timeSinceLastHeartbeat = std::chrono::duration_cast<std::chrono::seconds>(
                    now - state.lastHeartbeat);
                
                if (timeSinceLastHeartbeat > std::chrono::seconds(60)) {
                    sendHeartbeat(hdl);
                }
                
                if (timeSinceLastHeartbeat > std::chrono::seconds(120)) {
                    handleWebSocketError(hdl, "Connection timeout");
                }
            }
        }
    }
}

} // namespace visualization
} // namespace tradingbot 