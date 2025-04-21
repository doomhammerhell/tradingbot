#include "../../../include/technical/indicators/MACD.hpp"

namespace tradingbot {
namespace technical {
namespace indicators {

MACD::MACD(int fastPeriod, int slowPeriod, int signalPeriod)
    : fastPeriod_(fastPeriod)
    , slowPeriod_(slowPeriod)
    , signalPeriod_(signalPeriod)
    , fastEMA_(std::make_unique<EMA>(fastPeriod))
    , slowEMA_(std::make_unique<EMA>(slowPeriod))
    , signalEMA_(std::make_unique<EMA>(signalPeriod)) {}

void MACD::initialize(std::shared_ptr<core::IDataFeed> dataFeed) {
    dataFeed_ = dataFeed;
    fastEMA_->initialize(dataFeed);
    slowEMA_->initialize(dataFeed);
}

void MACD::calculate() {
    if (!dataFeed_) {
        return;
    }

    // Calculate EMAs
    fastEMA_->calculate();
    slowEMA_->calculate();

    // Calculate MACD line
    calculateMACDLine();

    // Calculate signal line
    calculateSignalLine();

    // Calculate histogram
    calculateHistogram();
}

std::vector<double> MACD::getValues() const {
    return macdLine_;
}

std::string MACD::getName() const {
    return "MACD_" + std::to_string(fastPeriod_) + "_" + 
           std::to_string(slowPeriod_) + "_" + 
           std::to_string(signalPeriod_);
}

std::vector<double> MACD::getMACDLine() const {
    return macdLine_;
}

std::vector<double> MACD::getSignalLine() const {
    return signalLine_;
}

std::vector<double> MACD::getHistogram() const {
    return histogram_;
}

double MACD::getCurrentValue() const {
    return macdLine_.empty() ? 0.0 : macdLine_.back();
}

double MACD::getCurrentSignal() const {
    return signalLine_.empty() ? 0.0 : signalLine_.back();
}

double MACD::getCurrentHistogram() const {
    return histogram_.empty() ? 0.0 : histogram_.back();
}

void MACD::calculateMACDLine() {
    const auto& fastValues = fastEMA_->getValues();
    const auto& slowValues = slowEMA_->getValues();

    if (fastValues.size() != slowValues.size()) {
        return;
    }

    macdLine_.clear();
    for (size_t i = 0; i < fastValues.size(); ++i) {
        macdLine_.push_back(fastValues[i] - slowValues[i]);
    }
}

void MACD::calculateSignalLine() {
    if (macdLine_.empty()) {
        return;
    }

    // Create a temporary data feed for the signal line calculation
    class MACDDataFeed : public core::IDataFeed {
    public:
        explicit MACDDataFeed(const std::vector<double>& values) : values_(values) {}
        void initialize(const std::string&, const std::string&) override {}
        void startStreaming() override {}
        void stopStreaming() override {}
        core::MarketData getHistoricalData(int) const override {
            core::MarketData data;
            data.prices = values_;
            return data;
        }
        void subscribe(std::function<void(const core::MarketData&)>) override {}
        std::string getName() const override { return "MACDDataFeed"; }
    private:
        std::vector<double> values_;
    };

    auto macdDataFeed = std::make_shared<MACDDataFeed>(macdLine_);
    signalEMA_->initialize(macdDataFeed);
    signalEMA_->calculate();
    signalLine_ = signalEMA_->getValues();
}

void MACD::calculateHistogram() {
    if (macdLine_.size() != signalLine_.size()) {
        return;
    }

    histogram_.clear();
    for (size_t i = 0; i < macdLine_.size(); ++i) {
        histogram_.push_back(macdLine_[i] - signalLine_[i]);
    }
}

} // namespace indicators
} // namespace technical
} // namespace tradingbot 