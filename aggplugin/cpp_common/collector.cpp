/*
 * collector.cpp - Background Metric Aggregation Engine Implementation
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Implements continuous background sampling of system metrics with statistical
 *   aggregation. Runs a dedicated thread that samples metrics at configurable
 *   intervals and maintains running statistics for each metric.
 *
 * ARCHITECTURE:
 *   Single sampling thread loops every base_interval_seconds:
 *     1. Iterate through all registered metrics
 *     2. Check if metric is due for sampling (based on multiplicator)
 *     3. Call plugin_sample_numeric() to get current value
 *     4. Add value to metric's accumulator (thread-safe with mutex)
 *     5. Auto-reset if max_samples threshold reached
 *
 * RELATION TO OTHER CODE:
 *   - Header: collector.hpp (API definitions)
 *   - Uses: plugin_common.hpp/cpp for metric sampling (plugin_sample_numeric)
 *   - Used by: collector_shared.cpp (CGO exports)
 *            : main.go (Go wrapper via CGO)
 *            : classic_wrapper.c (Classic Agent)
 *
 * THREAD MODEL:
 *   - Main thread: Calls init/stop/register/fetch functions
 *   - Collector thread: Background sampling loop (created in collector_init)
 *   - Synchronization: Per-metric mutexes (Accumulator::m), global metrics_m
 *
 * MEMORY MANAGEMENT:
 *   - Each metric stores all sample values in std::vector<double> for median/mode
 *   - Auto-reset when max_samples reached to prevent unbounded growth
 *   - Typical memory: 1000 samples * 8 bytes = 8 KB per metric
 */

// INCLUDE DOCUMENTATION:

#include "collector.hpp"         // Collector API (extern "C" function declarations)
#include "plugin_common.hpp"     // Metric sampling: plugin_sample_numeric(), plugin_sample_cpu_per_core()

// C++ Standard Library includes:
#include <thread>                // std::thread - background sampling thread
#include <chrono>                // std::chrono - time intervals for sleep
#include <atomic>                // std::atomic<bool> - thread-safe running flag
#include <unordered_map>         // std::unordered_map - metric name -> MetricEntry lookup
#include <mutex>                 // std::mutex, std::lock_guard - thread synchronization
#include <string>                // std::string - metric names and JSON building
#include <sstream>               // std::ostringstream - JSON string construction
#include <limits>                // std::numeric_limits - initial min/max values
#include <cmath>                 // std::sqrt, std::llround - statistics computation
#include <cstring>               // std::memcpy (not used directly, but may be required by other headers)
#include <vector>                // std::vector<double> - store all samples for median/mode
#include <algorithm>             // std::sort - median computation requires sorted values

/*
 * struct Accumulator - Thread-safe sample accumulator for a single metric
 *
 * PURPOSE:
 *   Stores all samples for one metric and provides thread-safe accumulation
 *   with automatic reset when threshold reached.
 *
 * MEMBERS:
 *   m: Mutex for thread-safe access to all other members
 *   
 *   count: Number of samples accumulated so far (uint64_t)
 *          Incremented on each add() call
 *          Reset to 0 when max_samples reached or fetch_and_reset_json() called
 *   
 *   max_samples: Auto-reset threshold (uint64_t, default 1000)
 *                When count reaches this value, accumulator resets automatically
 *                Prevents unbounded memory growth in long-running agents
 *                Configurable via collector_set_max_samples()
 *   
 *   sum: Running sum of all sample values (double)
 *        Used for average calculation: avg = sum / count
 *   
 *   sum_sq: Running sum of squared values (double)
 *           Used for variance: var = (sum_sq / count) - (avg * avg)
 *   
 *   min: Minimum value observed (double)
 *        Initialized to +infinity, updated on each add() if v < min
 *   
 *   max: Maximum value observed (double)
 *        Initialized to -infinity, updated on each add() if v > max
 *   
 *   values: All sample values stored in insertion order (std::vector<double>)
 *           Required for median (must sort) and mode (frequency count)
 *           Memory impact: count * 8 bytes (e.g., 8 KB for 1000 samples)
 *
 * THREAD SAFETY:
 *   All methods acquire lock_guard<mutex> before accessing members
 */
struct Accumulator {
    std::mutex m;                // Thread synchronization
    uint64_t count = 0;          // Number of samples
    uint64_t max_samples = 1000; // Auto-reset threshold
    double sum = 0.0;            // Sum for average
    double sum_sq = 0.0;         // Sum of squares for variance
    double min = std::numeric_limits<double>::infinity();   // Minimum value
    double max = -std::numeric_limits<double>::infinity();  // Maximum value
    std::vector<double> values;  // All values for median/mode

    /*
     * add - Add a sample value to the accumulator
     *
     * PARAMETERS:
     *   v: Sample value to add (double)
     *
     * BEHAVIOR:
     *   - Acquires mutex lock
     *   - Checks if max_samples reached, resets if yes
     *   - Increments count
     *   - Updates sum, sum_sq, min, max
     *   - Appends value to values vector
     *   - Releases mutex lock
     *
     * CALLED BY: collector_loop() for each metric sample
     *
     * THREAD SAFETY: Safe (mutex protected)
     */
    void add(double v) {
        std::lock_guard<std::mutex> lk(m);
        
        // Auto-reset if max samples reached
        if (count >= max_samples) {
            count = 0;
            sum = 0.0;
            sum_sq = 0.0;
            min = std::numeric_limits<double>::infinity();
            max = -std::numeric_limits<double>::infinity();
            values.clear();
        }
        
        count++;
        sum += v;
        sum_sq += v * v;
        if (v < min) min = v;
        if (v > max) max = v;
        values.push_back(v);
    }
    
    /*
     * set_max_samples - Update the auto-reset threshold
     *
     * PARAMETERS:
     *   max: New threshold value (uint64_t, must be > 0)
     *
     * BEHAVIOR:
     *   - Acquires mutex lock
     *   - Updates max_samples
     *   - Takes effect on next add() call
     *
     * CALLED BY: collector_set_max_samples() API
     *
     * THREAD SAFETY: Safe (mutex protected)
     */
    void set_max_samples(uint64_t max) {
        std::lock_guard<std::mutex> lk(m);
        max_samples = max;
    }

    /*
     * fetch_and_reset_json - Compute statistics, generate JSON, and reset
     *
     * PARAMETERS:
     *   metric: Metric name string for JSON output (const std::string&)
     *   include_per_core: If true and metric=="cpu_load", add per-core data (bool, default false)
     *                     Currently per-core is not implemented, so this is ignored
     *
     * BEHAVIOR:
     *   - Acquires mutex lock
     *   - Computes all statistics from accumulated samples:
     *     * avg = sum / count
     *     * median: sort values, take middle (or average of two middle)
     *     * mode: build frequency map of rounded values, find most frequent
     *     * variance = (sum_sq / count) - (avg * avg)
     *     * stddev = sqrt(variance)
     *   - Generates JSON string with format:
     *     {"metric":"NAME","values":{"all":{"avg":X,"min":X,"max":X,"med":X,"mod":X,"dev":X,"var":X,"cnt":N}}}
     *   - Resets all accumulator members to initial state (count=0, empty values)
     *   - Releases mutex lock
     *   - Returns JSON string
     *
     * RETURN VALUE:
     *   JSON string (std::string) with statistics or null values if count==0
     *
     * CALLED BY: Accumulator::fetch_and_reset_json in MetricEntry context
     *
     * THREAD SAFETY: Safe (mutex protected)
     *
     * ALGORITHM DETAILS:
     *   Median: O(n log n) due to sorting, where n = count
     *   Mode: O(n) for frequency map construction + O(m) for max search, where m = unique values
     *   Variance: O(1) using sum_sq formula (numerically stable for typical metrics)
     */
    std::string fetch_and_reset_json(const std::string &metric, bool include_per_core = false) {
        std::lock_guard<std::mutex> lk(m);
        std::ostringstream o;
        o.precision(6);
        o << "{\"metric\":\"" << metric << "\",";
        o << "\"values\":{\"all\":{";
        if (count == 0) {
            o << "\"avg\":null,\"min\":null,\"max\":null,\"med\":null,\"mod\":null,\"dev\":null,\"var\":null,\"cnt\":0";
        } else {
            double avg = sum / (double)count;
            
            // Compute median
            std::vector<double> sorted_vals = values;
            std::sort(sorted_vals.begin(), sorted_vals.end());
            double median;
            if (sorted_vals.size() % 2 == 0) {
                median = (sorted_vals[sorted_vals.size()/2 - 1] + sorted_vals[sorted_vals.size()/2]) / 2.0;
            } else {
                median = sorted_vals[sorted_vals.size()/2];
            }
            
            // Compute mode (most frequent rounded value)
            std::unordered_map<int, uint64_t> freq_map;
            for (double v : values) {
                freq_map[(int)std::llround(v)]++;
            }
            int mode_val = 0;
            uint64_t mode_count = 0;
            for (auto &p : freq_map) {
                if (p.second > mode_count) {
                    mode_count = p.second;
                    mode_val = p.first;
                }
            }
            
            // Compute variance and standard deviation
            double variance = (sum_sq / (double)count) - (avg * avg);
            double stddev = std::sqrt(variance);
            
            o << "\"avg\":" << avg << ",";
            o << "\"min\":" << min << ",";
            o << "\"max\":" << max << ",";
            o << "\"med\":" << median << ",";
            o << "\"mod\":" << mode_val << ",";
            o << "\"dev\":" << stddev << ",";
            o << "\"var\":" << variance << ",";
            o << "\"cnt\":" << count;
        }
        o << "}";
        
        // Add per-core data if requested (for CPU metrics)
        if (include_per_core && metric == "cpu_load") {
            char core_buffer[4096];
            int num_cores = plugin_sample_cpu_per_core(core_buffer, sizeof(core_buffer));
            if (num_cores > 0) {
                o << ",\"cores\":" << core_buffer;
            }
        }
        
        o << "}}";

        // reset
        count = 0; sum = 0.0; sum_sq = 0.0; min = std::numeric_limits<double>::infinity(); max = -std::numeric_limits<double>::infinity(); values.clear();
        return o.str();
    }
};

/*
 * struct MetricEntry - Registration info and accumulator for one metric
 *
 * PURPOSE:
 *   Stores sampling configuration and accumulator for each registered metric.
 *   Used in global metrics map.
 *
 * MEMBERS:
 *   multiplicator: Sampling frequency multiplier (double, default 1.0)
 *                  - 1.0 = sample every base_interval
 *                  - 0.5 = sample twice as often
 *                  - 2.0 = sample half as often
 *                  Set via collector_register_metric()
 *   
 *   next_tick: Next tick number when this metric should be sampled (double)
 *              - Starts at 1.0 (sample on first tick)
 *              - Incremented by multiplicator after each sample
 *              - Compared against tick counter in collector_loop()
 *   
 *   acc: Accumulator instance for this metric (Accumulator)
 *        Stores all samples and computes statistics
 *
 * USAGE:
 *   Created when metric is registered via collector_register_metric()
 *   Accessed in collector_loop() for sampling
 *   Accessed in collector_fetch_and_reset_json() for stat retrieval
 */
struct MetricEntry {
    double multiplicator = 1.0;  // Sampling frequency multiplier
    double next_tick = 1.0;      // Next tick to sample (first sample at tick 1)
    Accumulator acc;             // Sample accumulator with statistics
};

// GLOBAL VARIABLES:

/*
 * metrics - Map of registered metrics (metric name -> MetricEntry)
 * TYPE: std::unordered_map<std::string, MetricEntry>
 * PROTECTION: metrics_m mutex
 * USAGE:
 *   - Written by: collector_register_metric(), collector_set_max_samples()
 *   - Read by: collector_loop() (sampling), collector_fetch_and_reset_json() (stats)
 * THREAD SAFETY: All access must acquire metrics_m lock
 */
static std::unordered_map<std::string, MetricEntry> metrics;

/*
 * metrics_m - Mutex protecting the metrics map
 * TYPE: std::mutex
 * PROTECTS: metrics map access
 * USAGE: Acquired by all functions that access metrics map
 */
static std::mutex metrics_m;

/*
 * running - Flag indicating if collector thread is running
 * TYPE: std::atomic<bool>
 * USAGE:
 *   - Set to true by collector_init()
 *   - Set to false by collector_stop()
 *   - Checked by collector_loop() on each iteration
 * THREAD SAFETY: Atomic operations (no mutex needed)
 */
static std::atomic<bool> running(false);

/*
 * worker - Background sampling thread
 * TYPE: std::thread
 * LIFECYCLE:
 *   - Created by collector_init() running collector_loop()
 *   - Joined by collector_stop()
 * USAGE: Runs continuously while running flag is true
 */
static std::thread worker;

/*
 * base_interval - Sampling interval in seconds
 * TYPE: double
 * DEFAULT: 1.0 seconds
 * USAGE:
 *   - Set by collector_init(base_interval_seconds)
 *   - Used by collector_loop() for sleep duration
 * THREAD SAFETY: Written once during init, read-only afterwards (no mutex needed)
 */
static double base_interval = 1.0;

/*
 * collector_loop - Background sampling thread main loop
 *
 * PURPOSE:
 *   Continuously samples registered metrics at configured intervals until stopped.
 *
 * BEHAVIOR:
 *   - Runs in dedicated thread created by collector_init()
 *   - Loop structure:
 *     1. Increment tick counter
 *     2. Lock metrics map (metrics_m)
 *     3. For each registered metric:
 *        - Check if current tick >= next_tick (time to sample)
 *        - Call plugin_sample_numeric(metric_name, &ok)
 *        - If successful (ok==1), add value to accumulator
 *        - Update next_tick += multiplicator
 *     4. Unlock metrics map
 *     5. Sleep for base_interval seconds
 *     6. Check running flag, repeat if true
 *
 * TERMINATION:
 *   - Exits when running flag becomes false (set by collector_stop)
 *   - Thread is joined in collector_stop()
 *
 * CALLED BY: std::thread constructor in collector_init()
 *
 * THREAD SAFETY:
 *   - Only one instance runs (singleton thread)
 *   - Acquires metrics_m before accessing metrics map
 *   - Uses atomic running flag for termination check
 *
 * ERRORS:
 *   - If plugin_sample_numeric() fails (ok==0), sample is skipped (no logging)
 *   - No exceptions thrown (metrics continue sampling)
 */
void collector_loop() {
    uint64_t tick = 0;
    while (running.load()) {
        tick++;
        {
            std::lock_guard<std::mutex> lk(metrics_m);
            for (auto &kv : metrics) {
                MetricEntry &me = kv.second;
                if ((double)tick + 1e-9 >= me.next_tick) {
                    // sample
                    int ok = 0;
                    double v = plugin_sample_numeric(kv.first.c_str(), &ok);
                    if (ok) me.acc.add(v);
                    me.next_tick += me.multiplicator;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::duration<double>(base_interval));
    }
}

// API FUNCTION IMPLEMENTATIONS:
// See collector.hpp for detailed API documentation

/*
 * collector_init - Implementation (see collector.hpp for full docs)
 *
 * IMPLEMENTATION DETAILS:
 *   - Stores base_interval_seconds in global base_interval
 *   - Checks running flag atomically
 *   - If not running: sets running=true, creates worker thread executing collector_loop()
 *   - If already running: returns immediately (idempotent)
 *
 * CALLED BY: main.go main(), classic_wrapper.c zbx_module_init()
 */
extern "C" void collector_init(double base_interval_seconds) {
    base_interval = base_interval_seconds;
    if (running.load()) return; // Already running
    running.store(true);
    worker = std::thread(collector_loop);
}

/*
 * collector_stop - Implementation (see collector.hpp for full docs)
 *
 * IMPLEMENTATION DETAILS:
 *   - Checks running flag atomically
 *   - If running: sets running=false, joins worker thread (blocks until thread exits)
 *   - If not running: returns immediately (idempotent)
 *   - Thread termination: collector_loop() checks running flag on each iteration
 *
 * CALLED BY: main.go defer cleanup, classic_wrapper.c zbx_module_uninit()
 */
extern "C" void collector_stop() {
    if (!running.load()) return; // Not running
    running.store(false);
    if (worker.joinable()) worker.join();
}

/*
 * collector_register_metric - Implementation (see collector.hpp for full docs)
 *
 * PARAMETERS:
 *   name: Metric name (e.g., "cpu_load", "mem_free")
 *   multiplicator: Sampling frequency (e.g., 1.0, 0.5, 2.0)
 *
 * IMPLEMENTATION DETAILS:
 *   - Validates name != NULL
 *   - Acquires metrics_m lock
 *   - Creates or updates MetricEntry in metrics map
 *   - Sets multiplicator and resets next_tick to 1.0 (immediate sampling)
 *   - If metric already exists, only multiplicator is updated (accumulator preserved)
 *
 * RETURN: 0=success, 1=name is NULL
 *
 * CALLED BY: main.go main(), classic_wrapper.c zbx_module_init()
 */
extern "C" int collector_register_metric(const char *name, double multiplicator) {
    if (!name) return 1;
    std::lock_guard<std::mutex> lk(metrics_m);
    std::string s(name);
    MetricEntry &me = metrics[s];
    me.multiplicator = multiplicator;
    me.next_tick = 1.0; // Start sampling on first tick
    return 0;
}

/*
 * collector_set_max_samples - Implementation (see collector.hpp for full docs)
 *
 * PARAMETERS:
 *   name: Metric name (must be registered)
 *   max_samples: Auto-reset threshold (e.g., 1000, 10000)
 *
 * IMPLEMENTATION DETAILS:
 *   - Validates name != NULL
 *   - Acquires metrics_m lock
 *   - Looks up metric in map
 *   - If not found: returns 1 (error)
 *   - If found: calls acc.set_max_samples() to update threshold
 *
 * RETURN: 0=success, 1=name is NULL or metric not registered
 *
 * CALLED BY: main.go main()
 */
extern "C" int collector_set_max_samples(const char *name, unsigned max_samples) {
    if (!name) return 1;
    std::lock_guard<std::mutex> lk(metrics_m);
    std::string s(name);
    auto it = metrics.find(s);
    if (it == metrics.end()) return 1; // Metric not registered
    it->second.acc.set_max_samples(max_samples);
    return 0;
}

/*
 * collector_fetch_and_reset_json - Implementation (see collector.hpp for full docs)
 *
 * PARAMETERS:
 *   name: Metric name (must be registered)
 *   result: Output buffer (caller-allocated)
 *   result_len: Buffer size in bytes
 *
 * IMPLEMENTATION DETAILS:
 *   - Validates name != NULL and result != NULL
 *   - Acquires metrics_m lock
 *   - Looks up metric in map
 *   - If not found: returns 1 (error)
 *   - If found:
 *     * Determines if per-core data needed (cpu_load metric)
 *     * Calls acc.fetch_and_reset_json() to get JSON string
 *     * Copies JSON to result buffer (truncates if too large)
 *     * Ensures null-termination
 *     * Returns 0 (success)
 *
 * RETURN: 0=success, 1=name is NULL, result is NULL, or metric not registered
 *
 * CALLED BY: main.go Export() for each metric query
 *
 * BUFFER HANDLING:
 *   - If JSON fits: full copy with strcpy
 *   - If JSON too large: truncated copy with strncpy + null terminator
 *   - Truncation is silent (no error logged, caller sees truncated JSON)
 */
extern "C" int collector_fetch_and_reset_json(const char *name, char *result, unsigned result_len) {
    if (!name || !result) return 1;
    std::string s(name);
    std::lock_guard<std::mutex> lk(metrics_m);
    auto it = metrics.find(s);
    if (it == metrics.end()) return 1; // Metric not found
    
    // Enable per-core data for CPU metrics (future feature, currently not implemented)
    bool include_per_core = (s == "cpu_load");
    std::string js = it->second.acc.fetch_and_reset_json(s, include_per_core);
    
    if (js.size() + 1 > result_len) {
        // Truncate if buffer too small
        strncpy(result, js.c_str(), result_len - 1);
        result[result_len - 1] = '\0';
    } else {
        strcpy(result, js.c_str());
    }
    return 0;
}
