#include "matching_engine.h"
#include <iostream>
#include <cassert>

int main() {
    MatchingEngine engine;

    bool global_fill_triggered = false;
    bool global_cancel_triggered = false;
    bool local_fill_triggered = false;

    // 设置全局撮合回调
    engine.on_fill = [&](const FillReport& report) {
        global_fill_triggered = true;
        std::cout << "[FILL] taker_order_id=" << report.taker_order_id
                  << ", maker_order_id=" << report.maker_order_id
                  << ", volume=" << report.volume
                  << ", price=" << report.price << std::endl;
    };

    // 设置全局撤单回调
    engine.on_cancel = [&](const CancelReport& report) {
        global_cancel_triggered = true;
        std::cout << "[CANCEL] order_id=" << report.order_id
                  << ", volume=" << report.cancelled_volume << std::endl;
    };

    // 构造一个卖单挂单
    Order ask = {1001, 1, 10000, 10, Side::ASK};
    bool result = engine.match(ask, nullptr);  // 无局部回调
    assert(result);

    // 构造一个买单，并设置局部撮合回调
    Order bid = {2002, 2, 10000, 7, Side::BID};
    result = engine.match(bid, [&](uint32_t taker, uint32_t maker, uint32_t vol) {
        local_fill_triggered = true;
        std::cout << "[LOCAL FILL] taker=" << taker
                  << ", maker=" << maker
                  << ", vol=" << vol << std::endl;

        assert(taker == 2002);
        assert(maker == 1001);
        assert(vol == 7);

        // 手动调用全局撮合回调
        if (engine.on_fill) {
            engine.on_fill(FillReport{taker, maker, vol, bid.price});
        }
    });
    assert(result);

    // 撤销剩余挂单
    result = engine.cancel_order(1001);
    assert(result);

    // 验证全部回调是否触发
    assert(global_fill_triggered);
    assert(global_cancel_triggered);
    assert(local_fill_triggered);

    std::cout << "[TEST PASSED] All callbacks were successfully triggered." << std::endl;
    return 0;
}
