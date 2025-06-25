#pragma once
#include <immintrin.h>
#include <cstdint>
#include <functional>
#include "order.h"

__mmask16 match_tier_avx512(
    __m512i& book_prices,
    __m512i& book_volumes,
    __mmask16 active_mask,
    Side incoming_side,
    int32_t incoming_price,
    uint32_t& remaining,
    uint32_t incoming_order_id,
    const std::function<void(uint32_t, uint32_t, uint32_t)>& on_fill,
    __m512i& order_ids
) {
    __mmask16 side_mask = (incoming_side == Side::BID) ? 0xAAAA : 0x5555;
    __mmask16 valid_mask = active_mask & side_mask;

    __m512i price_vec = book_prices;
    __mmask16 price_cmp_mask = (incoming_side == Side::BID)
        ? _mm512_cmp_epi32_mask(_mm512_set1_epi32(incoming_price), price_vec, _MM_CMPINT_GE)
        : _mm512_cmp_epi32_mask(_mm512_set1_epi32(incoming_price), price_vec, _MM_CMPINT_LE);

    valid_mask &= price_cmp_mask;

    __m512i vol_vec = book_volumes;
    __mmask16 vol_nonzero = _mm512_cmp_epi32_mask(vol_vec, _mm512_setzero_si512(), _MM_CMPINT_GT);
    valid_mask &= vol_nonzero;

    alignas(64) int32_t vol_arr[16], id_arr[16];
    _mm512_store_epi32(vol_arr, vol_vec);
    _mm512_store_epi32(id_arr, order_ids);

    for (int i = 0; i < 16 && remaining > 0; ++i) {
        if ((valid_mask >> i) & 1) {
            uint32_t traded = std::min(remaining, static_cast<uint32_t>(vol_arr[i]));
            remaining -= traded;
            vol_arr[i] -= traded;

            if (on_fill) {
                on_fill(incoming_order_id, static_cast<uint32_t>(id_arr[i]), traded);
            }

            if (vol_arr[i] == 0) {
                valid_mask &= ~(1 << i);
            }
        }
    }

    book_volumes = _mm512_load_epi32(vol_arr);
    return valid_mask;
}
