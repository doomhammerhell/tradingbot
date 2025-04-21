#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <mutex>
#include <atomic>
#include <regex>
#include <nlohmann/json.hpp>
#include "core/Logger.hpp"

namespace tradingbot {
namespace core {

class SecurityManager {
public:
    static SecurityManager& getInstance();

    // Initialize security manager
    void initialize(const std::string& auditLogPath = "logs/audit.log");
    
    // Log security event
    void logEvent(const std::string& eventType, const std::string& details, 
                 const std::string& source = "", const std::string& severity = "info");
    
    // Check for suspicious activity
    bool checkSuspiciousActivity(const std::string& source, const std::string& action);
    
    // Get security metrics
    nlohmann::json getSecurityMetrics() const;
    
    // Get audit log entries
    std::vector<nlohmann::json> getAuditLogs(int limit = 100) const;
    
    // Clear audit logs
    void clearAuditLogs();
    
    // Set security thresholds
    void setThresholds(const nlohmann::json& thresholds);
    
    // Get current security status
    nlohmann::json getSecurityStatus() const;
    
    // IP whitelisting
    void addToWhitelist(const std::string& ip);
    void removeFromWhitelist(const std::string& ip);
    bool isWhitelisted(const std::string& ip) const;
    std::vector<std::string> getWhitelist() const;
    
    // Command validation
    void addAllowedCommand(const std::string& command);
    void removeAllowedCommand(const std::string& command);
    bool isCommandAllowed(const std::string& command) const;
    std::vector<std::string> getAllowedCommands() const;
    
    // Validate configuration
    bool validateConfig(const nlohmann::json& config) const;
    
    // Get security policies
    nlohmann::json getSecurityPolicies() const;
    
    // Set security policies
    void setSecurityPolicies(const nlohmann::json& policies);

private:
    SecurityManager() = default;
    ~SecurityManager() = default;
    
    // Prevent copying
    SecurityManager(const SecurityManager&) = delete;
    SecurityManager& operator=(const SecurityManager&) = delete;
    
    // Analyze event patterns
    void analyzeEventPatterns(const nlohmann::json& event);
    
    // Check rate limits
    bool checkRateLimit(const std::string& source, const std::string& action);
    
    // Update activity counters
    void updateActivityCounters(const std::string& source, const std::string& action);
    
    // Save audit log
    void saveAuditLog(const nlohmann::json& event);
    
    // Load audit log
    std::vector<nlohmann::json> loadAuditLog() const;
    
    // Validate IP address
    bool isValidIP(const std::string& ip) const;
    
    // Validate command format
    bool isValidCommandFormat(const std::string& command) const;

    struct ActivityCounter {
        std::atomic<int> count{0};
        std::chrono::system_clock::time_point lastReset;
    };

    struct SecurityThresholds {
        int maxRequestsPerMinute{100};
        int maxFailedLogins{5};
        int maxInvalidCommands{10};
        int maxConcurrentConnections{50};
        int maxCommandLength{256};
        int minPasswordLength{8};
        int maxLoginAttempts{3};
        int sessionTimeoutMinutes{30};
    };

    struct SecurityPolicies {
        bool requireStrongPasswords{true};
        bool enforceIPWhitelist{false};
        bool requireCommandValidation{true};
        bool enableRateLimiting{true};
        bool logAllCommands{true};
        bool requireTwoFactorAuth{false};
        std::vector<std::string> allowedIPRanges;
        std::vector<std::string> restrictedCommands;
    };

    std::string auditLogPath_;
    SecurityThresholds thresholds_;
    SecurityPolicies policies_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ActivityCounter> activityCounters_;
    std::unordered_set<std::string> whitelistedIPs_;
    std::unordered_set<std::string> allowedCommands_;
    std::atomic<bool> isInitialized_{false};
    std::atomic<int> totalEvents_{0};
    std::atomic<int> suspiciousEvents_{0};
};

} // namespace core
} // namespace tradingbot 