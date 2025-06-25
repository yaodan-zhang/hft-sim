#pragma once
#include <immintrin.h>
#include <atomic>
#include <unordered_map>
#include <cstdint>
#include <array>
#include <optional>
#include "order.h"
#include <optional>

class OrderBook {
public:
    static constexpr size_t MAX_TIERS = 8;
    static constexpr size_t SLOT_PER_TIER = 16;

    struct Slot {
        Order order;    // 包含 order_id, price, volume, side, timestamp
        bool active;    // 是否活跃（即有效）
        size_t tier_idx;
        size_t slot_idx;
    };    

    struct Tier {
        std::array<Slot, SLOT_PER_TIER> slots;
        std::atomic<uint64_t> seq{0};
        __mmask16 active_mask = 0;
    };

    OrderBook();

    bool insert(const Order& order);
    bool cancel(uint32_t order_id, uint32_t& canceled_volume);
    bool reduce(uint32_t order_id, uint32_t reduce_by);

    size_t get_tier_index(int32_t price) const;

    // Get top of book snapshot
    std::pair<int32_t, int32_t> get_top_of_book() const;

    friend class MatchingEngine;

private:
    std::array<Tier, MAX_TIERS> _tiers;
    std::unordered_map<uint32_t, std::pair<size_t, size_t>> _order_map; // order_id -> (tier, slot)
};
