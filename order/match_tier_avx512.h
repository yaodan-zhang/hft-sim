#pragma once
#include <immintrin.h>
#include <functional>
#include "order.h"
#include "orderbook.h"
#include "matching_engine.h"

// Performs AVX-512 vectorized order matching within a single tier.
// Applies price-time priority to match incoming order against active orders.
// Returns the updated active mask after matching.
__mmask16 match_tier_avx512(
    __m512i& tier_order_ids,
    __m512i& tier_timestamps,
    __m512i& tier_prices,
    __m512i& tier_volumes,
   
    OrderBook::order_map_t& order_map,

    __mmask16& tier_active_mask,

    const Order& incoming,

    uint32_t& remaining,

    std::function<void(FillReport&)> on_fill = nullptr
);
