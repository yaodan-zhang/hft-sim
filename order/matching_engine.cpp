#include "matching_engine.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <tuple>
#include <bitset>

OrderBook& MatchingEngine::order_book() {
    return _order_book;
}

__mmask16 MatchingEngine::match(const __m512i& prices, uint32_t (&volumes)[16], __mmask16 side_mask) {
    __mmask16 unmatched_mask = 0;
    alignas(64) int32_t price_arr[16];
    _mm512_store_epi32(price_arr, prices);

    // // DEBUG PRINT
    // std::cout<< "Matching engine receives prices " << std::endl;
    // for (auto p : price_arr) {
    //     std::cout << "price " << p << std::endl;
    // }

    // std::cout<< "Matching engine receives volumes " << std::endl;
    // for (auto v : volumes) {
    //     std::cout << "vol " << v << std::endl;
    // }

    // std::cout << "Matching engine receives side mask: " << std::bitset<16>(side_mask) << std::endl;

    // // END OF DEBUG

    // Collect bids (from order book and input)
    std::vector<std::tuple<int32_t, int, size_t, bool>> bids; // (price, index, tier_idx, is_book)
    // From order book
    for (size_t t = 0; t < _order_book.MAX_TIERS; ++t) {
        auto& tier = _order_book._tiers[t];
        // Pass empty bid tier
        if (!(tier.active_mask & 0x5555)) {
            continue;
        }

        alignas(64) int32_t book_prices[16];
        alignas(64) uint32_t book_volumes[16];
        _mm512_store_epi32(book_prices, tier.prices);
        _mm512_store_epi32(book_volumes, tier.volumes);

        for (int j = 0; j < 16; ++j) {
            if (((tier.active_mask & 0x5555) >> j) & 1) {
                bids.emplace_back(book_prices[j], j, t, true);
            }
        }
    }

    // From input
    for (int i = 0; i < 16; ++i) {
        if (!((side_mask >> i) & 1) && volumes[i] > 0 && price_arr[i] > 0) { // Bid: side=0
            bids.emplace_back(price_arr[i], i, SIZE_MAX, false);
        }
    }

    // Sort: prioritize order book bids, then price descending
    std::sort(bids.begin(), bids.end(), [](const auto& a, const auto& b) {
        if (std::get<3>(a) != std::get<3>(b)) return std::get<3>(a); // true (book) before false (input)
        return std::get<0>(a) > std::get<0>(b); // Higher price first
    });

    // Collect asks (from order book and input)
    std::vector<std::tuple<int32_t, int, size_t, bool>> asks; // (price, index, tier_idx, is_book)
    // From order book
    for (size_t t = 0; t < _order_book.MAX_TIERS; ++t) {
        auto& tier = _order_book._tiers[t];
        // Skip empty ask tier
        if (!(tier.active_mask & 0xAAAA)) {
            continue;
        }

        alignas(64) int32_t book_prices[16];
        alignas(64) uint32_t book_volumes[16];
        _mm512_store_epi32(book_prices, tier.prices);
        _mm512_store_epi32(book_volumes, tier.volumes);

        for (int j = 0; j < 16; ++j) {
            if (((tier.active_mask & 0xAAAA) >> j) & 1) {
                asks.emplace_back(book_prices[j], j, t, true);
            }
        }
    }

    // From input
    for (int i = 0; i < 16; ++i) {
        if ((side_mask >> i) & 1 && volumes[i] > 0 && price_arr[i] > 0) { // Ask: side=1
            asks.emplace_back(price_arr[i], i, SIZE_MAX, false);
        }
    }

    // Sort: prioritize order book asks, then price ascending
    std::sort(asks.begin(), asks.end(), [](const auto& a, const auto& b) {
        if (std::get<3>(a) != std::get<3>(b)) return std::get<3>(a); // true (book) before false (input)
        return std::get<0>(a) < std::get<0>(b); // Lower price first
    });

    // // DEBUG
    // // PRINT existing bid and ask.
    // std::cout << "Existing bids are: " << std::endl;
    // for (auto & bid: bids) {
    //     int32_t bid_price = std::get<0>(bid);
    //     int i = std::get<1>(bid);
    //     size_t bid_tier = std::get<2>(bid);
    //     bool is_book = std::get<3>(bid);
    //     uint32_t bid_vol;

    //     // Parse bid volume
    //     if (!is_book) {
    //         bid_vol = volumes[i];
    //     } else {
    //         alignas(64) uint32_t book_volumes[16];
    //         _mm512_store_epi32(book_volumes, _order_book._tiers[bid_tier].volumes);
    //         bid_vol = book_volumes[i];
    //     }

    //     std::cout << "Bid price: " << bid_price << " bid volume: " << bid_vol << std::endl; 
    // }

    // std::cout << "Existing asks are: " << std::endl;
    // for (auto & ask: asks) {
    //     int32_t ask_price = std::get<0>(ask);
    //     int i = std::get<1>(ask);
    //     size_t ask_tier = std::get<2>(ask);
    //     bool is_book = std::get<3>(ask);
    //     uint32_t ask_vol;

    //     // Parse bid volume
    //     if (!is_book) {
    //         ask_vol = volumes[i];
    //     } else {
    //         alignas(64) uint32_t book_volumes[16];
    //         _mm512_store_epi32(book_volumes, _order_book._tiers[ask_tier].volumes);
    //         ask_vol = book_volumes[i];
    //     }

    //     std::cout << "Ask price: " << ask_price << " bid volume: " << ask_vol << std::endl; 
    // }
    // // END OF DEBUG.


    // Match existing bids and asks : (price, index, tier_idx, is_book)
    for (auto bid_it = bids.begin(); bid_it != bids.end(); ++bid_it) {
        
        // Parse bid item - get bid price, index (input index or index in ._tier[tier_idx]), tier_idx
        int32_t bid_price = std::get<0>(*bid_it);
        int i = std::get<1>(*bid_it);
        size_t bid_tier = std::get<2>(*bid_it);
        bool is_book = std::get<3>(*bid_it);
        uint32_t bid_vol;

        // Parse bid volume
        if (!is_book) {
            bid_vol = volumes[i];
        } else {
            alignas(64) uint32_t book_volumes[16];
            _mm512_store_epi32(book_volumes, _order_book._tiers[bid_tier].volumes);
            bid_vol = book_volumes[i];
        }

        // std::cout << " Matching bid price " << bid_price << " and bid volume " << bid_vol << std::endl;

        // Iterate through existing asks and match with current bid
        bool terminate = false;
        for (auto ask_it = asks.begin(); ask_it != asks.end();) {

            // Parse ask price, index, tier_idx
            int32_t ask_price = std::get<0>(*ask_it);
            int j = std::get<1>(*ask_it);
            size_t ask_tier = std::get<2>(*ask_it);
            bool ask_is_book = std::get<3>(*ask_it);
            uint32_t ask_vol;

            // Parse ask volume
            if (!ask_is_book) {
                ask_vol = volumes[j];
            } else {
                alignas(64) uint32_t book_volumes[16];
                _mm512_store_epi32(book_volumes, _order_book._tiers[ask_tier].volumes);
                ask_vol = book_volumes[j];
            }

            // std::cout << " Matching ask price " << ask_price << " and ask volume " << ask_vol << std::endl;

            if (ask_price <= bid_price && ask_vol > 0 && bid_vol > 0) {
                uint32_t traded = std::min(ask_vol, bid_vol);
                bid_vol -= traded;

                // Update ask volume
                if (!ask_is_book) {
                    volumes[j] -= traded;
                } else {
                    auto& tier = _order_book._tiers[ask_tier];

                    alignas(64) uint32_t book_volumes[16];
                    _mm512_store_epi32(book_volumes, tier.volumes);

                    book_volumes[j] -= traded;
                    
                    // Close pos j of tier's active mask if ask is matched in whole.
                    if (book_volumes[j] == 0) {
                        tier.active_mask &= ~(1 << j);
                    }

                    tier.volumes = _mm512_load_epi32(book_volumes);
                }

                // Update bid volume
                if (!is_book) {
                    volumes[i] -= traded;
                } else {
                    auto& tier = _order_book._tiers[bid_tier];
                    alignas(64) uint32_t book_volumes[16];
                    _mm512_store_epi32(book_volumes, tier.volumes);
                    book_volumes[i] -= traded;

                    // Close pos i of tier's active mask if bid is matched in whole.
                    if (book_volumes[i] == 0) {
                        tier.active_mask &= ~(1 << i);
                    }
                    tier.volumes = _mm512_load_epi32(book_volumes);
                }

                if (ask_vol == traded) {
                    ask_it = asks.erase(ask_it);
                } else {
                    ++ask_it;
                }

            } else {
                // terminate = true;
                // break; // Ask price > bid. No matching is possible for current bid and all bids following.
                ++ask_it;
            }

            // // Current bid is matched in full, proceed with the next bid.
            if (bid_vol == 0) {
                break;
            }
        }

        if (bid_vol > 0 && !is_book) {
            unmatched_mask |= (1 << i);
        }

        // if (terminate) {
        //     break; // Terminate current matching.
        // }

    }

    // Update order book with unmatched input orders
    alignas(64) int32_t residual_prices[16] = {0};
    alignas(64) uint32_t residual_volumes[16] = {0};
    __mmask16 bid_residual_mask = 0;
    __mmask16 ask_residual_mask = 0;

    // Collect unmatched bids (input only)
    for (int i = 0; i < 16; ++i) {
        if (volumes[i] > 0 && prices[i] > 0 && !((side_mask >> i) & 1)) {
            residual_prices[i] = price_arr[i];
            residual_volumes[i] = volumes[i];
            bid_residual_mask |= (1 << i);
        }
    }

    // Collect unmatched asks (input only)
    for (int i = 0; i < 16; ++i) {
        if (volumes[i] > 0 && prices[i] > 0 && (side_mask >> i) & 1) {
            residual_prices[i] = price_arr[i];
            residual_volumes[i] = volumes[i];
            ask_residual_mask |= (1 << i);
        }
    }

    if (bid_residual_mask || ask_residual_mask) {
        _order_book.update(
            _mm512_load_epi32(residual_prices),
            _mm512_load_epi32(residual_volumes),
            ask_residual_mask,
            bid_residual_mask
        );
    }
 
    return unmatched_mask;
}