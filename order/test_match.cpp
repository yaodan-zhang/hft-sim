#include "matching_engine.h"
#include <iostream>
#include <cassert>
#include <immintrin.h>
#include "order.h"
#include "order_events.h"
#include "tier_mapper.h"

void test_basic_match() {
    MatchingEngine engine;

    engine.on_ack = [](const OrderAck& ack) {
        std::cout << "[ACK] id=" << ack.order_id << " px=" << ack.price << " vol=" << ack.volume << std::endl;
    };
    engine.on_fill = [](const OrderFill& fill) {
        std::cout << "[FILL] taker=" << fill.order_id << " maker=" << fill.matched_order_id
                  << " px=" << fill.price << " vol=" << fill.volume << std::endl;
    };
    engine.on_cancel = [](const OrderCancel& cancel) {
        std::cout << "[CANCEL] id=" << cancel.order_id << " px=" << cancel.price << " vol=" << cancel.canceled_volume << std::endl;
    };

    for (int i = 0; i < 16; ++i) {
        Order order;
        order.price = 100000 + i * 100;
        order.volume = 10;
        order.side = Side::ASK;
        order.order_id = 1000 + i;
        order.timestamp = i;
        engine.match(order);
    }

    for (int i = 0; i < 16; ++i) {
        Order order;
        order.price = 100000 + (15 - i) * 100;
        order.volume = 10;
        order.side = Side::BID;
        order.order_id = 2000 + i;
        order.timestamp = 100 + i;
        engine.match(order);
    }

    auto [bid, ask] = engine.order_book().get_top_of_book();
    std::cout << "[Basic Match] Top of Book - Bid: " << bid << " Ask: " << ask << std::endl;
}

void test_partial_match() {
    MatchingEngine engine;

    engine.on_fill = [](const OrderFill& fill) {
        std::cout << "[FILL] taker=" << fill.order_id << " maker=" << fill.matched_order_id
                  << " px=" << fill.price << " vol=" << fill.volume << std::endl;
    };

    for (int i = 0; i < 8; ++i) {
        Order order;
        order.price = 100000 + i * 100;
        order.volume = 5;
        order.side = Side::ASK;
        order.order_id = 3000 + i;
        order.timestamp = i;
        engine.match(order);
    }

    for (int i = 0; i < 8; ++i) {
        Order order;
        order.price = 100000 + i * 100;
        order.volume = 10;
        order.side = Side::BID;
        order.order_id = 4000 + i;
        order.timestamp = 100 + i;
        engine.match(order);
    }

    auto [bid, ask] = engine.order_book().get_top_of_book();
    std::cout << "[Partial Match] Top of Book - Bid: " << bid << " Ask: " << ask << std::endl;
}

void test_order_prioritization() {
    MatchingEngine engine;

    engine.match(Order{.price = 100000, .volume = 10, .side = Side::ASK, .order_id = 100, .timestamp = 10});
    engine.match(Order{.price = 100000, .volume = 10, .side = Side::ASK, .order_id = 101, .timestamp = 5});

    engine.match(Order{.price = 100000, .volume = 10, .side = Side::BID, .order_id = 200, .timestamp = 20});

    auto [bid, ask] = engine.order_book().get_top_of_book();
    std::cout << "[Time Priority] Top of Book - Bid: " << bid << " Ask: " << ask << std::endl;
}

void test_order_cancellation() {
    MatchingEngine engine;

    engine.match(Order{.price = 100000, .volume = 10, .side = Side::ASK, .order_id = 6000, .timestamp = 0});
    auto [bid1, ask1] = engine.order_book().get_top_of_book();
    assert(ask1 == 100000);

    bool cancel_result = engine.cancel_order(6000);
    assert(cancel_result);

    auto [bid2, ask2] = engine.order_book().get_top_of_book();
    assert(ask2 != 100000);

    std::cout << "[Cancel Order] Top of Book - Bid: " << bid2 << " Ask: " << ask2 << std::endl;
}

void test_residual_volume_tracking() {
    MatchingEngine engine;

    engine.match(Order{.price = 100000, .volume = 10, .side = Side::ASK, .order_id = 7000, .timestamp = 1});
    engine.match(Order{.price = 100000, .volume = 5, .side = Side::BID, .order_id = 7001, .timestamp = 2});

    auto [bid1, ask1] = engine.order_book().get_top_of_book();
    assert(ask1 == 100000);

    engine.match(Order{.price = 100000, .volume = 5, .side = Side::BID, .order_id = 7002, .timestamp = 3});

    auto [bid2, ask2] = engine.order_book().get_top_of_book();
    assert(ask2 != 100000);

    std::cout << "[Residual Volume] Top of Book - Bid: " << bid2 << " Ask: " << ask2 << std::endl;
}

int main() {
    test_basic_match();
    test_partial_match();
    test_order_prioritization();
    test_order_cancellation();
    test_residual_volume_tracking();
    return 0;
}
