#pragma once
#include "market_data.h"
#include <array>
#include <immintrin.h>

constexpr size_t TIER_SIZE = 16;  // AVX-512: 16 x 32 bit
constexpr size_t MAX_TIERS = 8;

struct alignas(64) PriceTier {
    __m512i prices;              // int32
    __m512i volumes;             // uint32
    std::atomic<uint64_t> seq;   // sequence number
    __mmask16 active_mask;
};

class OrderBook {
public:
    OrderBook();
    
    // Update and return new sequence number
    uint64_t update(const __m512i& prices, const __m512i& volumes, __mmask16 ask_mask);
    
    // Get snapshot of orderbook and return sequence number
    uint64_t get_snapshot(__m512i& out_prices, __m512i& out_volumes) const;
    
    // Get best bid and ask
    void get_top_of_book(int32_t& best_bid, int32_t& best_ask) const;

private:
    std::array<PriceTier, MAX_TIERS> _tiers;
    __m512i _min_valid_price = _mm512_set1_epi32(0);
    __m512i _max_valid_price = _mm512_set1_epi32(2'000'000);  // E.g., $20,000.00
};