#include "matching_engine.h"
#include "match_tier_avx512.h"
#include <algorithm>
#include <iostream>

MatchingEngine::MatchingEngine() : _order_book() {}

OrderBook& MatchingEngine::order_book() { return _order_book;}


bool MatchingEngine::match(const Order& incoming, std::function<void(uint32_t, uint32_t, uint32_t)> on_fill) {
    uint32_t remaining = incoming.volume;
    Side incoming_side = incoming.side;
    int32_t incoming_price = incoming.price;
    uint32_t incoming_id = incoming.order_id;

    for (size_t tier_idx = 0; tier_idx < OrderBook::MAX_TIERS && remaining > 0; ++tier_idx) {
        auto& tier = _order_book._tiers[tier_idx];

        __m512i& prices = tier.prices;
        __m512i& volumes = tier.volumes;
        __mmask16& active = tier.active_mask;
        __m512i& ids = tier.order_ids;

        __mmask16 matched_mask = match_tier_avx512(
            prices,
            volumes,
            active,
            incoming_side,
            incoming_price,
            remaining,
            incoming_id,
            on_fill,
            ids
        );

        active = matched_mask;
        tier.seq.fetch_add(1, std::memory_order_release);
    }

    if (remaining > 0) {
        Order residual = incoming;
        residual.volume = remaining;
        return _order_book.insert(residual);
    }

    return true;
}


bool MatchingEngine::cancel_order(uint32_t order_id) {
    auto it = _order_book._order_map.find(order_id);
    if (it == _order_book._order_map.end()) return false;

    auto [tier_idx, slot_idx] = it->second;
    auto& slot = _order_book._tiers[tier_idx].slots[slot_idx];

// 现在 slot 是真正的订单槽，可以访问 slot.active、slot.order.volume 等

    if (!slot.active || slot.order.volume == 0) return false;

    uint32_t cancelled_volume = slot.order.volume;
    slot.order.volume = 0;
    slot.active = false;

    _order_book._tiers[slot.tier_idx].active_mask &= ~(1 << slot.slot_idx);
    _order_book._tiers[slot.tier_idx].seq.fetch_add(1, std::memory_order_release);

    _order_book._order_map.erase(it);

    if (on_cancel) {
        on_cancel(CancelReport{
            .order_id = order_id,
            .cancelled_volume = cancelled_volume
        });
    }

    return true;
}
