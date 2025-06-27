#include "matching_engine.h"
#include "match_tier_avx512.h"
#include <algorithm>
#include <iostream>

OrderBook& MatchingEngine::order_book() {
    return _order_book;
}

bool MatchingEngine::match(const Order& incoming) {
    // Get order volume
    uint32_t remaining = incoming.volume;

    // Match
    for (size_t tier_idx = 0; tier_idx < OrderBook::MAX_TIERS && remaining > 0; ++tier_idx) {
        OrderBook::Tier& tier = _order_book.get_tier(tier_idx);
        OrderBook::order_map_t& order_map = _order_book.get_map();

        __mmask16 new_active_mask = match_tier_avx512(
            tier.order_ids,
            tier.timestamps,
            tier.prices,
            tier.volumes,

            order_map,
            
            tier.active_mask,
            
            incoming,

            remaining,
            
            on_fill
        );

        tier.active_mask = new_active_mask;
    }

    // Ack order with remaining volume
    if (remaining > 0) {
        Order residual = incoming;
        residual.volume = remaining;
        bool insert_order = _order_book.insert(residual);
        if (!insert_order) {
            return false;
        } else {
            AckReport ack = {
                .order_id = residual.id,
                .order_timestamp = residual.timestamp,
                .order_price = residual.price,
                .remaining_volume = residual.volume,
                .order_side = residual.side
            };

            if (on_ack) {
                on_ack(ack);
            }
        }
    }

    return true;
}

bool MatchingEngine::cancel_order(uint32_t order_id) {
    OrderBook::order_map_t& order_map = _order_book.get_map();
    auto it = order_map.find(order_id);

    if (it == order_map.end()){
        return false;
    }

    // Erase order item in order map
    order_map.erase(it);

    // Remove mask entry in tier for this order
    auto [tier_idx, slot_idx] = it->second;

    OrderBook::Tier& tier = _order_book.get_tier(tier_idx);

    tier.active_mask &= ~(1 << slot_idx);

    if (on_cancel) {
        CancelReport c = {
            .order_id = order_id,
            .cancelled_volume = reinterpret_cast<uint32_t*>(&tier.volumes)[slot_idx]
        };
        on_cancel(c);
    }
    return true;
}
