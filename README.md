### High-performance Matching Engine (AVX-512 Optimized)
A low-latency order book and matching engine inspired by high-frequency trading (HFT) systems. Supports price-time priority, SIMD-based matching, and real-time market data ingestion over UDP.

### Key technology
1. AVX-512 vectorized updates
2. Tiered book structure + bitmap
3. alignas(64) cache line alignment
4. Avoid branch predictions
5. UDP-based non-blocking I/O
6. SIMD
7. CPU affinity binding
8. Callback-based reporting for Order Fill, Ack, and Cancel

### Performance
[detail](order/benchmark/README.md)