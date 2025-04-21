#pragma once

#include <vector>
#include <memory>
#include "../core/IDataFeed.hpp"

namespace tradingbot {
namespace technical {

class TechnicalIndicators {
public:
    virtual ~TechnicalIndicators() = default;

    // Initialize with data feed
    virtual void initialize(std::shared_ptr<core::IDataFeed> dataFeed) = 0;

    // Calculate indicators
    virtual void calculate() = 0;

    // Get indicator values
    virtual std::vector<double> getValues() const = 0;

    // Get indicator name
    virtual std::string getName() const = 0;

protected:
    std::shared_ptr<core::IDataFeed> dataFeed_;
    std::vector<double> values_;
};

} // namespace technical
} // namespace tradingbot 