#pragma once

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "../core/TradingEngine.hpp"

namespace tradingbot {
namespace visualization {

class WebInterface {
public:
    WebInterface(std::shared_ptr<core::TradingEngine> engine, int port = 8080);
    ~WebInterface();

    // Start the web server
    void start();

    // Stop the web server
    void stop();

    // Set authentication credentials
    void setAuth(const std::string& username, const std::string& password);

private:
    using WebSocketServer = websocketpp::server<websocketpp::config::asio>;
    using WebSocketConnection = websocketpp::connection_hdl;
    using WebSocketMessage = websocketpp::server<websocketpp::config::asio>::message_ptr;

    std::shared_ptr<core::TradingEngine> engine_;
    std::unique_ptr<httplib::Server> server_;
    std::unique_ptr<WebSocketServer> ws_server_;
    std::thread serverThread_;
    std::thread wsThread_;
    std::atomic<bool> running_;
    int port_;
    std::mutex mutex_;
    std::string username_;
    std::string password_;
    std::unordered_map<WebSocketConnection, std::string> ws_connections_;

    // New visualization data structures
    struct CorrelationData {
        std::vector<std::vector<double>> matrix;
        std::vector<std::string> assets;
    };

    struct HeatmapData {
        struct AssetPoint {
            double x;
            double y;
            double value;
        };
        std::vector<AssetPoint> assets;
    };

    struct TradeDistributionData {
        std::vector<int> hours;
    };

    struct AssetPerformanceData {
        std::vector<std::string> assets;
        std::vector<double> winRates;
    };

    // New advanced visualization structures
    struct VolatilityData {
        std::vector<std::string> assets;
        std::vector<double> volatilities;
        std::vector<double> returns;
    };

    struct MarketRegimeData {
        std::vector<std::string> regimes;
        std::vector<double> probabilities;
        std::vector<std::chrono::system_clock::time_point> timestamps;
    };

    struct PortfolioRiskData {
        std::vector<std::string> assets;
        std::vector<double> riskContributions;
        std::vector<double> weights;
        double totalRisk;
    };

    struct MarketDepthData {
        std::string symbol;
        std::vector<std::pair<double, double>> bids;  // price, quantity
        std::vector<std::pair<double, double>> asks;  // price, quantity
        std::chrono::system_clock::time_point timestamp;
    };

    // Cache structures
    struct VisualizationCache {
        std::chrono::system_clock::time_point lastUpdate;
        nlohmann::json data;
        bool isValid;
    };

    // Technical Indicator Structures
    struct TechnicalIndicators {
        struct MovingAverage {
            std::vector<double> sma;  // Simple Moving Average
            std::vector<double> ema;  // Exponential Moving Average
            std::vector<std::chrono::system_clock::time_point> timestamps;
        };

        struct Oscillator {
            std::vector<double> rsi;  // Relative Strength Index
            std::vector<double> macd; // Moving Average Convergence Divergence
            std::vector<double> signal; // MACD Signal Line
            std::vector<std::chrono::system_clock::time_point> timestamps;
        };

        struct VolumeAnalysis {
            std::vector<double> volume;
            std::vector<double> obv;  // On-Balance Volume
            std::vector<double> vwap; // Volume Weighted Average Price
            std::vector<std::chrono::system_clock::time_point> timestamps;
        };

        struct SupportResistance {
            std::vector<double> supportLevels;
            std::vector<double> resistanceLevels;
            std::vector<double> pivotPoints;
        };

        struct BollingerBands {
            std::vector<double> upperBand;
            std::vector<double> middleBand;  // SMA
            std::vector<double> lowerBand;
            std::vector<std::chrono::system_clock::time_point> timestamps;
        };

        struct IchimokuCloud {
            struct CloudLine {
                std::vector<double> conversion;  // Tenkan-sen
                std::vector<double> base;        // Kijun-sen
                std::vector<double> leadingA;    // Senkou Span A
                std::vector<double> leadingB;    // Senkou Span B
                std::vector<double> lagging;     // Chikou Span
                std::vector<std::chrono::system_clock::time_point> timestamps;
            };
            CloudLine lines;
            std::vector<std::pair<double, double>> cloud;  // Upper and lower cloud boundaries
        };

        struct MomentumIndicators {
            std::vector<double> stochK;  // Stochastic %K
            std::vector<double> stochD;  // Stochastic %D
            std::vector<double> cci;     // Commodity Channel Index
            std::vector<std::chrono::system_clock::time_point> timestamps;
        };

        struct FibonacciRetracements {
            std::vector<double> levels;  // 0.236, 0.382, 0.5, 0.618, 0.786
            double high;
            double low;
            std::chrono::system_clock::time_point timestamp;
        };

        struct ParabolicSAR {
            std::vector<double> sar;
            std::vector<bool> trend;  // true for uptrend, false for downtrend
            std::vector<std::chrono::system_clock::time_point> timestamps;
        };

        struct ATR {
            std::vector<double> atr;  // Average True Range
            std::vector<std::chrono::system_clock::time_point> timestamps;
        };

        struct ADX {
            std::vector<double> adx;  // Average Directional Index
            std::vector<double> plusDI;  // +DI
            std::vector<double> minusDI;  // -DI
            std::vector<std::chrono::system_clock::time_point> timestamps;
        };
    };

    // WebSocket Connection Management
    struct WebSocketConnection {
        websocketpp::connection_hdl handle;
        std::string clientId;
        std::string token;
        std::chrono::system_clock::time_point lastMessage;
        int messageCount;
        bool authenticated;
    };

    // Rate Limiting Configuration
    struct RateLimitConfig {
        int maxMessagesPerMinute;
        int maxConnectionsPerIP;
        std::chrono::seconds windowSize;
    };

    // WebSocket Message Priority
    enum class MessagePriority {
        HIGH,      // Critical updates (e.g., position changes, errors)
        MEDIUM,    // Regular updates (e.g., price updates, indicators)
        LOW        // Background updates (e.g., historical data)
    };

    // Enhanced WebSocket Message Batching
    struct BatchedMessage {
        std::string type;
        nlohmann::json data;
        std::chrono::system_clock::time_point timestamp;
        MessagePriority priority;
        int retryCount;
    };

    // WebSocket Connection State
    struct ConnectionState {
        bool isConnected;
        std::chrono::system_clock::time_point lastHeartbeat;
        int consecutiveFailures;
        std::string lastError;
        std::vector<BatchedMessage> pendingMessages;
    };

    // API endpoints
    void setupRoutes();
    
    // Status endpoints
    void handleGetStatus(const httplib::Request& req, httplib::Response& res);
    void handleGetMetrics(const httplib::Request& req, httplib::Response& res);
    void handleGetTrades(const httplib::Request& req, httplib::Response& res);
    
    // Control endpoints
    void handleStart(const httplib::Request& req, httplib::Response& res);
    void handleStop(const httplib::Request& req, httplib::Response& res);
    void handlePause(const httplib::Request& req, httplib::Response& res);
    void handleResume(const httplib::Request& req, httplib::Response& res);
    void handleSetStrategy(const httplib::Request& req, httplib::Response& res);
    
    // Configuration endpoints
    void handleGetConfig(const httplib::Request& req, httplib::Response& res);
    void handleUpdateConfig(const httplib::Request& req, httplib::Response& res);

    // Authentication endpoints
    void handleLogin(const httplib::Request& req, httplib::Response& res);
    void handleLogout(const httplib::Request& req, httplib::Response& res);
    
    // WebSocket handlers
    void onWebSocketOpen(WebSocketConnection hdl);
    void onWebSocketClose(WebSocketConnection hdl);
    void onWebSocketMessage(WebSocketConnection hdl, WebSocketMessage msg);
    void broadcastMessage(const std::string& type, const nlohmann::json& payload);
    
    // Helper methods
    nlohmann::json getEngineStatus();
    nlohmann::json getPerformanceMetrics();
    nlohmann::json getRecentTrades(int limit = 100);
    void validateConfig(const nlohmann::json& config);
    bool authenticate(const httplib::Request& req);
    std::string generateToken(const std::string& username);
    bool validateToken(const std::string& token);

    // New visualization data methods
    CorrelationData calculateCorrelationMatrix();
    HeatmapData generateMarketHeatmap();
    TradeDistributionData calculateTradeDistribution();
    AssetPerformanceData calculateAssetPerformance();

    // New WebSocket message handlers
    void sendCorrelationData(const httplib::Request& req, httplib::Response& res);
    void sendHeatmapData(const httplib::Request& req, httplib::Response& res);
    void sendTradeDistributionData(const httplib::Request& req, httplib::Response& res);
    void sendAssetPerformanceData(const httplib::Request& req, httplib::Response& res);

    // New visualization methods
    VolatilityData calculateVolatilityMetrics();
    MarketRegimeData detectMarketRegimes();
    PortfolioRiskData analyzePortfolioRisk();
    MarketDepthData getMarketDepth(const std::string& symbol);
    
    // Cache management
    void updateCache(const std::string& key, const nlohmann::json& data);
    nlohmann::json getCachedData(const std::string& key);
    bool isCacheValid(const std::string& key, std::chrono::seconds maxAge);

    // WebSocket message handlers
    void sendVolatilityData(const httplib::Request& req, httplib::Response& res);
    void sendMarketRegimeData(const httplib::Request& req, httplib::Response& res);
    void sendPortfolioRiskData(const httplib::Request& req, httplib::Response& res);
    void sendMarketDepthData(const httplib::Request& req, httplib::Response& res);

    // WebSocket broadcast methods
    void broadcastVolatilityUpdate();
    void broadcastMarketRegimeUpdate();
    void broadcastPortfolioRiskUpdate();
    void broadcastMarketDepthUpdate(const std::string& symbol);

    // Cache for visualization data
    std::unordered_map<std::string, VisualizationCache> visualizationCache_;
    std::mutex cacheMutex_;

    // New WebSocket message handlers
    void sendMovingAverages(const httplib::Request& req, httplib::Response& res);
    void sendOscillators(const httplib::Request& req, httplib::Response& res);
    void sendVolumeAnalysis(const httplib::Request& req, httplib::Response& res);
    void sendSupportResistance(const httplib::Request& req, httplib::Response& res);

    // New WebSocket broadcast methods
    void broadcastMovingAveragesUpdate(const std::string& symbol);
    void broadcastOscillatorsUpdate(const std::string& symbol);
    void broadcastVolumeAnalysisUpdate(const std::string& symbol);
    void broadcastSupportResistanceUpdate(const std::string& symbol);

    // Connection management
    std::unordered_map<websocketpp::connection_hdl, WebSocketConnection, std::owner_less<websocketpp::connection_hdl>> connections_;
    std::mutex connectionsMutex_;
    RateLimitConfig rateLimitConfig_;

    // Compression buffer
    std::vector<char> compressionBuffer_;

    // New technical indicator methods
    TechnicalIndicators::BollingerBands calculateBollingerBands(const std::string& symbol, int period, double stdDev);
    TechnicalIndicators::IchimokuCloud calculateIchimokuCloud(const std::string& symbol);
    TechnicalIndicators::MomentumIndicators calculateMomentumIndicators(const std::string& symbol, int period);
    TechnicalIndicators::FibonacciRetracements calculateFibonacciRetracements(const std::string& symbol, int period);
    TechnicalIndicators::ParabolicSAR calculateParabolicSAR(const std::string& symbol, double acceleration = 0.02, double maximum = 0.2);
    TechnicalIndicators::ATR calculateATR(const std::string& symbol, int period = 14);
    TechnicalIndicators::ADX calculateADX(const std::string& symbol, int period = 14);

    // WebSocket batching methods
    void addToBatch(const std::string& type, const nlohmann::json& data, MessagePriority priority = MessagePriority::MEDIUM);
    void processBatchWithPriority();
    void handleWebSocketError(WebSocketConnection hdl, const std::string& error);
    void recoverWebSocketConnection(WebSocketConnection hdl);
    void sendHeartbeat(WebSocketConnection hdl);
    void checkConnectionHealth();

    // New WebSocket message handlers
    void sendBollingerBands(const httplib::Request& req, httplib::Response& res);
    void sendIchimokuCloud(const httplib::Request& req, httplib::Response& res);
    void sendMomentumIndicators(const httplib::Request& req, httplib::Response& res);
    void sendFibonacciRetracements(const httplib::Request& req, httplib::Response& res);
    void sendParabolicSAR(const httplib::Request& req, httplib::Response& res);
    void sendATR(const httplib::Request& req, httplib::Response& res);
    void sendADX(const httplib::Request& req, httplib::Response& res);

    // WebSocket broadcast methods for new indicators
    void broadcastBollingerBandsUpdate(const std::string& symbol);
    void broadcastIchimokuCloudUpdate(const std::string& symbol);
    void broadcastMomentumIndicatorsUpdate(const std::string& symbol);
    void broadcastFibonacciRetracementsUpdate(const std::string& symbol);
    void broadcastParabolicSARUpdate(const std::string& symbol);
    void broadcastATRUpdate(const std::string& symbol);
    void broadcastADXUpdate(const std::string& symbol);

    // Batch processing members
    std::vector<BatchedMessage> messageBatch_;
    std::mutex batchMutex_;
    std::chrono::milliseconds batchInterval_;
    std::atomic<bool> batchProcessing_;
    std::thread batchProcessorThread_;

    // Connection state management
    std::unordered_map<WebSocketConnection, ConnectionState> connectionStates_;
    std::mutex connectionStateMutex_;
    std::thread healthCheckThread_;
    std::atomic<bool> healthCheckRunning_;
};

} // namespace visualization
} // namespace tradingbot 