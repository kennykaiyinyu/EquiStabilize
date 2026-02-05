#include <iostream>
#include <memory>
#include "OrderBook.h"
#include "Order.h"

int main() {
    std::cout << "EquiStabilize: High-Performance Market Stability Engine\n";
    std::cout << "Initializing Order Book...\n";

    OrderBook book;

    // 1. Add some resting orders (Liquidity)
    std::cout << "\n[Step 1] Adding Resting Liquidity...\n";
    book.addOrder(std::make_shared<Order>(Order{1, Order::fromDouble(100.00), 100, Side::Buy, OrderType::Limit, 1000}));
    book.addOrder(std::make_shared<Order>(Order{2, Order::fromDouble(99.90), 50, Side::Buy, OrderType::Limit, 1001}));

    book.addOrder(std::make_shared<Order>(Order{3, Order::fromDouble(100.10), 100, Side::Sell, OrderType::Limit, 1002}));
    book.addOrder(std::make_shared<Order>(Order{4, Order::fromDouble(100.20), 200, Side::Sell, OrderType::Limit, 1003}));

    book.printBook();
    std::cout << "Imbalance: " << book.getImbalance() << "\n";

    // 2. Execute a Trade (Aggressive Buy)
    std::cout << "\n[Step 2] Incoming Aggressive Buy Order (150 @ 100.15)...\n";
    // This should eat the 100 @ 100.10 and leave 50 remaining
    auto aggressiveBuy = std::make_shared<Order>(Order{5, Order::fromDouble(100.15), 150, Side::Buy, OrderType::Limit, 1004});
    book.addOrder(aggressiveBuy);

    book.printBook();

    return 0;
}
