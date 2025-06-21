#include "orderbook.h"
#include <iostream>

OrderBook::OrderBook() {
    // Initialize each tier
    for (auto& tier : _tiers) {
        tier.prices = _mm512_setzero_si512();
        tier.volumes = _mm512_setzero_si512();
        tier.seq = 0;
        tier.active_mask = 0;
    }
}

uint64_t OrderBook::update(const __m512i& prices, const __m512i& volumes, __mmask16 ask_mask) {
    __mmask16 bid_mask = ~ask_mask;
    // 1. 将价格映射到 tier 索引
    __m512i tier_idx = _mm512_srli_epi32(prices, 14);

    for (size_t i = 0; i < MAX_TIERS; ++i) {
        __m512i tier_val = _mm512_set1_epi32(i);
        __mmask16 match_mask = _mm512_cmpeq_epi32_mask(tier_idx, tier_val);
        __mmask16 final_bid_mask = match_mask & bid_mask;
        __mmask16 final_ask_mask = match_mask & ask_mask;

        if (final_bid_mask | final_ask_mask) {
            auto& tier = _tiers[i];
            tier.prices = _mm512_mask_add_epi32(tier.prices, match_mask, tier.prices, prices);
            tier.volumes = _mm512_mask_add_epi32(tier.volumes, match_mask, tier.volumes, volumes);
            tier.active_mask |= final_bid_mask | final_ask_mask;
            tier.seq.fetch_add(1, std::memory_order_release);
        }
    }

    return _tiers[0].seq.load(std::memory_order_relaxed);
}


uint64_t OrderBook::get_snapshot(__m512i& out_prices, __m512i& out_volumes) const {
    const auto& tier = _tiers[0];

    // Acquire load to ensure consistent snapshot
    uint64_t version = tier.seq.load(std::memory_order_acquire);

    out_prices = tier.prices;
    out_volumes = tier.volumes;

    return version;
}

void OrderBook::get_top_of_book(int32_t& best_bid, int32_t& best_ask) const {
    __m512i max_bid = _mm512_set1_epi32(INT32_MIN);
    __m512i min_ask = _mm512_set1_epi32(INT32_MAX);
    
    for (const auto& tier : _tiers) {
        __mmask16 bid_mask = tier.active_mask & 0x5555; // 0b0101010101010101 
        __mmask16 ask_mask = tier.active_mask & 0xAAAA; // 0b1010101010101010
        
        // volume > 0 的有效项
        __mmask16 vol_mask = _mm512_cmpneq_epi32_mask(tier.volumes, _mm512_setzero_si512());

        max_bid = _mm512_mask_max_epi32(max_bid, bid_mask & vol_mask, max_bid, tier.prices);
        min_ask = _mm512_mask_min_epi32(min_ask, ask_mask & vol_mask, min_ask, tier.prices);
    }
    
    best_bid = _mm512_reduce_max_epi32(max_bid);
    best_ask = _mm512_reduce_min_epi32(min_ask);
}