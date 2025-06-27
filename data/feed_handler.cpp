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



// #include "feed_handler.h"
// #include <iostream>
// #include <unistd.h>
// #include <cstring>
// #include <netinet/in.h>
// #include <sched.h>
// #include <atomic>
// #include <bitset>
// #include <fcntl.h>  

// FeedHandler::FeedHandler(int cpu_core) : _cpu_core(cpu_core) {
//     _stats.packets_received = 0;
//     _stats.updates_processed = 0;
// }

// bool FeedHandler::is_running() const {
//     return _running.load(std::memory_order_acquire);
// }

// FeedHandler::~FeedHandler() {
//     stop();
// }

// void FeedHandler::stop() {
//     if (!_running.load(std::memory_order_acquire)) {
//         return;
//     }

//     _running.store(false, std::memory_order_release);

//     // Close socket
//     if (_fd >= 0) {
//         close(_fd);
//         _fd = -1;
//     }

//     if (_thread.joinable()) {
//         _thread.join();
//     }
// }

// void FeedHandler::bind_cpu_core() {
//     if (_cpu_core < 0) {
//         // Don't bind.
//         return;
//     }

//     // Bind to CPU #_cpu_core
//     cpu_set_t cpuset;
//     CPU_ZERO(&cpuset);
//     CPU_SET(_cpu_core, &cpuset);

//     if (pthread_setaffinity_np(_thread.native_handle(), sizeof(cpu_set_t), &cpuset) != 0) {
//         std::cerr << "Failed to bind _thread to CPU core " << _cpu_core << ": " << strerror(errno) << std::endl;
//     }
// }

// void FeedHandler::start(const char* multicast_addr, int port) {
//     // Create UDP socket, non-blocking & busy polling & bind to CPU core 2
//     _fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

//     if (_fd < 0) {
//         std::cerr << "Socket creation failed" << std::endl;
//         return;
//     }

//     // Enables reuse of local addresses; useful for restarting server quickly
//     int opt = 1;
//     if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
//         std::cerr << "Setsockopt SO_REUSEADDR failed" << std::endl;
//         close(_fd);
//         return;
//     }

//     // Define IPv4 socket address structure
//     sockaddr_in server_addr;

//     // Zero out the structure
//     memset(&server_addr, 0, sizeof(server_addr));

//     // Set address family to IPv4
//     server_addr.sin_family = AF_INET;

//     // Set port number (network byte order)
//     server_addr.sin_port = htons(port);

//     // Convert IP address from text to binary form
//     inet_pton(AF_INET, multicast_addr, &server_addr.sin_addr);

//     // Bind socket
//     if (bind(_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
//         std::cerr << "Bind failed" << std::endl;
//         close(_fd);
//         return;
//     }

//     // Join multicast group (if required)
//     if (strcmp(multicast_addr, "127.0.0.1") != 0) {
//         ip_mreq mreq;
//         inet_pton(AF_INET, multicast_addr, &mreq.imr_multiaddr);
//         mreq.imr_interface.s_addr = htonl(INADDR_ANY);
//         if (setsockopt(_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
//             std::cerr << "Failed to join multicast group" << std::endl;
//             close(_fd);
//             return;
//         }
//     }

//     _running.store(true, std::memory_order_release);

//     _thread = std::thread(&FeedHandler::receive_loop, this);
//     bind_cpu_core();

//     std::cout << "Listening on " << multicast_addr << ":" << port << std::endl;
// }

// void FeedHandler::receive_loop() {
//     char buffer[1024]; // Receive up to 16 Market Data at a time
//     sockaddr_in client_addr;
//     socklen_t client_len = sizeof(client_addr);

//     while (_running.load(std::memory_order_acquire)) {
        
//         ssize_t received = recvfrom(_fd, buffer, sizeof(buffer), 0, (sockaddr*) &client_addr, &client_len);

//         // Non-stopping receive
//         if (received < 0) {
//             // Busy polling
//             if (errno == EAGAIN || errno == EWOULDBLOCK) {
//                 _mm_pause(); // or std::this_thread::yield();
//                 continue;
//             } else {
//                 perror("recvfrom");
//                 continue;
//             }
//         }

//         _stats.packets_received++;
//         // std::cout << "Received " << received << " bytes, ";

//         size_t num_entries = received / sizeof(MarketData);
//         // std::cout << num_entries << " MarketData entries" << std::endl;

//         MarketData* data = reinterpret_cast<MarketData*>(buffer);
//         for (size_t i = 0; i < num_entries; ++i) {
//             std::cout << "MarketData[" << i << "]: price=" << data[i].price
//                       << ", volume=" << data[i].volume << ", side=" << (int)data[i].side
//                       << ", type=" << (char) data[i].type << std::endl;
//         }

//         // Parse prices and volumes data
//         alignas(64) int32_t prices[16] = {0};
//         alignas(64) uint32_t volumes[16] = {0};
//         __mmask16 side_mask = 0;

//         size_t valid_entries = 0;

//         for (size_t i = 0; i < num_entries; ++i) {
//             // Check Order Add
//             if (
//                 data[i].type != MsgType::ORDER_ADD 
//                 || data[i].volume <= 0 
//                 || (data[i].side != 0 && data[i].side != 1
//                 || (data[i].price <= 0
//                 ))) {
//                 std::cout << "Skipping invalid entry[" << i << "]: type=" << (char)data[i].type
//                           << ", side=" << (int)data[i].side << ", volume=" << data[i].volume << std::endl;
//                 continue;
//             }

//             prices[i] = data[i].price;
//             volumes[i] = data[i].volume;

//             if (data[i].side == 1) {
//                 side_mask |= (1 << i);
//             }

//             valid_entries++;
//         }

//         if (_callback && valid_entries > 0) {
            
//             _callback(_mm512_load_si512(prices), _mm512_load_si512(volumes), side_mask);

//             _stats.updates_processed += valid_entries;
//         }
//     }
// }