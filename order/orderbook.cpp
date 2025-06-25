#include "orderbook.h"
#include <immintrin.h>
#include <cstring>
#include <limits>
#include <algorithm>
#include "tier_mapper.h"

OrderBook::OrderBook() {
    for (auto& tier : _tiers) {
        for (auto& slot : tier.slots) {
            slot.order.volume = 0;
            slot.order.price = 0;
            slot.order.order_id = 0;
            slot.order.timestamp = 0;
            slot.order.side = Side::BID;
            slot.active = false;
        }
        tier.active_mask = 0;
        tier.seq.store(0, std::memory_order_relaxed);
    }
}

size_t OrderBook::get_tier_index(int32_t price) const {
    return TierMapper::map(price); // 默认实现是 price >> 14
}

bool OrderBook::insert(const Order& order) {
    size_t tier_idx = get_tier_index(order.price);
    if (tier_idx >= MAX_TIERS) return false;

    Tier& tier = _tiers[tier_idx];
    for (int i = 0; i < 16; ++i) {
        if (!tier.slots[i].active) {
            tier.slots[i] = {
                .order = {
                order.order_id,
                order.timestamp,
                order.price,
                order.volume,
                order.side,},
                .active = true
            };
            tier.active_mask |= (1 << i);
            tier.seq.fetch_add(1, std::memory_order_release);
            _order_map[order.order_id] = {tier_idx, static_cast<uint8_t>(i)};
            return true;
        }
    }
    return false;
}

bool OrderBook::cancel(uint32_t order_id, uint32_t& canceled_volume) {
    auto it = _order_map.find(order_id);
    if (it == _order_map.end()) return false;

    auto [tier_idx, slot_idx] = it->second;
    Tier& tier = _tiers[tier_idx];
    Slot& slot = tier.slots[slot_idx];

    if (!slot.active) return false;

    canceled_volume = slot.order.volume;
    slot.active = false;
    tier.active_mask &= ~(1 << slot_idx);
    tier.seq.fetch_add(1, std::memory_order_release);
    _order_map.erase(it);
    return true;
}

bool OrderBook::reduce(uint32_t order_id, uint32_t vol) {
    auto it = _order_map.find(order_id);
    if (it == _order_map.end()) return false;

    auto [tier_idx, slot_idx] = it->second;
    Tier& tier = _tiers[tier_idx];
    Slot& slot = tier.slots[slot_idx];

    if (!slot.active) return false;

    if (slot.order.volume <= vol) {
        tier.active_mask &= ~(1 << slot_idx);
        slot.active = false;
        _order_map.erase(it);
    } else {
        slot.order.volume -= vol;
    }
    tier.seq.fetch_add(1, std::memory_order_release);
    return true;
}

std::pair<int32_t, int32_t> OrderBook::get_top_of_book() const {
    int32_t best_bid = std::numeric_limits<int32_t>::min();
    int32_t best_ask = std::numeric_limits<int32_t>::max();

    for (const auto& tier : _tiers) {
        for (int i = 0; i < 16; ++i) {
            const auto& slot = tier.slots[i];
            if (!slot.active || slot.order.volume == 0) continue;
            if (slot.order.side == Side::BID) {
                best_bid = std::max(best_bid, slot.order.price);
            } else {
                best_ask = std::min(best_ask, slot.order.price);
            }
        }
    }

    if (best_bid == std::numeric_limits<int32_t>::min()) best_bid = 0;
    if (best_ask == std::numeric_limits<int32_t>::max()) best_ask = 0;

    return {best_bid, best_ask};
}
