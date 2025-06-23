#pragma once
#include "market_data.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <functional>

/**
 * @struct FeedStats
 * @brief Tracks statistics for market data packet processing.
 *
 * Maintains atomic counters for received packets, processed market data updates,
 * and AVX-512 operations. Aligned to 64 bytes for cache efficiency to minimize
 * contention in multithreaded environments.
 */
struct alignas(64) FeedStats {
    std::atomic<uint64_t> packets_received{0};  ///< Number of UDP packets received from the multicast feed.
    std::atomic<uint64_t> updates_processed{0}; ///< Number of valid market data entries processed.
};

/**
 * @class FeedHandler
 * @brief Receives and processes market data packets from a UDP multicast feed.
 *
 * Listens for market data packets on a specified multicast address and port, decodes
 * them into prices, volumes, and side masks, and invokes a registered callback to process
 * the data using AVX-512 instructions. Supports optional CPU core binding for performance
 * optimization and tracks processing statistics via FeedStats.
 */
class FeedHandler {
public:
    /**
     * @brief Constructs a FeedHandler with optional CPU core binding.
     *
     * Initializes the feed handler with an empty statistics structure and sets the CPU core
     * for thread binding. If cpu_core is -1, the processing thread is not bound to any core.
     *
     * @param cpu_core The CPU core to bind the receiving thread to (default: -1, no binding).
     */
    explicit FeedHandler(int cpu_core = -1);

    /**
     * @brief Destroys the FeedHandler, ensuring proper cleanup.
     *
     * Stops the receiving thread, closes the UDP socket, and joins the thread to ensure
     * resources are released properly.
     */
    ~FeedHandler();

    /**
     * @brief Starts the feed handler to receive market data packets.
     *
     * Creates a UDP socket, enables address reuse, binds to the specified multicast address
     * and port, and joins the multicast group (unless the address is 127.0.0.1). Launches
     * a thread to run the receive loop and binds it to the specified CPU core if applicable.
     *
     * @param multicast_addr The multicast address to listen on (e.g., "239.0.0.1" or "127.0.0.1").
     * @param port The port number to bind to (e.g., 50000).
     */
    void start(const char* multicast_addr, int port);

    /**
     * @brief Stops the feed handler and releases resources.
     *
     * Sets the running flag to false, closes the UDP socket, and joins the receiving thread
     * to ensure a clean shutdown.
     */
    void stop();

    /**
     * @brief Returns true if the feed is running, false otherwise.
     */
    bool is_running() const;

    /**
     * @brief Registers a callback to process decoded market data.
     *
     * Sets the callback function to be invoked for each valid market data packet. The callback
     * receives SIMD vectors of prices and volumes, along with a side mask indicating bids (0)
     * and asks (1) for up to 16 entries.
     *
     * @param cb Callback function taking prices, volumes, and side mask as arguments.
     */
    void register_callback(std::function<void(const __m512i& prices, const __m512i& volumes, __mmask16 side_mask)> cb) {
        _callback = std::move(cb);
    }

    /**
     * @brief Retrieves the current processing statistics.
     *
     * @return A const reference to the FeedStats structure containing packet and processing metrics.
     */
    const FeedStats& stats() const { return _stats; }

    /**
     * @brief Runs the loop to receive and process market data packets.
     *
     * Continuously receives UDP packets, decodes them into MarketData structures, validates
     * entries (type ORDER_ADD, non-zero volume, valid side), and constructs SIMD vectors for
     * prices, volumes, and a side mask. Invokes the registered callback for valid entries and
     * updates statistics.
     */
    void receive_loop();

    /**
     * @brief Binds the receiving thread to the specified CPU core.
     *
     * If a valid CPU core is specified, sets the thread's affinity to that core to improve
     * performance by reducing context switches. Logs an error if binding fails.
     */
    void bind_cpu_core();

private:
    int _fd = -1;   ///< File descriptor for the UDP socket used to receive multicast packets.
    int _cpu_core;  ///< CPU core for thread affinity (-1 indicates no binding).

    std::atomic<bool> _running{false};  ///< Flag indicating whether the receive loop is active.
    std::thread _thread;                ///< Thread running the receive_loop for packet processing.
    FeedStats _stats;                   ///< Statistics tracking packets received, updates processed,

    /**
     * @brief Callback function for processing market data.
     *
     * Invoked with SIMD vectors of prices, volumes, and a side mask for valid market data entries.
     */
    std::function<void(const __m512i& prices, const __m512i& volumes, __mmask16 side_mask)> _callback;
};