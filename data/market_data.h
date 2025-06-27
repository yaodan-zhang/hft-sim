#pragma once
#include <immintrin.h>
#include <atomic>
#include <functional>

// Market data type (compatible with ITCH 5.0 and binary protocols)

enum class MsgType : uint8_t {
    ORDER_ADD    = 'A',
    ORDER_CANCEL = 'X',
};

// 64-byte aligned market data structure
struct alignas(64) MarketData {
    MsgType  type;       
    uint32_t order_id;    
    uint32_t timestamp;
    int32_t  price;       // $0.01/unit
    uint32_t volume;
    uint8_t  side;        // Bid: 0, Ask: 1;
};

// Compatible with AVX-512 instructions
static_assert(sizeof(MarketData) == 64, "MarketData must be exactly 64 bytes");
