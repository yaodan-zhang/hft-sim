#include "feed_handler.h"
#include "orderbook.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    OrderBook book;
    FeedHandler feed(2); // Bind to CPU core 2 / 64

    feed.register_callback([&book](const __m512i& prices, const __m512i& volumes, __mmask16 side_mask) {
        book.update(prices, volumes, side_mask);
        
        // Broadcast every update
        int32_t bid, ask;
        book.get_top_of_book(bid, ask);
        std::cout << "Top of Book - Bid: " << bid << " Ask: " << ask << std::endl;      
    });

    feed.start("127.0.0.1", 50000);
    
    // Launch statistics thread
    std::thread stats_thread([&feed]() {
        while (true) {
            auto const & stats = feed.stats();
            std::cout << "RX: " << stats.packets_received 
                      << " Updates: " << stats.updates_processed
                      << " AVX ops: " << stats.avx_ops << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
    });

    stats_thread.join();
    return 0;
}