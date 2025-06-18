// benchmark.cpp
#include "orderbook.hpp"
#include <chrono>
#include <iostream>

int main() {
    OrderBook book;
    alignas(64) int32_t price_deltas[TIER_SIZE] = {1};
    alignas(64) uint32_t volume_deltas[TIER_SIZE] = {100};

    // Latency test
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1'000'000; ++i) {
        book.update(price_deltas, volume_deltas);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double latency = std::chrono::duration<double, std::nano>(end - start).count() / 1'000'000;
    std::cout << "Update latency: " << latency << " ns/op\n";

    // Snapshot test
    int32_t snap_prices[TIER_SIZE];
    uint32_t snap_volumes[TIER_SIZE];
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1'000'000; ++i) {
        book.get_snapshot(snap_prices, snap_volumes);
    }
    end = std::chrono::high_resolution_clock::now();
    latency = std::chrono::duration<double, std::nano>(end - start).count() / 1'000'000;
    std::cout << "Snapshot latency: " << latency << " ns/op\n";
}