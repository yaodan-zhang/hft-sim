
g++ -O1 -mavx512f -std=c++17 -march=native -pthread main.cpp feed_handler.cpp ../order/matching_engine.cpp ../order/orderbook.cpp -o exchange

g++ -O1 -std=c++17 udp_sender.cpp -o udp_sender
