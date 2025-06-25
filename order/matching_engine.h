#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H
#include "orderbook.h"
#include <functional>

class MatchingEngine {
public:
    MatchingEngine();

    OrderBook& order_book();

    // 单订单撮合，返回是否全部成交或成功挂单
    bool match(const Order& incoming, std::function<void(uint32_t, uint32_t, uint32_t)> on_fill);

    // 撤单接口
    bool cancel_order(uint32_t order_id);

    // 可选的外部回调
    std::function<void(const AckReport&)> on_ack;
    std::function<void(const FillReport&)> on_fill;
    std::function<void(const CancelReport&)> on_cancel;

private:
    OrderBook _order_book;
};

#endif // MATCHING_ENGINE_H
