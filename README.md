# High-Frequency Trading Simulator with AVX-512 Optimization
## Performance (compiled with -O3, benchmarked on an Intel Ice Lake)
### Order book
Update latency:   2.91 ns/op  
Snapshot latency: 1.35 ns/op
<details>
<summary><b>perf stat results</b> (click to expand)</summary>

<pre>
    16,658,726      cycles:u                                                    
    45,065,932      instructions:u   # 2.71  insn per cycle         
        29,725      L1-dcache-load-misses:u                                     

    0.007780069 seconds time elapsed

    0.005559000 seconds user
    0.002228000 seconds sys
</pre>
</details>

## How to compile and profile
g++ -mavx512f -O3 -Wall -std=c++17 test_orderbook.cpp ../../core/orderbook.cpp -o hft_test  
./hft_test

g++ -mavx512f -mavx512dq -O3 orderbook.cpp benchmark.cpp -o benchmark  
perf stat -e cycles,instructions,L1-dcache-load-misses ./benchmark