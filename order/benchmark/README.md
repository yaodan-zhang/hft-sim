### Compile:   
g++ -O3 -mavx512f -std=c++2a benchmark_match.cpp ../matching_engine.cpp ../orderbook.cpp ../match_tier_avx512.cpp -o benchmark 

### Profiling:  
perf stat ./benchmark  
====== PERFORMANCE BENCHMARK ======  
[Matching] Orders: 100000, Total time: 24675672 ns, Avg latency: 246 ns, Throughput: 4.05257e+06 ops/sec  
[TopOfBook] Calls: 100000, Total time: 987001 ns, Avg latency: 9 ns  
[Cancel] Orders: 100000, Total time: 1853386 ns, Avg latency: 18 ns, Throughput: 5.39553e+07 ops/sec  

 Performance counter stats for './benchmark':

             29.18 msec task-clock:u              #    0.942 CPUs utilized          
                 0      context-switches:u        #    0.000 /sec                   
                 0      cpu-migrations:u          #    0.000 /sec                   
               694      page-faults:u             #   23.785 K/sec                  
        93,540,622      cycles:u                  #    3.206 GHz                    
       312,683,169      instructions:u            #    3.34  insn per cycle         
        62,327,380      branches:u                #    2.136 G/sec                  
           421,471      branch-misses:u           #    0.68% of all branches        
       457,433,665      slots:u                   #   15.677 G/sec                  
       303,161,919      topdown-retiring:u        #     66.3% retiring              
        75,342,015      topdown-bad-spec:u        #     16.5% bad speculation       
        44,846,437      topdown-fe-bound:u        #      9.8% frontend bound        
        34,083,292      topdown-be-bound:u        #      7.5% backend bound         

       0.030991091 seconds time elapsed

       0.028273000 seconds user
       0.001949000 seconds sys