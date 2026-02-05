#include "../../inc/MarketMaker.h"
#include <cmath>
#include <iostream>

MarketMaker::MarketMaker(double riskAversion, double volatility, double timeHorizon)
    : gamma(riskAversion), sigma(volatility), T(timeHorizon), inventory(0), currentReservationPrice(0) {}

void MarketMaker::onTradeExecuted(uint32_t quantity, Side side) {
    if (side == Side::Buy) {
        inventory += quantity;
    } else {
        inventory -= quantity;
    }
}

double MarketMaker::calculateReservationPrice(double midPrice) {
    // Avellaneda-Stoikov Formula: r = s - q * gamma * sigma^2 * (T - t)
    // For simplicity, we assume (T-t) is constant or slowly decaying. Here we use T as a factor.
    double riskAdjustment = inventory * gamma * std::pow(sigma, 2) * T;
    return midPrice - riskAdjustment;
}

std::vector<std::shared_ptr<Order>> MarketMaker::getQuotes(const OrderBook& book) {
    std::vector<std::shared_ptr<Order>> quotes;

    double midPrice = book.getMidPrice();
    if (midPrice == 0) return quotes; // Market is empty, can't quote

    // 1. Calculate Reservation Price
    currentReservationPrice = calculateReservationPrice(midPrice);

    // 2. Calculate Optimal Spread (Simplified)
    // In full A-S model, spread is also dynamic. Here we use a fixed spread + volatility buffer.
    double halfSpread = (2.0 / PRICE_SCALE) + (gamma * std::pow(sigma, 2) * T);
    // Ensure minimum spread of 2 ticks
    if (halfSpread < 0.0005) halfSpread = 0.0005;

    double bidPriceDouble = currentReservationPrice - halfSpread;
    double askPriceDouble = currentReservationPrice + halfSpread;

    // 3. Generate Orders
    static uint64_t orderIdCounter = 5000; // Start IDs high to avoid conflict

    // Bid
    auto bid = std::make_shared<Order>();
    bid->id = orderIdCounter++;
    bid->price = Order::fromDouble(bidPriceDouble);
    bid->quantity = 100; // Standard lot size
    bid->side = Side::Buy;
    bid->type = OrderType::Limit;
    bid->timestamp = 0; // Placeholder
    quotes.push_back(bid);

    // Ask
    auto ask = std::make_shared<Order>();
    ask->id = orderIdCounter++;
    ask->price = Order::fromDouble(askPriceDouble);
    ask->quantity = 100;
    ask->side = Side::Sell;
    ask->type = OrderType::Limit;
    ask->timestamp = 0;
    quotes.push_back(ask);

    return quotes;
}
