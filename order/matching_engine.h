#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include "orderbook.h"
#include <immintrin.h>

/**
 * @class MatchingEngine
 * @brief Manages the matching of buy (bid) and sell (ask) orders against an order book.
 *
 * The MatchingEngine processes incoming orders, matching bids against asks using a greedy algorithm.
 * It interacts with an OrderBook to store unmatched orders and retrieve the top of the book.
 */
class MatchingEngine {
public:
    /**
     * @brief Constructs a MatchingEngine with an initialized OrderBook.
     *
     * Initializes the internal OrderBook to an empty state, ready to process orders.
     */
    MatchingEngine() : _order_book() {}

    /**
     * @brief Provides access to the internal OrderBook.
     *
     * @return A reference to the OrderBook used for storing and managing orders.
     */
    OrderBook& order_book();

    /**
     * @brief Matches incoming bids against asks using a greedy algorithm.
     *
     * Each bid is matched against all asks with a price less than or equal to the bid price
     * until the bid's volume is exhausted or no more compatible asks are available.
     * Unmatched bids and asks are stored in the OrderBook. Bids are expected in even indices
     * (0, 2, 4, 6, 8, 10, 12, 14), and asks in odd indices (1, 3, 5, 7, 9, 11, 13, 15).
     * 
     * Matching rules (decreasing priority):
     * 1. Existing orders in the orderbook is considered first than newly arriving ones.
     * 2. Highest bid is considered first and lowest ask is considered first.
     * 
     * @param prices SIMD vector containing 16 32-bit order prices.
     * @param volumes Array of 16 32-bit order volumes.
     * @param side_mask Bitmask indicating order sides (0 = bid, 1 = ask for each index).
     * @return Bitmask indicating which orders were unmatched (1 = unmatched, 0 = matched or invalid).
     */
    __mmask16 match(const __m512i& prices, uint32_t (&volumes)[16], __mmask16 side_mask);

private:
    OrderBook _order_book; ///< Internal OrderBook for storing unmatched bids and asks.
};

#endif // MATCHING_ENGINE_H
