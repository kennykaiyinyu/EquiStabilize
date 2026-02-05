#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "Order.h"
#include <map>
#include <unordered_map>
#include <list>
#include <memory>
#include <vector>

// A LimitLevel contains all orders at a specific price
struct LimitLevel {
    uint64_t price;
    uint64_t totalVolume;
    std::list<std::shared_ptr<Order>> orders;

    LimitLevel(uint64_t p) : price(p), totalVolume(0) {}
};

class OrderBook {
public:
    OrderBook();

    // Core actions
    void addOrder(std::shared_ptr<Order> order);
    void cancelOrder(uint64_t orderId);

    // View state
    void printBook() const;

    // Market Data / Telemetry
    double getMidPrice() const;
    double getSpread() const;

    // Imbalance: (BidVol - AskVol) / (BidVol + AskVol)
    double getImbalance() const;

private:
    // Bids: High to Low (std::greater)
    std::map<uint64_t, std::shared_ptr<LimitLevel>, std::greater<uint64_t>> bids;

    // Asks: Low to High (std::less - default)
    std::map<uint64_t, std::shared_ptr<LimitLevel>> asks;

    // Fast lookup for order management
    std::unordered_map<uint64_t, std::shared_ptr<Order>> orderLookup;

    // Internal matching logic
    // Returns true if the incoming order was fully filled
    bool matchOrder(std::shared_ptr<Order> order);
};

#endif // ORDERBOOK_H