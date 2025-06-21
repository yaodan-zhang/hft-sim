Feed Handler
g++ -O3 -mavx512f -mavx512cd -march=native -pthread main.cpp feed_handler.cpp orderbook.cpp -o hft_feed  
./hft_feed

UDP socket sender
g++ -O2 -std=c++17 udp_sender.cpp -o udp_sender  
./udp_sender 