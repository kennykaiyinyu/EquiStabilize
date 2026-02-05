#include "OrderBook.h"
#include <algorithm>
#include<iostream>

void OrderBook::addOrder(const Order & order) {
    if (order.side == Side::Buy) {
        bids[order.price].push_back(order);
    } else {
        asks[order.price].push_back(order);
    }
    match();
}

void OrderBook::match() {
    // While there's a buyer willing to pay at least what a seller wants
    while (!bids.empty() && !asks.empty() && bids.begin()->first >= asks.begin()->first) {

        auto& best_bid_list = bids.begin()->second;
        auto& best_ask_list = asks.begin()->second;

        Order& buy_order = best_bid_list.front();
        Order& sell_order = best_ask_list.front();

        uint32_t matched_qty = std::min(buy_order.quantity, sell_order.quantity);

        // Execution price logic: usually the price of the order already on the book
        std::cout << "EXECUTION: " << matched_qty << " units @ " << asks.begin()->first << std::endl;

        buy_order.quantity -= matched_qty;
        sell_order.quantity -= matched_qty;

        if (buy_order.quantity == 0) best_bid_list.erase(best_bid_list.begin());
        if (sell_order.quantity == 0) best_ask_list.erase(best_ask_list.begin());

        if (best_bid_list.empty()) bids.erase(bids.begin());
        if (best_ask_list.empty()) asks.erase(asks.begin());
    }
}