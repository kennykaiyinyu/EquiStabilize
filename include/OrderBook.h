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
    // Optimization: Use raw pointers. The OrderBook owns the memory via orderLookup.
    std::list<Order*> orders;

    LimitLevel(uint64_t p) : price(p), totalVolume(0) {}
};

class OrderBook {
public:
    OrderBook();

    // Core actions
    // Optimization: Pass by unique_ptr to transfer ownership (Move semantics)
    void addOrder(std::unique_ptr<Order> order);
    void cancelOrder(uint64_t orderId);

    // View state
    void printBook() const;

    // Market Data / Telemetry
    double getMidPrice() const;
    double getSpread() const;
    double getImbalance() const;

private:
    // Bids: High to Low (std::greater)
    std::map<uint64_t, std::shared_ptr<LimitLevel>, std::greater<uint64_t>> bids;

    // Asks: Low to High (std::less - default)
    std::map<uint64_t, std::shared_ptr<LimitLevel>> asks;

    // Fast lookup for order management
    // Optimization: This map OWNS the Order objects.
    std::unordered_map<uint64_t, std::unique_ptr<Order>> orderLookup;

    // Internal matching logic
    // Returns true if the incoming order was fully filled
    // Takes raw pointer because ownership is temporarily held by caller or lookup
    bool matchOrder(Order* order);
};

#endif // ORDERBOOK_H