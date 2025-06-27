#include "../order.h"
#include "../orderbook.h"
#include "../matching_engine.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>

using Clock = std::chrono::high_resolution_clock;

MatchingEngine engine;

uint64_t now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        Clock::now().time_since_epoch()
    ).count();
}

void benchmark_matching(int num_orders) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> price_dist(1000, 2000);
    std::uniform_int_distribution<int> volume_dist(1, 10);

    uint64_t start_time = now();

    for (int i = 0; i < num_orders; ++i) {
        Order order{
            .id = static_cast<uint32_t>(i),
            .timestamp = static_cast<uint32_t>(i),
            .price = price_dist(rng),
            .volume = volume_dist(rng),
            .side = (i % 2 == 0) ? Side::BID : Side::ASK
        };
        engine.match(order);
    }

    uint64_t end_time = now();
    uint64_t duration_ns = end_time - start_time;

    std::cout << "[Matching] Orders: " << num_orders
              << ", Total time: " << duration_ns << " ns"
              << ", Avg latency: " << duration_ns / num_orders << " ns"
              << ", Throughput: " << (1e9 * num_orders / duration_ns) << " ops/sec"
              << std::endl;
}

void benchmark_cancel(int num_orders) {
    // 假设前面所有订单都已经成功挂进去了
    uint64_t start_time = now();
    for (int i = 0; i < num_orders; ++i) {
        engine.cancel_order(i);
    }
    uint64_t end_time = now();
    uint64_t duration_ns = end_time - start_time;

    std::cout << "[Cancel] Orders: " << num_orders
              << ", Total time: " << duration_ns << " ns"
              << ", Avg latency: " << duration_ns / num_orders << " ns"
              << ", Throughput: " << (1e9 * num_orders / duration_ns) << " ops/sec"
              << std::endl;
}

void benchmark_top_of_book(int iterations) {
    uint64_t start_time = now();
    for (int i = 0; i < iterations; ++i) {
        auto [bid, ask] = engine.order_book().get_top_of_book();
        (void)bid;
        (void)ask;
    }
    uint64_t end_time = now();

    std::cout << "[TopOfBook] Calls: " << iterations
              << ", Total time: " << end_time - start_time << " ns"
              << ", Avg latency: " << (end_time - start_time) / iterations << " ns"
              << std::endl;
}

int main() {
    // engine.on_fill = [](const FillReport& f) {};
    // engine.on_ack  = [](const AckReport& a) {};
    // engine.on_cancel = [](const CancelReport& c) {};

    constexpr int NUM_ORDERS = 100000;

    std::cout << "====== PERFORMANCE BENCHMARK ======\n";
    benchmark_matching(NUM_ORDERS);     // 撮合性能
    benchmark_top_of_book(100000);      // 查询 Top-of-Book 性能
    benchmark_cancel(NUM_ORDERS);       // 撤单性能
    return 0;
}
