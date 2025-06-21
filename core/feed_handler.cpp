#include "feed_handler.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

FeedHandler::FeedHandler(int cpu_core) : _cpu_core(cpu_core) {}

FeedHandler::~FeedHandler() {
    stop();
    if (_fd != -1) {
        close(_fd);
        _fd = -1;
    }
}

void FeedHandler::stop() {
    _running = false;
    if (_thread.joinable()) {
        _thread.join();
    }
}

void FeedHandler::bind_cpu_core() {
    if (_cpu_core < 0) {
        return;
    }
    
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(_cpu_core, &cpuset);
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        perror("Failed to bind CPU core");
    }
}

void FeedHandler::start(const char* addr, int port) {
    // Create socket file
    _fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (_fd < 0) {
        perror("socket creation failed");
        return;
    }

    sockaddr_in saddr{};
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);

    if (bind(_fd, (const sockaddr*)&saddr, sizeof(saddr)) < 0) {
        perror("bind failed");
        close(_fd);
        _fd = -1;
        return;
    }

    std::cout << "Listening on " << addr << ":" << port << std::endl;

    _running = true;
    _thread = std::thread(&FeedHandler::receive_loop, this);
}

void FeedHandler::receive_loop() {
    if (_cpu_core >= 0) {
        bind_cpu_core();
    }

    alignas(64) char buffer[1024];
    sockaddr_in src{};
    socklen_t addrlen = sizeof(src);

    while (_running) {

        ssize_t n = recvfrom(_fd, buffer, sizeof(buffer), 0, (sockaddr*)&src, &addrlen);
        
        if (n <= 0) {
            continue;
        }

        _stats.packets_received++;

        size_t count = std::min<size_t>(n / sizeof(MarketData), 16);

        if (_callback && count > 0) {
            alignas(64) int32_t prices[16] = {};
            alignas(64) uint32_t volumes[16] = {};
            __mmask16 side_mask = 0;

            int bid_pos = 0;  // even indices
            int ask_pos = 1;  // odd indices
            

            for (size_t i = 0; i < count; ++i) {
                const auto* md = reinterpret_cast<const MarketData*>(buffer + i * sizeof(MarketData));
                if (md->side == 0) {  // Bid
                    prices[bid_pos] = md->price;
                    volumes[bid_pos] = md->volume;
                    // bit remains 0 (bid)
                    bid_pos += 2;
                } else {  // Ask
                    prices[ask_pos] = md->price;
                    volumes[ask_pos] = md->volume;
                    side_mask |= (1 << ask_pos);  // set ask bit
                    ask_pos += 2;
                }
                
            }

            __m512i price_vec = _mm512_load_epi32(prices);
            __m512i volume_vec = _mm512_load_epi32(volumes);

            _callback(price_vec, volume_vec, side_mask);
            _stats.avx_ops++;
            _stats.updates_processed += count;

            std::cout << "Decoded side_mask = 0x" << std::hex << side_mask << std::dec << std::endl;
    
            for (int i = 0; i < 16; ++i) {
                std::cout << "P[" << i << "] = " << prices[i] << ", side = " << ((side_mask >> i) & 1) << std::endl;
            }

        }
    }
}
