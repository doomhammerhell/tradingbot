#include "strategies/RSIStrategy.hpp"
#include <algorithm>
#include <numeric>

namespace tradingbot {

RSIStrategy::RSIStrategy(const Config& config)
    : config_(config), current_rsi_(50.0), last_price_(0.0) {
    price_changes_.reserve(config_.period);
}

Signal RSIStrategy::generateSignal(const MarketData& data) {
    if (price_changes_.size() < config_.period) {
        return Signal::HOLD;
    }

    if (current_rsi_ > config_.overbought) {
        return Signal::SELL;
    }
    else if (current_rsi_ < config_.oversold) {
        return Signal::BUY;
    }

    return Signal::HOLD;
}

void RSIStrategy::updateState(const MarketData& data) {
    if (last_price_ > 0.0) {
        double price_change = data.close - last_price_;
        price_changes_.push_back(price_change);

        if (price_changes_.size() > config_.period) {
            price_changes_.erase(price_changes_.begin());
        }

        if (price_changes_.size() == config_.period) {
            current_rsi_ = calculateRSI();
        }
    }

    last_price_ = data.close;
}

void RSIStrategy::reset() {
    price_changes_.clear();
    current_rsi_ = 50.0;
    last_price_ = 0.0;
}

double RSIStrategy::calculateRSI() const {
    std::vector<double> gains, losses;
    gains.reserve(price_changes_.size());
    losses.reserve(price_changes_.size());

    for (double change : price_changes_) {
        if (change > 0) {
            gains.push_back(change);
            losses.push_back(0.0);
        }
        else {
            gains.push_back(0.0);
            losses.push_back(-change);
        }
    }

    double avg_gain = std::accumulate(gains.begin(), gains.end(), 0.0) / config_.period;
    double avg_loss = std::accumulate(losses.begin(), losses.end(), 0.0) / config_.period;

    if (avg_loss == 0.0) {
        return 100.0;
    }

    double rs = avg_gain / avg_loss;
    return 100.0 - (100.0 / (1.0 + rs));
}

} // namespace tradingbot 