#include "feed_handler.h"
#include "../order/matching_engine.h"
#include "../order/orderbook.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

std::atomic<bool> signal_received{false};

// Handle exit signals
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
        signal_received.store(true, std::memory_order_release);
    }
}

int main() {
    // Register signal handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    MatchingEngine engine;
    OrderBook& book = engine.order_book();
    FeedHandler feed(2);

    feed.register_callback([&engine, &book](const __m512i& prices, const __m512i& volumes, __mmask16 side_mask) {
        
        alignas(64) uint32_t updated_volumes[16];
        _mm512_store_epi32(updated_volumes, volumes);

        // 执行匹配（包含订单簿更新）
        __mmask16 residual_bid_mask = engine.match(prices, updated_volumes, side_mask);
    
        // Print new best bid and ask
        auto [bid, ask] = book.get_top_of_book();
        std::cout << "Top of Book - Bid: " << bid << ", Ask: " << ask << std::endl;

    });

    feed.start("127.0.0.1", 50000);

    std::thread stats_thread([&feed]() {
        while (feed.is_running()) {
            const auto& stats = feed.stats();
            std::cout << "RX: " << stats.packets_received
                      << ", Updates: " << stats.updates_processed << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
    });

    while (!signal_received.load(std::memory_order_acquire) && feed.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    feed.stop();

    if (stats_thread.joinable()) {
        stats_thread.join();
    }

    std::cout << "Program terminated." << std::endl;
    return 0;
}