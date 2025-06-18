# High-Frequency Trading Simulator with AVX-512 Optimization
## Performance (compiled with -O3, benchmarked on an Intel Ice Lake)
### Order book
Update latency:   2.94 ns/op  
Snapshot latency: 1.36 ns/op
<details>
<summary><b>perf stat results</b> (click to expand)</summary>

<pre>
        17,177,219      cycles:u                                                    
        41,067,009      instructions:u            #    2.39  insn per cycle         
            30,329      L1-dcache-load-misses:u                                     

       0.007374889 seconds time elapsed

       0.005264000 seconds user
       0.002093000 seconds sys
</pre>
</details>
