#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdint>
#include <cassert>
#include "market_data.h"

// Send market data via UDP packets. 
// Listen on port 50000.
int main(int argc, char *argv[]) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(50000);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    alignas(64) MarketData packet[16];  // Send 16 MarketData entries

    for (int i = 0; i < 16; ++i) {
        MarketData& md = packet[i];
        std::memset(&md, 0, sizeof(md));

        md.type = MsgType::ORDER_ADD;
        md.timestamp = 1000000000 + i * 1000;
        md.order_id = 1000 + i;
        md.price = 100000 + i * 100;  // 1000.00, 1001.00, 1002.00, ...
        md.volume = 10 + i; // 10, 11, 12, ...

        // alternate between bid (even i) and ask (odd i)
        md.side = (i % 2 == 0) ? 0 : 1;
    }

    ssize_t total_bytes = sendto(fd, packet, sizeof(packet), 0, (sockaddr*)&dest, sizeof(dest));
    
    if (total_bytes == sizeof(packet)) {
        std::cout << "Sent " << sizeof(packet) << " bytes (16 MarketData packets)" << std::endl;
    } else {
        perror("sendto");
    }

    close(fd);
    return 0;
}
