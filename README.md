### High-performance Matching Engine (AVX-512 Optimized)
A low-latency order book and matching engine inspired by high-frequency trading (HFT) systems. Supports price-time priority, SIMD-based matching, and real-time market data ingestion over UDP.

### Key technology
1. AVX-512项量化更新
2. Tiered book结构 + bitmap
3. alignas(64)缓存行对齐
4. 指令流水线优化（避免分支预测）
5. UDP + 非阻塞
6. SIMD
7. CPU亲和性绑定
8. 支持回调播报Order Fill, Ack, 和 Cancel

### Performance
[detail](order/benchmark/README.md)