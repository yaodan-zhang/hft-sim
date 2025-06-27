#include "feed_handler.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sched.h>
#include <arpa/inet.h>

FeedHandler::FeedHandler(int cpu_core) : _cpu_core(cpu_core) {
    _stats.packets_received = 0;
    _stats.updates_processed = 0;
}

bool FeedHandler::is_running() const {
    return _running.load(std::memory_order_acquire);
}

FeedHandler::~FeedHandler() {
    stop();
}

void FeedHandler::stop() {
    if (!_running.load(std::memory_order_acquire)) {
        return;
    }

    _running.store(false, std::memory_order_release);

    // Close socket
    if (_fd >= 0) {
        close(_fd);
        _fd = -1;
    }

    if (_thread.joinable()) {
        _thread.join();
    }
}

void FeedHandler::bind_cpu_core() {
    if (_cpu_core < 0) {
        // Don't bind.
        return;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(_cpu_core, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "Failed to bind _thread to CPU core " << _cpu_core << ": " << strerror(errno) << std::endl;
    }
}

void FeedHandler::register_callback(MarketDataCallback cb) {
    _callback = std::move(cb);
}

void FeedHandler::start(const char* addr, int port) {
    _fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (_fd < 0) {
        perror("socket");
        return;
    }

    // Enables reuse of local addresses; useful for restarting server quickly
    int opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Setsockopt SO_REUSEADDR failed" << std::endl;
        close(_fd);
        return;
    }

    sockaddr_in saddr{};
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    inet_pton(AF_INET, addr, &saddr.sin_addr);

    if (bind(_fd, (sockaddr*)&saddr, sizeof(saddr)) < 0) {
        perror("bind");
        close(_fd);
        _fd = -1;
        return;
    }

    if (strcmp(addr, "127.0.0.1") != 0) {
        ip_mreq mreq;
        inet_pton(AF_INET, addr, &mreq.imr_multiaddr);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            perror("setsockopt");
            close(_fd);
            _fd = -1;
            return;
        }
    }

    _running.store(true, std::memory_order_release);

   _thread = std::thread(&FeedHandler::receive_loop, this);

}


void FeedHandler::receive_loop() {
    if (_cpu_core >= 0) {
        bind_cpu_core();
    }

    alignas(64) char buffer[1024];
    sockaddr_in src{};
    socklen_t len = sizeof(src);

    while (_running.load(std::memory_order_acquire)) {
        ssize_t received = recvfrom(_fd, buffer, sizeof(buffer), 0, (sockaddr*)&src, &len);
        
        // Non-stopping receive
        if (received < 0) {
            // Busy polling
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                _mm_pause();
                continue;
            } else {
                perror("recvfrom");
                continue;
            }
        }

        _stats.packets_received++;
        std::cout << "Received " << received << " bytes, ";

        size_t count = received / sizeof(MarketData);
        // std::cout << count << " MarketData entries" << std::endl;

        size_t valid_entries = 0;

        for (size_t i = 0; i < count; ++i) {
            // 64字节对齐的行情数据结构 (40字节有效载荷)
            // struct alignas(64) MarketData {
            //     MsgType  type;       
            //     uint32_t order_id;    
            //     uint32_t timestamp;
            //     int32_t  price;       // $0.01/unit
            //     uint32_t volume;
            //     uint8_t  side;        // Bid: 0, Ask: 1;
            // };
            const MarketData* md = reinterpret_cast<const MarketData*>(buffer + i * sizeof(MarketData));

            std::cout << "MarketData id" << md->order_id
            << " time stamp " << md->timestamp 
            << ": price=" << md->price
            << ", volume=" << md->volume 
            << ", side=" << (int)md->side
            << ", type=" << (char)md->type << std::endl;

            
            if ( // Validate ORDER ADD
                (md->type == MsgType::ORDER_ADD) && 
                (md->price > 0) && 
                (md->volume > 0) && 
                (md->side == 0 || md->side == 1)
            ) {
                _callback(*md);
                valid_entries++;
                
            } else if ( // Validate ORDER CANCEL
                md->type == MsgType::ORDER_CANCEL
            ) {
                _callback(*md);
                valid_entries++;
            } else {
                std::cout << "Skipping invalid order id" << md->order_id << std::endl;
            }
        }

        _stats.updates_processed += valid_entries;
    }
}
