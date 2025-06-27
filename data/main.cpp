#include "feed_handler.h"
#include "../order/matching_engine.h"
#include "../order/orderbook.h"
#include "../order/order.h"
#include "../order/match_tier_avx512.h"
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

    // Register on_fill, on_ack, on_cancel
    engine.on_fill = [&](const FillReport& report) {
        std::cout << "[FILL] taker_order_id=" << report.taker_order_id
                    << ", maker_order_id=" << report.maker_order_id
                    << ", price=" << report.traded_price 
                    << ", volume=" << report.traded_volume << std::endl;
                    
    };

    engine.on_ack = [&](const AckReport& report) {
        std::cout << "[ACK] order_id=" << report.order_id
        << ", time stamp=" << report.order_timestamp
        << ", price=" << report.order_price
        << ", remaining volume=" << report.remaining_volume
        << ", side=" << static_cast<int>(report.order_side) << std::endl;
    };

    engine.on_cancel = [&](const CancelReport& report) {
        std::cout << "[CANCEL] order_id=" << report.order_id
                    << ", volume=" << report.cancelled_volume << std::endl;
    };

    OrderBook& book = engine.order_book();
    FeedHandler feed(2);

    feed.register_callback([&engine, &book](const MarketData& market_data) {
        // enum class MsgType : uint8_t {
        //     ORDER_ADD    = 'A',
        //     ORDER_CANCEL = 'X',
        // };

        // struct alignas(64) MarketData {
        //     MsgType  type;       
        //     uint32_t order_id;    
        //     uint32_t timestamp;
        //     int32_t  price;       // $0.01/unit
        //     uint32_t volume;
        //     uint8_t  side;        // Bid: 0, Ask: 1;
        // };
        
        // Construct order
        // struct Order {
        //     uint32_t id;
        //     uint32_t timestamp;
        //     int32_t price;
        //     uint32_t volume;
        //     Side side;
        // };
        Order o = {
            .id = market_data.order_id, 
            .timestamp = market_data.timestamp, 
            .price = market_data.price, 
            .volume = market_data.volume, 
            .side = market_data.side == 1? Side::ASK : Side::BID
        };
        
        // EXECUTE ADD
        if (market_data.type == MsgType::ORDER_ADD) {
            if (!engine.match(o)) {
                std::cout << "[ERROR ADD ORDER] Order id " << o.id << std::endl;
            }
        }

        // EXECUTE CANCEL
        if (market_data.type == MsgType::ORDER_CANCEL) {
            if(!engine.cancel_order(o.id)) {
                std::cout << "[ERROR CANCEL ORDER] Order id " << o.id << std::endl;
            }
        }

        // Print new best bid and ask
        auto [bid, ask] = book.get_top_of_book();
        std::cout << "[TOP OF BOOK] Bid: " << bid << ", Ask: " << ask << std::endl;
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

    std::cout << "Engine terminated." << std::endl;
    return 0;
}