#pragma once
#include <cstdint>
enum class Side : uint8_t {
    BID = 0,
    ASK = 1,
};

struct Order {
    Order(uint32_t id, uint32_t ts, int32_t pr, uint32_t vol, Side sd)
        : id(id), timestamp(ts), price(pr), volume(vol), side(sd) {}

    uint32_t id;
    uint32_t timestamp;
    int32_t price;
    uint32_t volume;
    Side side;
};
