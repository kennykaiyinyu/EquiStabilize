#include <iostream>
#include <memory>
#include "OrderBook.h"
#include "Order.h"

int main() {
    std::cout << "EquiStabilize: High-Performance Market Stability Engine\n";
    std::cout << "Initializing Order Book...\n";

    OrderBook book;

    // Create some dummy orders
    // Buy Orders (Bids)
    auto bid1 = std::make_shared<Order>(Order{1, Order::fromDouble(100.50), 100, Side::Buy, OrderType::Limit, 1000});
    auto bid2 = std::make_shared<Order>(Order{2, Order::fromDouble(100.45), 50, Side::Buy, OrderType::Limit, 1001});
    auto bid3 = std::make_shared<Order>(Order{3, Order::fromDouble(100.50), 25, Side::Buy, OrderType::Limit, 1002}); // Same price as bid1

    // Sell Orders (Asks)
    auto ask1 = std::make_shared<Order>(Order{4, Order::fromDouble(100.55), 100, Side::Sell, OrderType::Limit, 1003});
    auto ask2 = std::make_shared<Order>(Order{5, Order::fromDouble(100.60), 200, Side::Sell, OrderType::Limit, 1004});

    std::cout << "Adding orders to book...\n";
    book.addOrder(bid1);
    book.addOrder(bid2);
    book.addOrder(bid3);
    book.addOrder(ask1);
    book.addOrder(ask2);

    book.printBook();

    std::cout << "Current Spread: " << book.getSpread() << "\n";
    std::cout << "Mid Price: " << book.getMidPrice() << "\n";

    std::cout << "\nCancelling Order ID 1 (Bid @ 100.50)...\n";
    book.cancelOrder(1);

    book.printBook();

    return 0;
}
