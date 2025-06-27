#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <cassert>
#include "market_data.h"

// // 市场数据类型 (兼容ITCH 5.0和Binary协议)
// enum class MsgType : uint8_t {
//     ORDER_ADD    = 'A',
//     ORDER_CANCEL = 'X',
// };

// // 64字节对齐的行情数据结构 (40字节有效载荷)
// struct alignas(64) MarketData {
//     MsgType  type;       
//     uint32_t order_id;    
//     uint32_t timestamp;
//     int32_t  price;       // $0.01/unit
//     uint32_t volume;
//     uint8_t  side;        // Bid: 0, Ask: 1;
// };

int main() {
    // Create socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(50000); // Listen on port 50000
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    std::vector<MarketData> packets(16);

    // Create the first MarketData packet
    packets[0]  = {MsgType::ORDER_ADD, 1, 0, 1000, 5, 0};
    packets[1]  = {MsgType::ORDER_ADD, 2, 2, 995,  5, 0};
    packets[2]  = {MsgType::ORDER_ADD, 3, 3, 1010, 5, 1};
    packets[3]  = {MsgType::ORDER_ADD, 4, 5, 1005, 5, 1};

    if (sendto(sock, packets.data(), 4 * sizeof(MarketData), 0, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
    } 

    sleep(1);

    // Create the second MarketData packet
    packets[0]  = {MsgType::ORDER_ADD, 5, 10, 1005, 5, 0};
    packets[1]  = {MsgType::ORDER_ADD, 6, 12, 1010,  5, 0};
    packets[2]  = {MsgType::ORDER_ADD, 7, 13, 1010, 5, 0};
    packets[3]  = {MsgType::ORDER_CANCEL, 7, 16, 0, 0, 0};

    if (sendto(sock, packets.data(), 4 * sizeof(MarketData), 0, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
    } 

    sleep(1);

    // Create the third packet
    packets[0]  = {MsgType::ORDER_ADD, 8, 18, 0, 5, 0};  // invalid, price == 0
    packets[1]  = {MsgType::ORDER_ADD, 9, 24, 1010,  0, 1}; // invalid, volume == 0
    packets[2]  = {MsgType::ORDER_ADD, 10, 26, 1000, 5, 1}; // Top of book should be bid 995, ask 0 (no ask)

    if (sendto(sock, packets.data(), 3 * sizeof(MarketData), 0, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto");
    } 

    sleep(1);

    close(sock);
    return 0;
}