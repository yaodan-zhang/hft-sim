#pragma once
#include "market_data.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <atomic>
#include <functional>

// 性能统计结构
struct alignas(64) FeedStats {
    std::atomic<uint64_t> packets_received{0};
    std::atomic<uint64_t> updates_processed{0};
    std::atomic<uint64_t> avx_ops{0};
};

class FeedHandler {
public:
    explicit FeedHandler(int cpu_core = -1); // Default not to bind
    ~FeedHandler();

    void start(const char* multicast_addr, int port);
    void stop();

    void register_callback(std::function<void(const __m512i& prices, const __m512i& volumes, __mmask16 side_mask)> cb) {
        _callback = std::move(cb);
    }

    const FeedStats& stats() const { return _stats; }

private:
    void receive_loop();
    void bind_cpu_core();

    int _fd = -1;
    int _cpu_core;

    std::atomic<bool> _running{false};
    std::thread _thread;
    FeedStats _stats;

    std::function<void(const __m512i& prices, const __m512i& volumes, __mmask16 side_mask)> _callback;

};