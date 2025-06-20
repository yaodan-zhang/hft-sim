// tests/unit/test_orderbook.cpp
#include <immintrin.h>
#include <thread>
#include "../../core/orderbook.hpp"
#include "hft_test.h"

alignas(64) int32_t test_prices[TIER_SIZE];
alignas(64) uint32_t test_volumes[TIER_SIZE];
OrderBook book;

TEST_CASE(BasicUpdate) {
    for (size_t i = 0; i < BATCH_SIZE - 1; ++i) {
        book.update(test_prices, test_volumes);
    }
    uint64_t v1 = book.update(test_prices, test_volumes);  // Batch size reaches, seq no updates + 1
    uint64_t v2 = book.update(test_prices, test_volumes);  // seq no stays the same
    EXPECT_GT(v1, 0);
    EXPECT_EQ(v2, v1);
}

TEST_CASE(AlignmentCheck) {
    int32_t* unaligned = test_prices + 1; // not 64-byte aligned
    EXPECT_THROW(book.update(unaligned, test_volumes), std::runtime_error);
}

TEST_CASE(SnapshotConsistency) {
    OrderBook fresh_book; 
    int k = 13; // Add price and volume deltas k times.
    for (int i = 0; i < k; ++i) {
        fresh_book.update(test_prices, test_volumes);
    }

    alignas(64) int32_t snap_prices[TIER_SIZE];
    alignas(64) uint32_t snap_volumes[TIER_SIZE];
    uint64_t ver = fresh_book.get_snapshot(snap_prices, snap_volumes);
    
    EXPECT_GT(ver, 0);
    for (size_t i = 0; i < TIER_SIZE; ++i) {
        EXPECT_EQ(snap_prices[i], test_prices[i] * k);
        EXPECT_EQ(snap_volumes[i], test_volumes[i] * k);
    }
}

TEST_CASE(PerformanceBenchmark) {
    const int iterations = 1'000'000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        book.update(test_prices, test_volumes);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double latency = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
    std::cout << "Update latency: " << latency << " ns/op\n";
    EXPECT_LT(latency, 3.0); // Passes if latency < 3ns
}

int main() {
    // Initialize test input arrays: prices and volumes
    for (size_t i = 0; i < TIER_SIZE; ++i) {
        test_prices[i] = i * 100;
        test_volumes[i] = i * 10;
    }
    return TestRunner::run_all();
}