#include "orderbook.h"
#include <iostream>
#include <limits>

// Ask_mask should only reserve ones on odd position, and bid_mask on even positions.
void OrderBook::update(const __m512i& prices, const __m512i& volumes, __mmask16 ask_mask, __mmask16 bid_mask) {
    // Load new orders – price and volume
    alignas(64) int32_t price_arr[16];
    alignas(64) uint32_t volume_arr[16];
    _mm512_store_si512(price_arr, prices);
    _mm512_store_si512(volume_arr, volumes);

    size_t tier_idx = 0; // Single tier for now
    auto& tier = _tiers[tier_idx];

    // Load existing orders - price and volume
    alignas(64) int32_t existing_prices[16];
    alignas(64) uint32_t existing_volumes[16];
    _mm512_store_si512(existing_prices, tier.prices);
    _mm512_store_si512(existing_volumes, tier.volumes);

    __mmask16 existing_active_mask = tier.active_mask;
    
    for (int i = 0; i < 16; ++i) {
        if ((volume_arr[i] > 0) && (price_arr[i] > 0) && ((ask_mask >> i) & 1 || (bid_mask >> i) & 1)) {
            // Only update if the slot is not active
            if (!((existing_active_mask >> i) & 1)) {
                existing_prices[i] = price_arr[i];
                existing_volumes[i] = volume_arr[i];
                existing_active_mask |= (1 << i);
            }
        }
    }

    // Update tier
    tier.prices = _mm512_load_si512(existing_prices);
    tier.volumes = _mm512_load_si512(existing_volumes);
    tier.active_mask = existing_active_mask;
}


std::pair<int32_t, int32_t> OrderBook::get_top_of_book() const {
    int32_t top_bid = 0;
    int32_t top_ask = std::numeric_limits<int32_t>::max();

    for (size_t t = 0; t < MAX_TIERS; ++t) {
        const auto& tier = _tiers[t];
        if (!tier.active_mask) {
            continue;
        }

        alignas(64) int32_t prices[16];
        alignas(64) uint32_t volumes[16];
        _mm512_store_si512(prices, tier.prices);
        _mm512_store_si512(volumes, tier.volumes);

        for (int i = 0; i < 16; ++i) {
            if (((tier.active_mask & 0x5555) >> i) & 1 && volumes[i] > 0) {
                top_bid = std::max(top_bid, prices[i]);
            }
            if (((tier.active_mask & 0xAAAA) >> i) & 1 && volumes[i] > 0) {
                top_ask = std::min(top_ask, prices[i]);
            }
        }
    }

    return {top_bid, top_ask == std::numeric_limits<int32_t>::max() ? 0 : top_ask};
}
