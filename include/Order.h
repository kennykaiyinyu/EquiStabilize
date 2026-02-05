#ifndef ORDER_H
#define ORDER_H

#include <cstdint>
#include <iostream>

// Fixed-point multiplier (e.g., 10000 means 4 decimal places, so 10.50 is 105000)
constexpr uint64_t PRICE_SCALE = 10000;

enum class Side : uint8_t {
    Buy,
    Sell
};

enum class OrderType : uint8_t {
    Limit,
    Market,
    Cancel
};

struct Order {
    uint64_t id;
    uint64_t price;      // Fixed-point representation
    uint32_t quantity;
    Side side;
    OrderType type;
    uint64_t timestamp;  // Nanoseconds since epoch

    // Helper to convert double to fixed-point (for input convenience)
    static uint64_t fromDouble(double p) {
        return static_cast<uint64_t>(p * PRICE_SCALE + 0.5);
    }

    // Helper to convert fixed-point to double (for display)
    static double toDouble(uint64_t p) {
        return static_cast<double>(p) / PRICE_SCALE;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << "Order[ID=" << order.id
       << ", Side=" << (order.side == Side::Buy ? "Buy" : "Sell")
       << ", Type=" << (order.type == OrderType::Limit ? "Limit" : (order.type == OrderType::Market ? "Market" : "Cancel"))
       << ", Price=" << Order::toDouble(order.price)
       << ", Qty=" << order.quantity
       << ", Time=" << order.timestamp << "]";
    return os;
}

#endif // ORDER_H