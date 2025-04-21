#include "core/SecurityManager.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <regex>
#include <spdlog/spdlog.h>

namespace tradingbot {
namespace core {

SecurityManager& SecurityManager::getInstance() {
    static SecurityManager instance;
    return instance;
}

void SecurityManager::initialize(const std::string& auditLogPath) {
    if (isInitialized_) {
        return;
    }
    
    auditLogPath_ = auditLogPath;
    
    // Create audit log directory if it doesn't exist
    std::filesystem::path logPath(auditLogPath);
    std::filesystem::create_directories(logPath.parent_path());
    
    isInitialized_ = true;
    spdlog::info("Security manager initialized with audit log: {}", auditLogPath);
}

void SecurityManager::logEvent(const std::string& eventType, const std::string& details,
                             const std::string& source, const std::string& severity) {
    if (!isInitialized_) {
        spdlog::warn("Security manager not initialized");
        return;
    }
    
    nlohmann::json event = {
        {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
        {"event_type", eventType},
        {"details", details},
        {"source", source},
        {"severity", severity}
    };
    
    // Analyze event patterns
    analyzeEventPatterns(event);
    
    // Save to audit log
    saveAuditLog(event);
    
    // Update counters
    totalEvents_++;
    if (severity == "warning" || severity == "error") {
        suspiciousEvents_++;
    }
    
    // Log to console
    spdlog::log(spdlog::level::from_str(severity), 
                "Security event: {} - {}", eventType, details);
}

bool SecurityManager::checkSuspiciousActivity(const std::string& source, const std::string& action) {
    if (!isInitialized_) {
        return false;
    }
    
    // Check rate limits
    if (!checkRateLimit(source, action)) {
        logEvent("rate_limit_exceeded", 
                "Rate limit exceeded for " + source + " performing " + action,
                source, "warning");
        return true;
    }
    
    // Update activity counters
    updateActivityCounters(source, action);
    
    return false;
}

nlohmann::json SecurityManager::getSecurityMetrics() const {
    return {
        {"total_events", totalEvents_.load()},
        {"suspicious_events", suspiciousEvents_.load()},
        {"activity_counters", [this]() {
            nlohmann::json counters;
            for (const auto& [key, counter] : activityCounters_) {
                counters[key] = counter.count.load();
            }
            return counters;
        }()}
    };
}

std::vector<nlohmann::json> SecurityManager::getAuditLogs(int limit) const {
    auto logs = loadAuditLog();
    if (limit > 0 && logs.size() > limit) {
        logs.erase(logs.begin(), logs.end() - limit);
    }
    return logs;
}

void SecurityManager::clearAuditLogs() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(auditLogPath_, std::ios::trunc);
    file.close();
    spdlog::info("Audit logs cleared");
}

void SecurityManager::setThresholds(const nlohmann::json& thresholds) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (thresholds.contains("max_requests_per_minute")) {
        thresholds_.maxRequestsPerMinute = thresholds["max_requests_per_minute"];
    }
    if (thresholds.contains("max_failed_logins")) {
        thresholds_.maxFailedLogins = thresholds["max_failed_logins"];
    }
    if (thresholds.contains("max_invalid_commands")) {
        thresholds_.maxInvalidCommands = thresholds["max_invalid_commands"];
    }
    if (thresholds.contains("max_concurrent_connections")) {
        thresholds_.maxConcurrentConnections = thresholds["max_concurrent_connections"];
    }
}

nlohmann::json SecurityManager::getSecurityStatus() const {
    return {
        {"initialized", isInitialized_.load()},
        {"thresholds", {
            {"max_requests_per_minute", thresholds_.maxRequestsPerMinute},
            {"max_failed_logins", thresholds_.maxFailedLogins},
            {"max_invalid_commands", thresholds_.maxInvalidCommands},
            {"max_concurrent_connections", thresholds_.maxConcurrentConnections}
        }},
        {"metrics", getSecurityMetrics()}
    };
}

void SecurityManager::analyzeEventPatterns(const nlohmann::json& event) {
    // Implement pattern analysis logic here
    // For example, detect repeated failed login attempts
    // or unusual command sequences
}

bool SecurityManager::checkRateLimit(const std::string& source, const std::string& action) {
    std::string key = source + ":" + action;
    auto now = std::chrono::system_clock::now();
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto& counter = activityCounters_[key];
    
    // Reset counter if more than a minute has passed
    if (now - counter.lastReset > std::chrono::minutes(1)) {
        counter.count = 0;
        counter.lastReset = now;
    }
    
    // Check if rate limit exceeded
    return ++counter.count <= thresholds_.maxRequestsPerMinute;
}

void SecurityManager::updateActivityCounters(const std::string& source, const std::string& action) {
    std::string key = source + ":" + action;
    std::lock_guard<std::mutex> lock(mutex_);
    activityCounters_[key].count++;
}

void SecurityManager::saveAuditLog(const nlohmann::json& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream file(auditLogPath_, std::ios::app);
    if (file.is_open()) {
        file << event.dump() << std::endl;
    }
}

std::vector<nlohmann::json> SecurityManager::loadAuditLog() const {
    std::vector<nlohmann::json> logs;
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream file(auditLogPath_);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            try {
                logs.push_back(nlohmann::json::parse(line));
            } catch (const std::exception& e) {
                spdlog::error("Failed to parse audit log entry: {}", e.what());
            }
        }
    }
    
    return logs;
}

void SecurityManager::addToWhitelist(const std::string& ip) {
    if (!isValidIP(ip)) {
        throw std::invalid_argument("Invalid IP address format");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    whitelistedIPs_.insert(ip);
    logEvent("ip_whitelist_add", "Added IP to whitelist: " + ip);
}

void SecurityManager::removeFromWhitelist(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    whitelistedIPs_.erase(ip);
    logEvent("ip_whitelist_remove", "Removed IP from whitelist: " + ip);
}

bool SecurityManager::isWhitelisted(const std::string& ip) const {
    if (!policies_.enforceIPWhitelist) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    return whitelistedIPs_.count(ip) > 0;
}

std::vector<std::string> SecurityManager::getWhitelist() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::vector<std::string>(whitelistedIPs_.begin(), whitelistedIPs_.end());
}

void SecurityManager::addAllowedCommand(const std::string& command) {
    if (!isValidCommandFormat(command)) {
        throw std::invalid_argument("Invalid command format");
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    allowedCommands_.insert(command);
    logEvent("command_whitelist_add", "Added command to allowed list: " + command);
}

void SecurityManager::removeAllowedCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    allowedCommands_.erase(command);
    logEvent("command_whitelist_remove", "Removed command from allowed list: " + command);
}

bool SecurityManager::isCommandAllowed(const std::string& command) const {
    if (!policies_.requireCommandValidation) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    return allowedCommands_.count(command) > 0;
}

std::vector<std::string> SecurityManager::getAllowedCommands() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::vector<std::string>(allowedCommands_.begin(), allowedCommands_.end());
}

bool SecurityManager::validateConfig(const nlohmann::json& config) const {
    try {
        // Validate required fields
        if (!config.contains("api_key") || !config.contains("api_secret")) {
            return false;
        }
        
        // Validate password strength if required
        if (policies_.requireStrongPasswords && config.contains("password")) {
            std::string password = config["password"];
            if (password.length() < thresholds_.minPasswordLength) {
                return false;
            }
        }
        
        // Validate IP whitelist if enabled
        if (policies_.enforceIPWhitelist && config.contains("allowed_ips")) {
            for (const auto& ip : config["allowed_ips"]) {
                if (!isValidIP(ip)) {
                    return false;
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Configuration validation failed: {}", e.what());
        return false;
    }
}

nlohmann::json SecurityManager::getSecurityPolicies() const {
    return {
        {"require_strong_passwords", policies_.requireStrongPasswords},
        {"enforce_ip_whitelist", policies_.enforceIPWhitelist},
        {"require_command_validation", policies_.requireCommandValidation},
        {"enable_rate_limiting", policies_.enableRateLimiting},
        {"log_all_commands", policies_.logAllCommands},
        {"require_two_factor_auth", policies_.requireTwoFactorAuth},
        {"allowed_ip_ranges", policies_.allowedIPRanges},
        {"restricted_commands", policies_.restrictedCommands}
    };
}

void SecurityManager::setSecurityPolicies(const nlohmann::json& policies) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (policies.contains("require_strong_passwords")) {
        policies_.requireStrongPasswords = policies["require_strong_passwords"];
    }
    if (policies.contains("enforce_ip_whitelist")) {
        policies_.enforceIPWhitelist = policies["enforce_ip_whitelist"];
    }
    if (policies.contains("require_command_validation")) {
        policies_.requireCommandValidation = policies["require_command_validation"];
    }
    if (policies.contains("enable_rate_limiting")) {
        policies_.enableRateLimiting = policies["enable_rate_limiting"];
    }
    if (policies.contains("log_all_commands")) {
        policies_.logAllCommands = policies["log_all_commands"];
    }
    if (policies.contains("require_two_factor_auth")) {
        policies_.requireTwoFactorAuth = policies["require_two_factor_auth"];
    }
    if (policies.contains("allowed_ip_ranges")) {
        policies_.allowedIPRanges = policies["allowed_ip_ranges"].get<std::vector<std::string>>();
    }
    if (policies.contains("restricted_commands")) {
        policies_.restrictedCommands = policies["restricted_commands"].get<std::vector<std::string>>();
    }
    
    logEvent("security_policies_updated", "Security policies updated");
}

bool SecurityManager::isValidIP(const std::string& ip) const {
    std::regex ipv4("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    std::regex ipv6("^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$");
    
    return std::regex_match(ip, ipv4) || std::regex_match(ip, ipv6);
}

bool SecurityManager::isValidCommandFormat(const std::string& command) const {
    if (command.empty() || command.length() > thresholds_.maxCommandLength) {
        return false;
    }
    
    // Basic command format validation
    std::regex validCommand("^[a-zA-Z0-9_\\-]+(\\s+[a-zA-Z0-9_\\-]+)*$");
    return std::regex_match(command, validCommand);
}

} // namespace core
} // namespace tradingbot 