#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <vector>
#include <deque>
#include <cstdint>
#include "Order.h"

// VPIN Configuration
struct VPINConfig {
    uint64_t bucketVolumeSize; // V: Volume per bucket (e.g., every 10,000 shares)
    int windowSize;            // n: Number of buckets to use for moving average (e.g., 50)
};

class Telemetry {
public:
    Telemetry(VPINConfig config);

    // Feed a trade into the analyzer
    // price: Trade Price
    // quantity: Trade Volume
    // side: Aggressor Side (Who initiated? Buy or Sell)
    void onTrade(uint64_t price, uint32_t quantity, Side side);

    // Get the current VPIN score (0.0 to 1.0)
    // High score = High Toxicity (Crash Risk)
    double getVPIN() const;

private:
    VPINConfig config;

    // Current Bucket State
    uint64_t currentBucketVolume;
    uint64_t currentBuyVolume;
    uint64_t currentSellVolume;

    // History of Buckets (for Moving Average)
    // Each entry is the "Order Imbalance" |BuyVol - SellVol| for that bucket
    std::deque<double> bucketImbalances;

    // Total volume of all buckets in the window (cached for speed)
    double totalWindowVolume;

    void closeBucket();
};

#endif // TELEMETRY_H