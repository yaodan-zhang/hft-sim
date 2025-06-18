// orderbook.hpp
// AVX-512 optimized order book implementation
// Inspired by Jump Trading's tiered book design

#pragma once
#include <immintrin.h>
#include <array>

constexpr size_t TIER_SIZE = 16; // Process 16 price levels per AVX-512 op

// Cache-aligned price tier structure (64-byte for optimal L1 cache utilization)
struct alignas(64) PriceTier {
    __m512i prices;      // Price levels in 0.01 cent increments (int32)
    __m512i volumes;     // Corresponding volumes (uint32)
    uint32_t update_seq; // Atomic sequence number for lock-free reads
    uint16_t active_mask;// Bitmask for active levels (Jump's tier compression)
};

/**
 * High-frequency trading order book
 * Features:
 * - AVX-512 vectorized updates
 * - Cache-conscious design
 * - Lock-free synchronization
 */
class OrderBook {
public:
    // Update order book with price/volume deltas
    void update(const int32_t* price_deltas, const uint32_t* volume_deltas);
private:
    std::array<PriceTier, 8> m_tiers; // 8 tiers × 16 levels = 128 levels depth
};