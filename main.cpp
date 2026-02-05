#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <random>
#include "OrderBook.h"
#include "Order.h"
#include "MarketMaker.h"
#include "Telemetry.h"

int main() {
    std::cout << "EquiStabilize: Full System Simulation (Exporting to CSV)\n";

    // 1. Setup Components
    OrderBook book;
    MarketMaker mm(0.1, 0.5, 1.0); // Risk Aversion = 0.1
    VPINConfig vpinConfig{1000, 5}; // Bucket = 1000 shares
    Telemetry telemetry(vpinConfig);

    // 2. Setup CSV Output
    std::ofstream csvFile("simulation_data.csv");
    csvFile << "Step,MidPrice,MM_Inventory,MM_ReservationPrice,VPIN\n";

    // 3. Simulation Setup
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> qtyDist(50, 150);

    // Initial Price
    double currentMarketPrice = 100.00;

    // Run 100 Steps
    for (int step = 0; step < 100; ++step) {
        // --- A. Market Maker Actions ---
        // 1. MM places quotes based on current state
        // (For simplicity in this data-gen, we assume MM quotes track the market price + inventory skew)
        // We update the book just to have a valid MidPrice
        book = OrderBook(); // Reset book for this step (simple snapshot model)

        // MM calculates reservation price
        // We need a fake midprice to feed the MM if the book is empty,
        // but in this loop we control the "External Price"

        // Let's simulate the MM's internal logic:
        // If Inventory is High, Reservation Price drops.
        // We'll just log the MM's state.

        // --- B. Market Environment (The Scenario) ---
        Side tradeSide;
        if (step < 50) {
            // Normal Market: Random Walk
            tradeSide = (step % 2 == 0) ? Side::Buy : Side::Sell;
            if (tradeSide == Side::Buy) currentMarketPrice += 0.05;
            else currentMarketPrice -= 0.05;
        } else {
            // CRASH SCENARIO: Heavy Selling
            // 80% chance of Sell
            tradeSide = (gen() % 10 < 8) ? Side::Sell : Side::Buy;

            if (tradeSide == Side::Sell) currentMarketPrice -= 0.20; // Fast drop
            else currentMarketPrice += 0.05;
        }

        // --- C. Execution & Telemetry ---
        uint32_t tradeQty = qtyDist(gen);

        // 1. Update VPIN
        telemetry.onTrade(Order::fromDouble(currentMarketPrice), tradeQty, tradeSide);

        // 2. Update MM Inventory (Simulate MM participating in 50% of trades)
        if (step % 2 == 0) {
            // If Market Sells, MM Buys (Inventory goes UP)
            // If Market Buys, MM Sells (Inventory goes DOWN)
            if (tradeSide == Side::Sell) mm.onTradeExecuted(tradeQty, Side::Buy);
            else mm.onTradeExecuted(tradeQty, Side::Sell);
        }

        // --- D. Log Data ---
        // We need to feed a dummy book to MM to get the Reservation Price
        OrderBook dummyBook;
        dummyBook.addOrder(std::make_shared<Order>(Order{1, Order::fromDouble(currentMarketPrice - 0.05), 100, Side::Buy, OrderType::Limit, 0}));
        dummyBook.addOrder(std::make_shared<Order>(Order{2, Order::fromDouble(currentMarketPrice + 0.05), 100, Side::Sell, OrderType::Limit, 0}));
        auto quotes = mm.getQuotes(dummyBook); // Just to update internal state if needed

        csvFile << step << ","
                << currentMarketPrice << ","
                << mm.getInventory() << ","
                << mm.getReservationPrice() << ","
                << telemetry.getVPIN() << "\n";
    }

    csvFile.close();
    std::cout << "Simulation complete. Data written to 'simulation_data.csv'.\n";
    std::cout << "Run the Python script to visualize the results.\n";

    return 0;
}
