// matching_engine.h
#pragma once
#include "orderbook.h"
#include <functional>
#include <immintrin.h>

struct FillReport {
    uint32_t taker_order_id;
    uint32_t maker_order_id;
    int32_t  traded_price;
    uint32_t traded_volume; // min of taker volume and maker volume
};

struct AckReport {
    uint32_t order_id;
    uint64_t order_timestamp;
    int32_t  order_price;
    uint32_t remaining_volume; // <= order volume
    Side     order_side;
};

struct CancelReport {
    uint32_t order_id;
    uint32_t cancelled_volume;
};

class MatchingEngine {
    public:
        MatchingEngine() = default;

        // Return order book.
        OrderBook& order_book();

        // Match an order, if success (matched in full or partial), call on_fill,
        // if failure (can't match) or partially filled, call on_ack and ack the order.
        // If above is successful, return true, o/w return false.
        bool match(const Order& order);

        // Cancel an order, if order exists and gets canceled, return true,
        // else return false (order doesn't exist or is already filled).
        bool cancel_order(uint32_t order_id);

        // Optional external callbacks
        std::function<void(const FillReport&)> on_fill = nullptr;
        std::function<void(const AckReport&)> on_ack = nullptr;
        std::function<void(const CancelReport&)> on_cancel = nullptr;

    private:
        OrderBook _order_book;
};
