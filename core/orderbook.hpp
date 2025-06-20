// orderbook.hpp
// AVX-512 optimized order book implementation
// Inspired by Jump Trading's tiered book design

#pragma once
#include <immintrin.h>
#include <array>
#include <atomic>

constexpr size_t TIER_SIZE = 16; // Process 16 price levels per AVX-512 op
constexpr uint8_t BATCH_SIZE = 8;

// Cache-aligned price tier structure (64-byte for optimal L1 cache utilization)
struct alignas(64) PriceTier {
    __m512i prices = _mm512_setzero_si512();    // Price levels in 0.01 cent increments (int32)
    __m512i volumes = _mm512_setzero_si512();   // Corresponding volumes (uint32)
    std::atomic<uint64_t> update_seq{0};        // Atomic sequence number for lock-free reads
    uint16_t active_mask;
};

/**
 * Order book
 * Features:
 * - AVX-512 vectorized updates
 * - Cache-conscious design
 * - Lock-free synchronization
 */
class OrderBook {
public:
    /**
     * Update order book with deltas
     * @param price_deltas Must be 64-byte aligned
     * @param volume_deltas Must be 64-byte aligned
     * @return New sequence number (for consistency checks)
     */
    uint64_t update(const int32_t* price_deltas, const uint32_t* volume_deltas);

    /**
     * Get thread-safe snapshot of current state
     * @param out_prices Must have >=TIER_SIZE elements
     * @param out_volumes Must have >=TIER_SIZE elements
     * @return Snapshot sequence number
     */
    uint64_t get_snapshot(int32_t* out_prices, uint32_t* out_volumes) const;

private:
    std::array<PriceTier, 8> m_tiers; // 8 tiers × 16 levels/tier = 128 levels depth
};