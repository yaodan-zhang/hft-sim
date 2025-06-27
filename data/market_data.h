#pragma once
#include <immintrin.h>
#include <atomic>
#include <functional>

// 市场数据类型 (兼容ITCH 5.0和Binary协议)
enum class MsgType : uint8_t {
    ORDER_ADD    = 'A',
    ORDER_CANCEL = 'X',
};

// 64字节对齐的行情数据结构 (40字节有效载荷)
struct alignas(64) MarketData {
    MsgType  type;       
    uint32_t order_id;    
    uint32_t timestamp;
    int32_t  price;       // $0.01/unit
    uint32_t volume;
    uint8_t  side;        // Bid: 0, Ask: 1;
};

// Aligns with AVX requirements
static_assert(sizeof(MarketData) == 64, "MarketData must be exactly 64 bytes");
