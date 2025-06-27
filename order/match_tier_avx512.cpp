#include "match_tier_avx512.h"
#include <immintrin.h>
#include <functional>
#include <algorithm>
#include <vector>
#include <iostream>
#include <bitset>

__mmask16 match_tier_avx512(
    __m512i& tier_order_ids,
    __m512i& tier_timestamps,
    __m512i& tier_prices,
    __m512i& tier_volumes,
   
    OrderBook::order_map_t& order_map,

    __mmask16& tier_active_mask,

    const Order& incoming,

    uint32_t& remaining,

    std::function<void(FillReport&)> on_fill
) {
    // Get target order side mask, oppsed to incoming side.
    __mmask16 side_mask = (incoming.side == Side::BID) ? 0xAAAA : 0x5555;
    __mmask16 valid_mask = tier_active_mask & side_mask;

    // Get tier prices.
    __mmask16 price_cmp_mask = (incoming.side == Side::BID)
        ? _mm512_cmp_epi32_mask(_mm512_set1_epi32(incoming.price), tier_prices, _MM_CMPINT_GE)
        : _mm512_cmp_epi32_mask(_mm512_set1_epi32(incoming.price), tier_prices, _MM_CMPINT_LE);

    valid_mask &= price_cmp_mask;

    alignas(64) uint32_t id_arr[16];
    alignas(64) uint32_t ts_arr[16];
    alignas(64) int32_t price_arr[16];
    alignas(64) uint32_t vol_arr[16];

    _mm512_store_epi32(id_arr, tier_order_ids);
    _mm512_store_epi32(ts_arr, tier_timestamps);
    _mm512_store_epi32(price_arr, tier_prices);
    _mm512_store_epi32(vol_arr, tier_volumes);
    
    // Sort indices by price-time priority
    std::vector<int> indices;
    for (int i = 0; i < 16; ++i){ 
        if ((valid_mask >> i) & 1) {
            indices.push_back(i);
        }
    }

    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        if (incoming.side == Side::BID) {
            if (price_arr[a] != price_arr[b]) return price_arr[a] < price_arr[b]; // Get Ask: price lower first
        } else {
            if (price_arr[a] != price_arr[b]) return price_arr[a] > price_arr[b]; // Get Bid: price higher first
        }
        return ts_arr[a] < ts_arr[b]; // Then FIFO
    });

    // Match
    for (int i : indices) {
        // Break when incoming order is matched in full
        if (remaining == 0) {
            break;
        }

        // Updated traded volume for both sides
        uint32_t vol = vol_arr[i];
        uint32_t traded = std::min(remaining, vol);
        remaining -= traded;
        vol_arr[i] -= traded;

        // Erase mask entry and order item in order map
        if (vol_arr[i] == 0) {
            order_map.erase(order_map.find(id_arr[i]));
        }

        // Report fill for both sides
        if (on_fill) {
            FillReport f = {
                .taker_order_id = incoming.id,
                .maker_order_id = id_arr[i],
                .traded_price = price_arr[i],
                .traded_volume = traded
            };
            on_fill(f);
        }
    }

    // Updated tier volumes
    tier_volumes = _mm512_load_epi32(vol_arr);

    // Return tier new active mask
    return ((valid_mask ^ (tier_active_mask & side_mask)) | (tier_active_mask & (~side_mask))); // Keep incoming side's active mask unchanged.
}
