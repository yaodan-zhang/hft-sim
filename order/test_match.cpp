#include "matching_engine.h"
#include <iostream>
#include <cassert>

void test_basic_match() {
    std::cout << "Test basic match..." << std::endl;
    MatchingEngine engine;

    alignas(64) int32_t ask_prices[16] = {
        0, 1100, 0, 1300,
        0, 1500, 0, 1700,
        0, 1900, 0, 2100,
        0, 2300, 0, 2500
    };
    
    alignas(64) uint32_t ask_volumes[16];
    for (int i = 0; i < 16; ++i) {
        ask_volumes[i] = (i % 2 == 1) ? 10 : 0;
    }

    __m512i ask_price_vec = _mm512_load_epi32(ask_prices);
    __m512i ask_volume_vec = _mm512_load_epi32(ask_volumes);
    __mmask16 ask_mask = 0xAAAA;

    engine.order_book().update(ask_price_vec, ask_volume_vec, ask_mask, 0);

    alignas(64) int32_t bid_prices[16] = {
        2500, 0, 2300, 0,
        2100, 0, 1900, 0,
        1700, 0, 1500, 0,
        1300, 0, 1100, 0
    };

    alignas(64) uint32_t bid_volumes[16];
    for (int i = 0; i < 16; ++i) {
        bid_volumes[i] = (i % 2 == 0) ? 10 : 0;
    }

    __m512i bid_price_vec = _mm512_load_epi32(bid_prices);

    __mmask16 unmatched = engine.match(bid_price_vec, bid_volumes, ask_mask);

    std::cout << "Unmatched Bid mask = ";
    for (int i = 15; i >= 0; --i)
        std::cout << ((unmatched >> i) & 1);
    std::cout << std::endl;

    for (int i = 0; i < 16; ++i) {
        if ((unmatched >> i) & 1) {
            std::cout << "[Unmatched] Bid[" << i << "] volume left = " << bid_volumes[i] << std::endl;
        } else {
            assert(bid_volumes[i]==0);
        }
    }

    auto [bid, ask] = engine.order_book().get_top_of_book();
    std::cout << "[Basic Match] Top of Book - Bid: " << bid << " Ask: " << ask << std::endl;
    assert(bid == 1700 && ask == 1900);
}

void test_full_match() {
    std::cout << "Test full match..." << std::endl;
    MatchingEngine engine;

    int32_t prices[16] = {0};
    uint32_t volumes[16] = {0};
    for (int i = 1; i < 16; i += 2) {
        prices[i] = 1010 + (i / 2) * 10; // ask[1,3,...,15] = 1010, 1020, ..., 1080
        volumes[i] = 10;
    }

    for (int i = 0; i < 16; i += 2) {
        prices[i] = 1080 + (i / 2) * 10; // bid[0,2,...,14] = 1080, 1090, ..., 1150
        volumes[i] = 10;
    }

    __m512i ask_prices = _mm512_load_epi32(prices);
    __m512i ask_vols = _mm512_load_epi32(volumes);
    engine.order_book().update(ask_prices, ask_vols, 0xAAAA, 0);

    __m512i bid_prices = _mm512_load_epi32(prices);
    uint32_t bid_vols_arr[16] = {0};
    for (int i = 0; i < 16; i += 2) {
        bid_vols_arr[i] = volumes[i];
    }

    __mmask16 unmatched = engine.match(bid_prices, bid_vols_arr, 0xAAAA);

    std::cout << "Unmatched Bid mask = ";
    for (int i = 15; i >= 0; --i)
        std::cout << ((unmatched >> i) & 1);
    std::cout << std::endl;

    assert(unmatched == 0);

    std::cout << "[Full Match] Top of Book..." << std::endl;
    auto [bid, ask] = engine.order_book().get_top_of_book();
    std::cout << "Top of Book - Bid: " << bid << " Ask: " << ask << std::endl;

    assert(bid == 0 && ask == 0);
}

void test_vol_partial_match() {
    std::cout << "Test volume partial match..." << std::endl;
    MatchingEngine engine;

    alignas(64) int32_t ask_prices[16] = {0};
    alignas(64) uint32_t ask_volumes[16] = {0};
    for (int i = 1; i < 16; i += 2) {
        ask_prices[i] = 1010 + (i / 2) * 10; // ask[1,3,...,15] = 1010, 1020, ..., 1080
        ask_volumes[i] = 5;
    }

    alignas(64) int32_t bid_prices[16] = {0};
    alignas(64) uint32_t bid_vols[16] = {0};
    for (int i = 0; i < 16; i += 2) {
        bid_prices[i] = 1010 + (i / 2) * 10; // bid[0,2,...,14] = 1010, 1020, ..., 1080
        bid_vols[i] = 10;
    }

    __m512i ask_price_vec = _mm512_load_epi32(ask_prices);
    __m512i ask_volume_vec = _mm512_load_epi32(ask_volumes);
    engine.order_book().update(ask_price_vec, ask_volume_vec, 0xAAAA, 0);

    __m512i bid_price_vec = _mm512_load_epi32(bid_prices);
    __mmask16 unmatched = engine.match(bid_price_vec, bid_vols, 0xAAAA);

    for (int i = 0; i < 16; i += 2) {
        if ((unmatched >> i) & 1) {
            std::cout << "Bid[" << i << "] volume left = " << bid_vols[i] << " (unmatched = 1)" << std::endl;
            assert(bid_vols[i] == 10);
        } else {
            std::cout << "Bid[" << i << "] volume left = " << bid_vols[i] << " (matched = 1)" << std::endl;
            assert(bid_vols[i] == 0);
        }
    }

    auto [bid, ask] = engine.order_book().get_top_of_book();
    std::cout << "[Vol Partial Match] Top of Book - Bid: " << bid << " Ask: " << ask << std::endl;
    assert(bid == 1050 && ask == 1070);
}

int main() {
    std::cout << "[All Tests] Begin:" << std::endl;
    test_basic_match();
    test_full_match();
    test_vol_partial_match();
    std::cout << "[All Tests] Completed." << std::endl;
    return 0;
}