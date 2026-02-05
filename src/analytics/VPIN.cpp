#include "../../inc/Telemetry.h"
#include <cmath>
#include <numeric>
#include <iostream>

Telemetry::Telemetry(VPINConfig cfg)
    : config(cfg), currentBucketVolume(0), currentBuyVolume(0), currentSellVolume(0), totalWindowVolume(0) {}

void Telemetry::onTrade(uint64_t price, uint32_t quantity, Side side) {
    // Distribute trade volume into buckets
    // A single large trade might fill multiple buckets
    uint32_t remainingQty = quantity;

    while (remainingQty > 0) {
        uint64_t spaceInBucket = config.bucketVolumeSize - currentBucketVolume;
        uint64_t fillQty = std::min((uint64_t)remainingQty, spaceInBucket);

        if (side == Side::Buy) {
            currentBuyVolume += fillQty;
        } else {
            currentSellVolume += fillQty;
        }

        currentBucketVolume += fillQty;
        remainingQty -= fillQty;

        // If bucket is full, close it and start a new one
        if (currentBucketVolume >= config.bucketVolumeSize) {
            closeBucket();
        }
    }
}

void Telemetry::closeBucket() {
    // Calculate Order Imbalance for this bucket: |Buy - Sell|
    // Note: We use double for calculation precision
    double imbalance = std::abs((double)currentBuyVolume - (double)currentSellVolume);

    // Add to history
    bucketImbalances.push_back(imbalance);

    // Maintain sliding window size
    if (bucketImbalances.size() > config.windowSize) {
        bucketImbalances.pop_front();
    }

    // Reset current bucket
    currentBucketVolume = 0;
    currentBuyVolume = 0;
    currentSellVolume = 0;
}

double Telemetry::getVPIN() const {
    if (bucketImbalances.empty()) return 0.0;

    // Sum of imbalances in the window
    double sumImbalance = 0;
    for (double val : bucketImbalances) {
        sumImbalance += val;
    }

    // Total Volume in the window = Number of Buckets * Bucket Size
    // (Approximation is fine for this implementation, or we could track exact volume)
    double totalVolume = bucketImbalances.size() * config.bucketVolumeSize;

    if (totalVolume == 0) return 0.0;

    return sumImbalance / totalVolume;
}
