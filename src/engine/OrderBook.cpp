#include "../../include/OrderBook.h"
#include <iostream>
#include <iomanip>

OrderBook::OrderBook() {}

void OrderBook::addOrder(std::shared_ptr<Order> order) {
    if (orderLookup.find(order->id) != orderLookup.end()) {
        std::cerr << "Order ID " << order->id << " already exists." << std::endl;
        return;
    }

    // 1. Try to match immediately
    bool fullyFilled = matchOrder(order);

    // 2. If not fully filled, add the remainder to the book
    if (!fullyFilled) {
        orderLookup[order->id] = order;

        if (order->side == Side::Buy) {
            if (bids.find(order->price) == bids.end()) {
                bids[order->price] = std::make_shared<LimitLevel>(order->price);
            }
            bids[order->price]->orders.push_back(order);
            bids[order->price]->totalVolume += order->quantity;
        } else {
            if (asks.find(order->price) == asks.end()) {
                asks[order->price] = std::make_shared<LimitLevel>(order->price);
            }
            asks[order->price]->orders.push_back(order);
            asks[order->price]->totalVolume += order->quantity;
        }
    }
}

bool OrderBook::matchOrder(std::shared_ptr<Order> incomingOrder) {
    while (incomingOrder->quantity > 0) {
        // If Buy, look at Asks (lowest first). If Sell, look at Bids (highest first).
        if (incomingOrder->side == Side::Buy) {
            if (asks.empty()) return false;

            auto bestAskIt = asks.begin();
            uint64_t bestAskPrice = bestAskIt->first;

            // If Bid Price < Best Ask Price, no match possible
            if (incomingOrder->price < bestAskPrice) return false;

            // Match found!
            auto level = bestAskIt->second;
            auto bookOrder = level->orders.front();

            uint32_t tradeQty = std::min(incomingOrder->quantity, bookOrder->quantity);

            // Execute Trade (In a real system, we'd log this or send a callback)
            std::cout << ">>> TRADE EXECUTED: " << tradeQty << " @ " << Order::toDouble(bestAskPrice) << "\n";

            // Update quantities
            incomingOrder->quantity -= tradeQty;
            bookOrder->quantity -= tradeQty;
            level->totalVolume -= tradeQty;

            // If book order is filled, remove it
            if (bookOrder->quantity == 0) {
                level->orders.pop_front();
                orderLookup.erase(bookOrder->id);
                if (level->orders.empty()) {
                    asks.erase(bestAskIt);
                }
            }
        }
        else { // Incoming Sell Order
            if (bids.empty()) return false;

            auto bestBidIt = bids.begin();
            uint64_t bestBidPrice = bestBidIt->first;

            // If Sell Price > Best Bid Price, no match possible
            if (incomingOrder->price > bestBidPrice) return false;

            // Match found!
            auto level = bestBidIt->second;
            auto bookOrder = level->orders.front();

            uint32_t tradeQty = std::min(incomingOrder->quantity, bookOrder->quantity);

            std::cout << ">>> TRADE EXECUTED: " << tradeQty << " @ " << Order::toDouble(bestBidPrice) << "\n";

            incomingOrder->quantity -= tradeQty;
            bookOrder->quantity -= tradeQty;
            level->totalVolume -= tradeQty;

            if (bookOrder->quantity == 0) {
                level->orders.pop_front();
                orderLookup.erase(bookOrder->id);
                if (level->orders.empty()) {
                    bids.erase(bestBidIt);
                }
            }
        }
    }
    return true; // Fully filled
}

void OrderBook::cancelOrder(uint64_t orderId) {
    auto it = orderLookup.find(orderId);
    if (it == orderLookup.end()) {
        std::cerr << "Order ID " << orderId << " not found." << std::endl;
        return;
    }

    std::shared_ptr<Order> order = it->second;

    if (order->side == Side::Buy) {
        auto levelIt = bids.find(order->price);
        if (levelIt != bids.end()) {
            auto& level = levelIt->second;
            level->orders.remove(order);
            level->totalVolume -= order->quantity;
            if (level->orders.empty()) {
                bids.erase(levelIt);
            }
        }
    } else {
        auto levelIt = asks.find(order->price);
        if (levelIt != asks.end()) {
            auto& level = levelIt->second;
            level->orders.remove(order);
            level->totalVolume -= order->quantity;
            if (level->orders.empty()) {
                asks.erase(levelIt);
            }
        }
    }

    orderLookup.erase(it);
}

void OrderBook::printBook() const {
    std::cout << "\n--- Order Book Snapshot ---\n";
    std::cout << "ASKS:\n";
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        std::cout << "  Price: " << std::setw(10) << Order::toDouble(it->first)
                  << " | Vol: " << it->second->totalVolume << "\n";
    }

    std::cout << "---------------------------\n";

    std::cout << "BIDS:\n";
    for (const auto& [price, level] : bids) {
        std::cout << "  Price: " << std::setw(10) << Order::toDouble(price)
                  << " | Vol: " << level->totalVolume << "\n";
    }
    std::cout << "---------------------------\n";
}

double OrderBook::getMidPrice() const {
    if (bids.empty() || asks.empty()) return 0.0;
    return Order::toDouble((bids.begin()->first + asks.begin()->first) / 2);
}

double OrderBook::getSpread() const {
    if (bids.empty() || asks.empty()) return 0.0;
    return Order::toDouble(asks.begin()->first - bids.begin()->first);
}

double OrderBook::getImbalance() const {
    uint64_t totalBidVol = 0;
    uint64_t totalAskVol = 0;

    for (const auto& [price, level] : bids) totalBidVol += level->totalVolume;
    for (const auto& [price, level] : asks) totalAskVol += level->totalVolume;

    if (totalBidVol + totalAskVol == 0) return 0.0;

    return (double)(totalBidVol - totalAskVol) / (double)(totalBidVol + totalAskVol);
}
