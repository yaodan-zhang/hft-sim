#include <immintrin.h>
#include <iostream>
#include <bitset>
#include <array>

int main() {
    // 主动订单（taker）
    int32_t taker_price = 100;
    uint32_t taker_volume = 30;

    __m512i taker_price_vec = _mm512_set1_epi32(taker_price);
    __m512i taker_vol_vec = _mm512_set1_epi32(taker_volume);

    // 假设一个 Tier 中有 16 个 ask 挂单
    alignas(64) int32_t prices[16]  = {95, 98, 100, 101, 102, 103, 104, 105,
                                       106, 107, 108, 109, 110, 111, 112, 113};
    alignas(64) uint32_t volumes[16] = {5, 5, 5, 5, 5, 5, 5, 5,
                                        5, 5, 5, 5, 5, 5, 5, 5};

    __m512i price_vec = _mm512_load_epi32(prices);
    __m512i volume_vec = _mm512_load_epi32(volumes);

    // 假设有效位掩码（active）和 side 掩码（ask=1）
    __mmask16 active_mask = 0xFFFF;  // 所有 slot 都有效
    __mmask16 ask_mask = 0xFFFF;     // 全是 ask 单

    // 撮合逻辑
    __mmask16 cmp_mask = _mm512_cmple_epi32_mask(price_vec, taker_price_vec);
    __mmask16 match_mask = cmp_mask & active_mask & ask_mask;

    __m512i traded = _mm512_mask_min_epu32(_mm512_set1_epi32(0), match_mask, volume_vec, taker_vol_vec);

    // 更新 volume_vec 和 taker_vol_vec
    volume_vec = _mm512_mask_sub_epi32(volume_vec, match_mask, volume_vec, traded);
    taker_vol_vec = _mm512_mask_sub_epi32(taker_vol_vec, match_mask, taker_vol_vec, traded);

    // 输出撮合结果
    alignas(64) uint32_t traded_arr[16];
    alignas(64) uint32_t vol_arr[16];
    alignas(64) uint32_t remaining_taker[16];

    _mm512_store_epi32(traded_arr, traded);
    _mm512_store_epi32(vol_arr, volume_vec);
    _mm512_store_epi32(remaining_taker, taker_vol_vec);

    std::cout << "Matched slots: " << std::bitset<16>(match_mask) << "\n";
    uint32_t taker_left = taker_volume;
    for (int i = 0; i < 16; ++i) {
        if (match_mask & (1 << i)) {
            uint32_t traded = std::min(taker_left, volumes[i]);
            volumes[i] -= traded;
            taker_left -= traded;  // ✅ 关键更新
            std::cout << "Slot " << i << ": traded=" << traded
                    << ", remain_in_book=" << volumes[i]
                    << ", taker_left=" << taker_left << std::endl;
        }
    }


    return 0;
}
