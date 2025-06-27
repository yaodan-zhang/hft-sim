#pragma once
#include <immintrin.h>
#include <atomic>
#include <unordered_map>
#include <cstdint>
#include <array>
#include <utility>
#include "order.h"

class OrderBook {
public:
    using order_map_t = std::unordered_map<uint32_t, std::pair<size_t, size_t>>;

    // 16 orders per tier (8 bid on even positions + 8 ask on odd positions)
    static constexpr size_t MAX_TIERS = 10;
    static constexpr int32_t TIER_GRANULARITY = 8; // 每个 tier 跨越 8 bid/ask tick

    // bid 偶数，ask 奇数
    struct Tier {
        __m512i order_ids   = _mm512_setzero_si512();
        __m512i timestamps  = _mm512_setzero_si512();
        __m512i prices      = _mm512_setzero_si512();
        __m512i volumes     = _mm512_setzero_si512();
        __mmask16 active_mask = 0;
    };

    OrderBook() = default;

    // Get tier index of the order by its price, 
    // If price is invalid, return -1;
    // Else return a number between 0 and MAX_TIERS.
    size_t get_tier_index(int32_t price) const;

    // Insert a new order into the orderbook.
    // Return true if the order is inserted, and false if either input price is invalid or book is full.
    bool insert(const Order& order);

    // Cancel an order with order id, if successfully canceled, return true and store canceled volume into canceled_volume.
    // Else return false (order doesn't exist).
    bool cancel(uint32_t order_id, uint32_t& canceled_volume);

    // Reduce the volume of an order by reduce_by, return true if reduction is successful,
    // return false other wise (order doesn't exist or current volume is less than reduce_by).
    bool reduce(uint32_t order_id, uint32_t reduce_by);

    // Get the current highest bid and lowest ask. 
    // Return [highest_bid, lowest_ask].
    std::pair<int32_t, int32_t> get_top_of_book() const;

    // Get tier of order book using tier index.
    Tier& get_tier(size_t tier_idx);

    // Get order map.
    order_map_t& get_map();

private:
    std::array<Tier, MAX_TIERS> _tiers;
    order_map_t _order_map; // order_id -> (tier index, slot index)
};
