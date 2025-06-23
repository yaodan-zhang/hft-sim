#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <cassert>
#include "market_data.h"

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(50000);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    std::vector<MarketData> packets(16);

    // Create the first MarketData packet
    packets[0]  = {MsgType::ORDER_ADD, 0, 0, 1, 100000, 5,};
    packets[1]  = {MsgType::ORDER_ADD, 1, 0, 2, 99995,  5,};
    packets[2]  = {MsgType::ORDER_ADD, 0, 0, 3, 100010, 5,};
    packets[3]  = {MsgType::ORDER_ADD, 1, 0, 4, 100005, 5,};
    packets[4]  = {MsgType::ORDER_ADD, 0, 0, 5, 100020, 5,};
    packets[5]  = {MsgType::ORDER_ADD, 1, 0, 6, 100015, 5,};
    packets[6]  = {MsgType::ORDER_ADD, 0, 0, 7, 100030, 5,};
    packets[7]  = {MsgType::ORDER_ADD, 1, 0, 8, 100025, 5,};
    packets[8]  = {MsgType::ORDER_ADD, 0, 0, 9, 100040, 5,};
    packets[9]  = {MsgType::ORDER_ADD, 1, 0, 10, 100035, 5,};
    packets[10] = {MsgType::ORDER_ADD, 0, 0, 11, 100050, 5,};
    packets[11] = {MsgType::ORDER_ADD, 1, 0, 12, 100045, 5,};
    packets[12] = {MsgType::ORDER_ADD, 0, 0, 13, 100060, 5,};
    packets[13] = {MsgType::ORDER_ADD, 1, 0, 14, 100055, 5,};
    packets[14] = {MsgType::ORDER_ADD, 0, 0, 15, 100070, 5,};
    packets[15] = {MsgType::ORDER_ADD, 1, 0, 16, 100065, 5,};

    ssize_t sent = sendto(sock, packets.data(), packets.size() * sizeof(MarketData), 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (sent < 0) {
        std::cerr << "Send failed" << std::endl;
    } else {
        std::cout << "Sent " << sent << " bytes (a " << packets.size() << " MarketData packet)" << std::endl;
    }

    sleep(1);

    // Create the second MarketData packet
    packets[0]  = {MsgType::ORDER_ADD, 0, 0, 17, 100040, 5,}; 
    packets[1]  = {MsgType::ORDER_ADD, 1, 0, 18, 99990,  5,};
    packets[2]  = {MsgType::ORDER_ADD, 0, 0, 19, 100050, 5,};
    packets[3]  = {MsgType::ORDER_ADD, 1, 0, 20, 99990,  5,};
    packets[4]  = {MsgType::ORDER_ADD, 0, 0, 21, 100060, 5,};
    packets[5]  = {MsgType::ORDER_ADD, 1, 0, 22, 99990,  5,};
    packets[6]  = {MsgType::ORDER_ADD, 0, 0, 23, 100070, 5,};
    packets[7]  = {MsgType::ORDER_ADD, 1, 0, 24, 99990,  5,};
    packets[8]  = {MsgType::ORDER_ADD, 0, 0, 25, 100080, 5,};
    packets[9]  = {MsgType::ORDER_ADD, 1, 0, 26, 99990, 5,};
    packets[10] = {MsgType::ORDER_ADD, 0, 0, 27, 100090, 5,};
    packets[11] = {MsgType::ORDER_ADD, 1, 0, 28, 100085, 5,};
    packets[12] = {MsgType::ORDER_ADD, 0, 0, 29, 100100, 5,};
    packets[13] = {MsgType::ORDER_ADD, 1, 0, 30, 100095, 5,};
    packets[14] = {MsgType::ORDER_ADD, 0, 0, 31, 100110, 5,};
    packets[15] = {MsgType::ORDER_ADD, 1, 0, 32, 100105, 5,};

    sent = sendto(sock, packets.data(), packets.size() * sizeof(MarketData), 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (sent < 0) {
        std::cerr << "Send failed" << std::endl;
    } else {
        std::cout << "Sent " << sent << " bytes (a " << packets.size() << " MarketData packet)" << std::endl;
    }

    sleep(1);

    // Create the third packet
    packets[0]  = {MsgType::ORDER_ADD, 0, 0, 33, 100105, 5,}; 
    packets[1]  = {MsgType::ORDER_ADD, 1, 0, 34, 100000,  5,};
    packets[2]  = {MsgType::ORDER_ADD, 0, 0, 35, 100105, 5,};
    packets[3]  = {MsgType::ORDER_ADD, 1, 0, 36, 100000,  5,};
    packets[4]  = {MsgType::ORDER_ADD, 0, 0, 37, 100105, 5,};
    packets[5]  = {MsgType::ORDER_ADD, 1, 0, 38, 100000,  5,};
    packets[6]  = {MsgType::ORDER_ADD, 0, 0, 39, 100000, 5,};
    packets[7]  = {MsgType::ORDER_ADD, 1, 0, 40, 100000,  5,};
    packets[8]  = {MsgType::ORDER_ADD, 0, 0, 41, 100000, 5,};
    packets[9]  = {MsgType::ORDER_ADD, 1, 0, 42, 99990, 0,};    // invalid
    packets[10] = {MsgType::ORDER_ADD, 0, 0, 43, 100090, 0,};   // invalid
    packets[11] = {MsgType::ORDER_ADD, 1, 0, 44, 100085, 0,};   // invalid
    packets[12] = {MsgType::ORDER_ADD, 0, 0, 45, -100100, 5,};  // invalid
    packets[13] = {MsgType::ORDER_ADD, 1, 0, 46, 0, 0,};        // invalid
    packets[14] = {MsgType::ORDER_ADD, 0, 0, 47, -100110, 10,}; // invalid
    packets[15] = {MsgType::ORDER_ADD, 1, 0, 48, 100105, 0,};   // invalid

    sent = sendto(sock, packets.data(), packets.size() * sizeof(MarketData), 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (sent < 0) {
        std::cerr << "Send failed" << std::endl;
    } else {
        std::cout << "Sent " << sent << " bytes (a " << packets.size() << " MarketData packet)" << std::endl;
    }
    
    close(sock);
    return 0;
}