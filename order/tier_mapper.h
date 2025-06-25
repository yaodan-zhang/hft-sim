#ifndef TIER_MAPPER_H
#define TIER_MAPPER_H

#include <cstdint>
#include <cstddef>
#include <algorithm>

/**
 * @brief Maps price to tier index using static partitioning.
 */
class TierMapper {
public:
    static constexpr int TIER_GRANULARITY = 512;  // price per tier
    static constexpr int MAX_TIERS = 64;          // max tier buckets

    static size_t map(int32_t price) {
        return static_cast<size_t>(price >> 14);  // 举例：每 tier 跨 16384 个 price
    }
    
    /**
     * @brief Map a price to a tier index.
     */
    static inline size_t price_to_tier(int32_t price) {
        return std::min<size_t>(price / TIER_GRANULARITY, MAX_TIERS - 1);
    }
};

#endif // TIER_MAPPER_H
