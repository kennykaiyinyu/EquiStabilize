#ifndef MARKETMAKER_H
#define MARKETMAKER_H

#include "OrderBook.h"
#include "Order.h"
#include <vector>
#include <memory>

class MarketMaker {
public:
    // Parameters for Avellaneda-Stoikov
    // gamma: Risk aversion (higher = more afraid of holding inventory)
    // sigma: Volatility of the asset
    // timeHorizon: Time remaining in the trading session (T-t)
    MarketMaker(double riskAversion, double volatility, double timeHorizon);

    // The core logic: Look at the book, check inventory, and decide on quotes
    // Returns a vector of Orders to be placed (cancels previous orders internally if this were a real gateway)
    std::vector<std::shared_ptr<Order>> getQuotes(const OrderBook& book);

    // Update internal inventory state (called when a trade happens)
    void onTradeExecuted(uint32_t quantity, Side side);

    // Getters for NIW Telemetry
    double getInventory() const { return inventory; }
    double getReservationPrice() const { return currentReservationPrice; }

private:
    double gamma; // Risk Aversion
    double sigma; // Volatility
    double T;     // Time Horizon

    double inventory; // q (Positive = Long, Negative = Short)
    double currentReservationPrice; // r

    // Helper to calculate Reservation Price: r = s - q * gamma * sigma^2 * T
    double calculateReservationPrice(double midPrice);
};

#endif // MARKETMAKER_H