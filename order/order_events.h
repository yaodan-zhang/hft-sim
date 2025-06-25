#pragma once
#include <cstdint>

enum class EventType {
    ACK,
    FILL,
    CANCEL
};

struct OrderAck {
    uint32_t order_id;
    int32_t price;
    uint32_t volume;
    bool is_partial;
};

struct OrderFill {
    uint32_t order_id;
    uint32_t matched_order_id;
    uint32_t volume;
    int32_t price;
    bool is_passive; // 被动成交？
};

struct OrderCancel {
    uint32_t order_id;
    int32_t price;
    uint32_t canceled_volume;
};
