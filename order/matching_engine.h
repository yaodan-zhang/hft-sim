// matching_engine.h
#pragma once
#include "orderbook.h"
#include <functional>
#include <immintrin.h>

struct FillReport {
    uint32_t taker_order_id;
    uint32_t maker_order_id;
    int32_t  traded_price;
    uint32_t traded_volume;
};

struct AckReport {
    uint32_t order_id;
    uint64_t order_timestamp;
    int32_t  order_price;
    uint32_t remaining_volume;
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

        // 撮合一个订单，若成功，调用on_fill (taker_id, maker_id, price, traded volume (<= volume)),
        // 若撮合不成功或有剩余，调用on_ack (order_id, timestamp, price, remaining volume (<=volume), side).
        // 若成功执行上述请求，返回true，否则返回false。
        bool match(const Order& order);

        // 撤单,成功的话返回true，同时调用on_cancel, 失败返回false(订单不存在).
        bool cancel_order(uint32_t order_id);

        // 可选的外部回调
        std::function<void(const FillReport&)> on_fill = nullptr;
        std::function<void(const AckReport&)> on_ack = nullptr;
        std::function<void(const CancelReport&)> on_cancel = nullptr;

    private:
        OrderBook _order_book;
};
