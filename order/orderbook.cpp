#include "orderbook.h"
#include <limits>
#include <cstring>
#include <iostream>
#include <bitset>

OrderBook::Tier& OrderBook::get_tier(size_t tier_idx) {
    return _tiers[tier_idx];
}

OrderBook::order_map_t& OrderBook::get_map() {
    return _order_map;
}

size_t OrderBook::get_tier_index(int32_t price) const  {
    int32_t tier = price / TIER_GRANULARITY;
    if (tier < 0) {
        return -1;
    }
    if (tier >= MAX_TIERS) {
        return MAX_TIERS - 1;
    }
    return static_cast<size_t>(tier);
}

bool OrderBook::insert(const Order& order) {
    size_t tier_idx = get_tier_index(order.price);
    if (tier_idx < 0) {
        return false;
    }
    
    auto& tier = _tiers[tier_idx];

    size_t start = order.side == Side::BID? 0 : 1;
    for (size_t i = start; i < 16; i+=2) {

        // 检查该口是否为空
        if (!((tier.active_mask >> i) & 1)) {
            // 写入order
            reinterpret_cast<uint32_t*>(&tier.order_ids)[i]     = order.id;
            reinterpret_cast<uint32_t*>(&tier.timestamps)[i]    = order.timestamp;
            reinterpret_cast<int32_t*>(&tier.prices)[i]         = order.price;
            reinterpret_cast<uint32_t*>(&tier.volumes)[i]       = order.volume;

            // 设置 active bit
            tier.active_mask |= (1 << i);

            _order_map[order.id] = {tier_idx, i};
            return true;
        }
    }
    return false;
}

bool OrderBook::cancel(uint32_t order_id, uint32_t& canceled_volume) {
    auto it = _order_map.find(order_id);
    if (it == _order_map.end()) {
        return false;
    }

    auto [tier_idx, slot_idx] = it->second;
    Tier& tier = _tiers[tier_idx];

    // 从 volumes 向量中取出该 slot 的原始值
    uint32_t* volume_ptr = reinterpret_cast<uint32_t*>(&tier.volumes);
    canceled_volume = volume_ptr[slot_idx];

    // 清除 active_mask
    tier.active_mask &= ~(1 << slot_idx);

    // 移除 order_map 映射
    _order_map.erase(it);

    return true;
}

bool OrderBook::reduce(uint32_t order_id, uint32_t reduce_by) {
    auto it = _order_map.find(order_id);
    if (it == _order_map.end()) {
        return false;
    }

    auto [tier_idx, slot_idx] = it->second;
    Tier& tier = _tiers[tier_idx];

    uint32_t* volume_ptr = reinterpret_cast<uint32_t*>(&tier.volumes);
    if (volume_ptr[slot_idx] < reduce_by) {
        return false;
    }

    // Reduce
    volume_ptr[slot_idx] -= reduce_by;
    if (volume_ptr[slot_idx] == 0) {
        // 全部减完，视为取消
        tier.active_mask &= ~(1 << slot_idx);
        _order_map.erase(it);
    }
    return true;
}

std::pair<int32_t, int32_t> OrderBook::get_top_of_book() const {
    __m512i max_bid = _mm512_set1_epi32(std::numeric_limits<int32_t>::min());
    __m512i min_ask = _mm512_set1_epi32(std::numeric_limits<int32_t>::max());

    for (const auto& tier : _tiers) {
        // 加载价格和数量
        __m512i prices = tier.prices;
        __mmask16 mask = tier.active_mask;

        // 对于有效的 bid 槽位（偶数位）
        __mmask16 bid_mask = mask & 0x5555;
        max_bid = _mm512_mask_max_epi32(max_bid, bid_mask, max_bid, prices);

        // 对于有效的 ask 槽位（奇数位）
        __mmask16 ask_mask = mask & 0xAAAA;
        min_ask = _mm512_mask_min_epi32(min_ask, ask_mask, min_ask, prices);
    }

    alignas(64) int32_t bid_arr[16], ask_arr[16];
    _mm512_store_epi32(bid_arr, max_bid);
    _mm512_store_epi32(ask_arr, min_ask);

    int32_t best_bid = std::numeric_limits<int32_t>::min();
    int32_t best_ask = std::numeric_limits<int32_t>::max();

    for (int i = 0; i < 16; ++i) {
        best_bid = std::max(best_bid, bid_arr[i]);
        best_ask = std::min(best_ask, ask_arr[i]);
    }

    return {
        best_bid == std::numeric_limits<int32_t>::min()? 0 : best_bid, 
        best_ask == std::numeric_limits<int32_t>::max()? 0 : best_ask
    };
}
