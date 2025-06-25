#pragma once
#include <cstdint>

enum class Side : uint8_t {
    BID = 0,
    ASK = 1
};

struct Order {
    uint32_t order_id;
    uint64_t timestamp;
    int32_t price;
    uint32_t volume;
    Side side;

    Order() = default;
    Order(uint32_t id, uint64_t ts, int32_t pr, uint32_t vol, Side sd)
        : order_id(id), timestamp(ts), price(pr), volume(vol), side(sd) {}
};

// 成交回报
struct FillReport {
    uint32_t taker_order_id;
    uint32_t maker_order_id;
    uint32_t volume;
    int32_t  price;
};

// 挂单确认回报
struct AckReport {
    uint32_t order_id;
    int32_t  price;
    uint32_t volume;
    Side     side;
};

// 撤单回报
struct CancelReport {
    uint32_t order_id;
    uint32_t cancelled_volume;
};
