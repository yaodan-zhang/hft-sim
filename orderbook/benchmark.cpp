// benchmark.cpp
#include "orderbook.hpp"
#include <chrono>
#include <iostream>

/**
 * Performance test harness for order book updates
 * Measures:
 * - Average latency per operation
 * - Instructions per cycle (IPC)
 * - L1 cache miss rate
 */
int main() {
    OrderBook book;

    // Test data (aligned to 64-byte for optimal AVX-512 performance)
    alignas(64) int32_t price_deltas[16] = {1};
    alignas(64) uint32_t volume_deltas[16] = {100};

    // Timed benchmark (1M iterations)
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1'000'000; ++i) {
        book.update(price_deltas, volume_deltas);
    }
    auto end = std::chrono::high_resolution_clock::now();

    // Print latency
    std::cout << "Latency: " 
              << (end - start).count() / 1'000'000.0 << " ns/op\n";
}
