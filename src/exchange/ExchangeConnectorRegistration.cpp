#include "exchange/BinanceConnector.hpp"
#include "exchange/MockBinanceConnector.hpp"
#include "exchange/ExchangeConnectorFactory.hpp"

// Register the exchange connectors
REGISTER_EXCHANGE_CONNECTOR("binance", BinanceConnector)
REGISTER_EXCHANGE_CONNECTOR("mock_binance", MockBinanceConnector) 