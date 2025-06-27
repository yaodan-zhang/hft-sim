#pragma once
#include "market_data.h"
#include <functional>
#include <thread>
#include <atomic>
#include <netinet/in.h>

struct alignas(64) FeedStats {
    std::atomic<uint64_t> packets_received{0};  // < Number of UDP packets received from the multicast feed.
    std::atomic<uint64_t> updates_processed{0}; // < Number of valid market data entries processed.
};

class FeedHandler {
public:
    using MarketDataCallback = std::function<void(const MarketData&)>;

    explicit FeedHandler(int cpu_core = -1);

    ~FeedHandler();

    void start(const char* addr, int port);

    void stop();

    bool is_running() const;

    void register_callback(MarketDataCallback cb);

    void bind_cpu_core();
    
    void receive_loop();

    const FeedStats& stats() const { return _stats; }

private:
    int _fd = -1;
    int _cpu_core = -1;

    std::atomic<bool> _running{false};

    std::thread _thread;

    FeedStats _stats;   
    
    MarketDataCallback _callback;
};
