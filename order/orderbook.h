#ifndef ORDERBOOK_H
#define ORDERBOOK_H
#include <immintrin.h>
#include <atomic>
#include <utility>

/**
 * @class OrderBook
 * @brief Represents an order book for matching buy (bid) and sell (ask) orders.
 * 
 * Manages a MAX_TIERS of orders using AVX-512 SIMD instructions for efficient
 * processing. Bids are stored in even indices (0, 2, 4, 6, 8, 10, 12, 14) and
 * asks in odd indices (1, 3, 5, 7, 9, 11, 13, 15) to maintain separation.
 */
class OrderBook {
public:
    static constexpr size_t MAX_TIERS = 8;

    /**
     * @struct Tier
     * @brief Represents a single tier in the order book, holding prices and volumes.
     */
    struct Tier {
        __m512i prices;       ///< SIMD vector storing 16 32-bit order prices.
        __m512i volumes;      ///< SIMD vector storing 16 32-bit order volumes.
        __mmask16 active_mask; ///< Bitmask indicating active slots (1 = active, 0 = inactive).
        /// __mmask16 bid_mask = active_mask & 0x5555;
        /// __mmask16 ask_mask = active_mask & 0xAAAA;
        std::atomic<uint64_t> seq{0};
    };

    Tier _tiers[MAX_TIERS];

    /**
     * @brief Constructs an OrderBook with initialized empty tiers.
     * 
     * Initializes all tiers with zeroed prices, volumes, and masks.
     */
    OrderBook() {
        for (auto& tier : _tiers) {
            tier.prices = _mm512_setzero_si512();
            tier.volumes = _mm512_setzero_si512();
            tier.active_mask = 0;
        }
    }
    
    /**
     * @brief Updates the order book with new prices and volumes.
     * 
     * Processes incoming bid and ask orders, mapping bids to even indices (0, 2, 4, 6, 8, 10, 12, 14)
     * and asks to odd indices (1, 3, 5, 7, 9, 11, 13, 15). Preserves existing orders and updates masks accordingly.
     * 
     * @param prices SIMD vector containing 16 32-bit order prices.
     * @param volumes SIMD vector containing 16 32-bit order volumes.
     * @param ask_mask Bitmask indicating which slots are asks (1 = ask).
     * @param bid_mask Bitmask indicating which slots are bids (1 = bid).
     */
    void update(const __m512i& prices, const __m512i& volumes, __mmask16 ask_mask, __mmask16 bid_mask);

    /**
     * @brief Retrieves the top of the book (highest bid and lowest ask prices).
     * 
     * Scans the order book to find the highest bid price (from even indices) and
     * the lowest ask price (from odd indices) with non-zero volume.
     * 
     * @return A pair containing the highest bid price and the lowest ask price.
     *         Returns (0, 0) if no valid bid or ask is found.
     */
    std::pair<int32_t, int32_t> get_top_of_book() const;
    
};

#endif // ORDERBOOK_H