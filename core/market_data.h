#pragma once
#include <immintrin.h>
#include <atomic>
#include <functional>

// 市场数据类型 (兼容ITCH 5.0和Binary协议)
enum class MsgType : uint8_t {
    ORDER_ADD    = 'A',
    ORDER_EXEC   = 'E',
    ORDER_CANCEL = 'X',
    SNAPSHOT     = 'S'
};

// 64字节对齐的行情数据结构 (40字节有效载荷)
struct alignas(64) MarketData {
    MsgType  type;        // 1 byte
    uint8_t  side;        // 1 = ask, 0 = bid
    uint16_t _pad1;       // padding
    uint32_t _pad2;       // padding

    uint64_t timestamp;   // 纳秒级时间戳
    uint32_t order_id;    
    int32_t  price;       // 以 0.01 美分为单位
    uint32_t volume;

    uint8_t  flags;       // 可用于 future 扩展
    uint8_t  _reserved[15];  // 保证总共 64 字节
};

static_assert(sizeof(MarketData) == 64, "MarketData must be exactly 64 bytes");
