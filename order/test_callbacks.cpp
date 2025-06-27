#include "order.h"
#include "orderbook.h"
#include "matching_engine.h"
#include <iostream>
#include <cassert>
#include <vector>

MatchingEngine engine;

// struct Order {
//     Order(uint32_t id, uint32_t ts, int32_t pr, uint32_t vol, Side sd)
//         : id(id), timestamp(ts), price(pr), volume(vol), side(sd) {}

//     uint32_t id;
//     uint32_t timestamp;
//     int32_t price;
//     uint32_t volume;
//     Side side;
// };

void run_basic_match_test() {
    Order ask = {1001, 10, 10005, 5, Side::ASK};
    Order bid = {1002, 11, 10010, 5, Side::BID};

    assert(engine.match(ask));
    assert(engine.match(bid));

    auto [bid_top, ask_top] = engine.order_book().get_top_of_book();
    assert(bid_top == 0 && ask_top == 0);  // 完全撮合

    std::cout << "[PASSED] Full match test.\n";
}

void run_partial_fill_test() {
    // 构造一个卖单挂单
    Order ask = {2001, 100, 10000, 10, Side::ASK};
    assert(engine.match(ask));

    // 构造一个买单
    Order bid = {2002, 101, 10000, 7, Side::BID};
    assert(engine.match(bid));

    // 尝试撤销挂剩的订单（剩余3）
    assert(engine.cancel_order(2001));

    std::cout << "[PASSED] Partial fill + cancel test.\n";
}

void run_time_priority_test() {
    // 重新定义一个on_fill来测试吃单时间优先顺序
    std::vector<FillReport> fills;
    engine.on_fill = [&](const FillReport& report) {

        fills.push_back(report);

        std::cout << "[FILL] taker_order_id=" << report.taker_order_id
                << ", maker_order_id=" << report.maker_order_id
                << ", price=" << report.traded_price 
                << ", volume=" << report.traded_volume << std::endl;
                
    };

    // 同价不同时间戳挂单
    assert(engine.match(Order{3001, 10, 10000, 5, Side::ASK}));

    assert(engine.match(Order{3002, 12, 10000, 5, Side::ASK}));

    // 买单应该先吃早来的
    assert(engine.match(Order{4001, 20, 10000, 5, Side::BID}));

    assert(fills.size() == 1);

    assert(fills[0].maker_order_id == 3001);

    std::cout << "[PASSED] Time priority test.\n";
}

int main() {
    // 设置全局撮合回调
    // struct FillReport {
    //     uint32_t taker_order_id;
    //     uint32_t maker_order_id;
    //     int32_t  traded_price;
    //     uint32_t traded_volume;
    // };
    engine.on_fill = [&](const FillReport& report) {
        std::cout << "[FILL] taker_order_id=" << report.taker_order_id
                    << ", maker_order_id=" << report.maker_order_id
                    << ", price=" << report.traded_price 
                    << ", volume=" << report.traded_volume << std::endl;
                    
    };

    // 设置全局挂单回调
    // struct AckReport {
    //     uint32_t order_id;
    //     uint64_t order_timestamp;
    //     int32_t  order_price;
    //     uint32_t remaining_volume;
    //     Side     order_side;
    // };
    engine.on_ack = [&](const AckReport& report) {
        std::cout << "[ACK] order_id=" << report.order_id
        << ", time stamp=" << report.order_timestamp
        << ", price=" << report.order_price
        << ", remaining volume=" << report.remaining_volume
        << ", side=" << static_cast<int>(report.order_side) << std::endl;
    };

    // 设置全局撤单回调
    // struct CancelReport {
    //     uint32_t order_id;
    //     uint32_t cancelled_volume;
    // };
    engine.on_cancel = [&](const CancelReport& report) {
        std::cout << "[CANCEL] order_id=" << report.order_id
                    << ", volume=" << report.cancelled_volume << std::endl;
    };

    run_basic_match_test();
    run_partial_fill_test();
    run_time_priority_test();

    std::cout << "[TEST PASSED]" << std::endl;

    return 0;
}
