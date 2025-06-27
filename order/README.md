### Compile
g++ -O2 -std=c++2a -march=native test_callbacks.cpp matching_engine.cpp orderbook.cpp match_tier_avx512.cpp -o test_callbacks