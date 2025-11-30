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
#include "plugin_loader.hpp"     // Multi-source sampling: plugin_sample_multi_from_dll()

// C++ Standard Library includes:
#include <thread>                // std::thread - background sampling thread
#include <chrono>                // std::chrono - time intervals for sleep
#include <atomic>                // std::atomic<bool> - thread-safe running flag
#include <unordered_map>         // std::unordered_map - metric name -> MetricEntry lookup
#include <mutex>                 // std::mutex, std::lock_guard - thread synchronization
#include <string>                // std::string - metric names and JSON building
#include <sstream>               // std::ostringstream - JSON string construction
#include <limits>                // std::numeric_limits - initial min/max values
#include <cstdlib>               // free() - cleanup multi-source results
#include <cmath>                 // std::sqrt, std::llround - statistics computation
#include <cstring>               // std::memcpy (not used directly, but may be required by other headers)
#include <vector>                // std::vector<double> - store all samples for median/mode
#include <algorithm>             // std::sort - median computation requires sorted values
#include <map>                   // std::map - mode frequency map (limited to 100 unique values)
#include <cstdio>                // fprintf - debugging output
#include <ctime>                 // time(), localtime() - timestamps for logging
#include <cstdarg>               // va_list, va_start, va_end - variadic function support

// Debug logging (write to file since stderr is not available when running as Windows Service)
static FILE* debug_log = nullptr;
static std::mutex debug_log_mutex;

void log_debug(const char* fmt, ...) {
    std::lock_guard<std::mutex> lk(debug_log_mutex);
    if (!debug_log) {
        debug_log = fopen("C:\\Zabbix\\log\\collector_debug.log", "a");
        if (!debug_log) return;
    }
    
    // Timestamp
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    fprintf(debug_log, "[%04d-%02d-%02d %02d:%02d:%02d] [COLLECTOR] ",
            tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
            tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
    
    // Message
    va_list args;
    va_start(args, fmt);
    vfprintf(debug_log, fmt, args);
    va_end(args);
    fprintf(debug_log, "\n");
    fflush(debug_log);
}
#include <cmath>                 // std::sqrt, std::llround - statistics computation
#include <cstring>               // std::memcpy (not used directly, but may be required by other headers)
#include <vector>                // std::vector<double> - store all samples for median/mode
#include <algorithm>             // std::sort - median computation requires sorted values

/*
 * struct Accumulator - Thread-safe sample accumulator using incremental statistics
 *
 * PURPOSE:
 *   Stores summary statistics for one metric using online/incremental algorithms
 *   to avoid storing all samples. Uses weighted updates when max_samples reached.
 *
 * MEMBERS:
 *   m: Mutex for thread-safe access to all other members
 *   
 *   count: Number of samples accumulated (uint64_t, stays at max_samples after threshold)
 *   max_samples: Weight threshold (uint64_t, default 1000)
 *   sum: Running weighted sum (avg = sum / count)
 *   sum_sq: Running weighted sum of squares (for variance)
 *   min: Minimum value observed (never reset, only updated if new value lower)
 *   max: Maximum value observed (never reset, only updated if new value higher)
 *   last: Last value added (for "last" statistic in JSON output)
 *   median: Approximate median (updated using weighted average when max_samples reached)
 *   mode_map: Frequency map for mode calculation (limited to 100 unique values max)
 *
 * ALGORITHM (when count >= max_samples):
 *   New value has weight=1, old summary has weight=max_samples
 *   avg_new = (max_samples * avg_old + new_value) / (max_samples + 1)
 *   This keeps count fixed at max_samples, prevents memory growth
 *
 * MEMORY: ~100 bytes per accumulator (no value vector storage)
 *
 * THREAD SAFETY: All methods acquire lock_guard<mutex> before accessing members
 */
struct Accumulator {
    std::mutex m;
    uint64_t count = 0;
    uint64_t max_samples = 1000;
    double sum = 0.0;
    double sum_sq = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = -std::numeric_limits<double>::infinity();
    double last = 0.0;
    double median = 0.0;  // Approximate median (weighted update after max_samples)
    std::map<int, uint64_t> mode_map;  // Value (rounded to int) -> frequency count

    /*
     * add - Add sample using incremental statistics with weighted update
     *
     * BEHAVIOR:
     *   - If count < max_samples: accumulate normally
     *   - If count >= max_samples: use weighted update (old weight=max_samples, new weight=1)
     *     This keeps statistics current while preventing unbounded growth
     *
     * THREAD SAFETY: Safe (mutex protected)
     */
    void add(double v) {
        std::lock_guard<std::mutex> lk(m);
        
        if (count < max_samples) {
            // Normal accumulation phase
            count++;
            sum += v;
            sum_sq += v * v;
            median = v;  // Simplified: last value becomes median estimate
        } else {
            // Weighted update phase (count stays at max_samples)
            // New avg = (max_samples * old_avg + new_value) / (max_samples + 1)
            double old_avg = sum / count;
            double old_avg_sq = sum_sq / count;
            
            sum = (max_samples * old_avg + v) / (max_samples + 1) * max_samples;
            sum_sq = (max_samples * old_avg_sq + v * v) / (max_samples + 1) * max_samples;
            
            // Median: weighted average
            median = (max_samples * median + v) / (max_samples + 1);
        }
        
        // Min/max never reset
        if (v < min) min = v;
        if (v > max) max = v;
        last = v;
        
        // Mode: update frequency map (limit to prevent unbounded memory growth)
        int rounded_val = (int)std::round(v);
        if (mode_map.count(rounded_val)) {
            mode_map[rounded_val]++;
        } else if (mode_map.size() < 100) {
            mode_map[rounded_val] = 1;
        }
        // If map is full and value is new, ignore it (mode will be approximate)
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
            o << "\"avg\":null,\"min\":null,\"max\":null,\"med\":null,\"mod\":null,\"dev\":null,\"var\":null,\"cnt\":0,\"last\":null";
        } else {
            double avg = sum / (double)count;
            
            // Use precomputed median (approximate, updated incrementally)
            // No sorting needed - median is maintained during add()
            
            // Compute mode from prebuilt frequency map
            int mode_val = 0;
            uint64_t mode_count = 0;
            for (auto &p : mode_map) {
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
            o << "\"cnt\":" << count << ",";
            o << "\"last\":" << last;
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
        count = 0; sum = 0.0; sum_sq = 0.0; min = std::numeric_limits<double>::infinity(); max = -std::numeric_limits<double>::infinity(); median = 0.0; mode_map.clear();
        return o.str();
    }
};

/*
 * struct MetricEntry - Registration info and accumulators for one metric
 *
 * PURPOSE:
 *   Stores sampling configuration and accumulator(s) for each registered metric.
 *   Supports both single-source and multi-source metrics.
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
 *   source_accumulators: Map of source_id → Accumulator for multi-source metrics
 *                       - Key="" for single-source metrics
 *                       - Key="0","1","2" for per-disk metrics
 *                       - Key="core0","core1" for per-CPU metrics
 *                       Each source maintains independent statistics
 *   
 *   filter: Filter pattern for source selection (std::string, default empty)
 *           - Empty = no filter set, skip sampling (no data collected)
 *           - Must be min 3 chars + wildcard (e.g. "svc*", "chrome*")
 *           - Set via fetch_and_reset_json parameter
 *           - Used by plugins to filter which sources to sample
 *
 * USAGE:
 *   Created when metric is registered via collector_register_metric()
 *   Accessed in collector_loop() for sampling
 *   Accessed in collector_fetch_and_reset_json() for stat retrieval
 */
struct MetricEntry {
    double multiplicator = 1.0;  // Sampling frequency multiplier
    double next_tick = 1.0;      // Next tick to sample (first sample at tick 1)
    std::unordered_map<std::string, Accumulator> source_accumulators;  // Per-source accumulators
    std::string filter;          // Filter pattern (empty = skip sampling)
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
Get-Process | Where-Object { $_.ProcessName -like 'aggplugin*' } | Select-Object ProcessName, @{Name='Memory(MB)';Expression={[math]::Round($_.WorkingSet64/1MB,2)}}, @{Name='CPU(%)';Expression={$_.CPU}}, @{Name='Runtime(min)';Expression={[math]::Round(((Get-Date) - $_.StartTime).TotalMinutes,1)}} */

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
 * output_format - Global output format selector
 * TYPE: std::atomic<int>
 * VALUES:
 *   0 = array format (default, Zabbix-compatible): [{"source":"_all","avg":...},{"source":"C:",...}]
 *   1 = nestedJSON format (legacy): {"metric":"...","values":{"all":{...},"sources":{...}}}
 * USAGE:
 *   - Set by collector_set_output_format()
 *   - Read by collector_fetch_and_reset_json()
 * DEFAULT: 0 (array format)
 * THREAD SAFETY: Atomic operations (no mutex needed)
 */
static std::atomic<int> output_format(0);  // Default: array format

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
    uint64_t total_samples = 0;
    // Collector thread logging disabled (verbose, impacts performance)
    // Uncomment for debugging: log_debug("Thread started, base_interval=%.1fs, running=%d", base_interval, running.load());
    while (running.load()) {
        tick++;
        // Debug: log every 10 seconds
        if (tick % 10 == 0) {
            std::lock_guard<std::mutex> lk(metrics_m);
            size_t total_accumulators = 0;
            for (auto &kv : metrics) {
                total_accumulators += kv.second.source_accumulators.size();
            }
            log_debug("Tick %llu: %zu metrics, %zu accumulators, %llu total samples",
                     (unsigned long long)tick, metrics.size(), total_accumulators, 
                     (unsigned long long)total_samples);
        }
        {
            std::lock_guard<std::mutex> lk(metrics_m);
            for (auto &kv : metrics) {
                MetricEntry &me = kv.second;
                if ((double)tick + 1e-9 >= me.next_tick) {
                    // Skip metrics with no filter set (wait for first query to set filter)
                    // This prevents sampling hundreds of services/processes until user queries with filter
                    if (me.filter.empty() && (kv.first == "proc.running" || kv.first == "service.status")) {
                        me.next_tick += me.multiplicator;
                        continue;
                    }
                    
                    // Try multi-source collection first (for DLL plugins)
                    size_t source_count = 0;
                    int multi_ok = 0;
                    multi_source_result_t* sources = plugin_sample_multi_from_dll(
                        kv.first.c_str(), &source_count, &multi_ok);
                    
                    if (multi_ok && sources) {
                        // Multi-source metric: add each source independently
                        // Limit to 100 sources per metric to prevent memory explosion
                        for (size_t i = 0; i < source_count; i++) {
                            // Check source limit BEFORE creating std::string (which allocates memory)
                            const char* source_cstr = sources[i].source_id ? sources[i].source_id : "";
                            
                            // Quick check: if map is at limit and this is a new source, skip
                            if (me.source_accumulators.size() >= 100) {
                                bool found = false;
                                for (auto& acc_pair : me.source_accumulators) {
                                    if (acc_pair.first == source_cstr) {
                                        found = true;
                                        acc_pair.second.add(sources[i].value);
                                        total_samples++;
                                        break;
                                    }
                                }
                                if (!found) {
                                    // New source but limit reached - skip and free
                                    if (i < 3 || i % 100 == 0) {  // Log only first few to avoid spam
                                        log_debug("WARNING: Metric %s has 100+ sources, ignoring: %s",
                                                 kv.first.c_str(), source_cstr);
                                    }
                                }
                                // Always free the source_id string
                                if (sources[i].source_id) free(sources[i].source_id);
                            } else {
                                // Not at limit yet - normal path
                                std::string source_id(source_cstr);
                                me.source_accumulators[source_id].add(sources[i].value);
                                total_samples++;
                                
                                // Free source_id string
                                if (sources[i].source_id) free(sources[i].source_id);
                            }
                        }
                        free(sources);
                    } else {
                        // Fallback to single-value collection
                        int ok = 0;
                        double v = plugin_sample_numeric(kv.first.c_str(), &ok);
                        if (ok) {
                            // Single-source metric: use empty string as source_id
                            me.source_accumulators[""].add(v);
                            total_samples++;
                        } else {
                            log_debug("ERROR: Failed to sample %s", kv.first.c_str());
                        }
                    }
                    
                    me.next_tick += me.multiplicator;
                }
            }
        }
        // Uncomment for sleep/wake debugging:
        // log_debug("About to sleep for %.1fs (tick %llu)", base_interval, (unsigned long long)tick);
        std::this_thread::sleep_for(std::chrono::duration<double>(base_interval));
        // log_debug("Woke up from sleep (tick %llu), running=%d", (unsigned long long)tick, running.load());
    }
    // log_debug("Thread exiting (running=%d)", running.load());
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
    // Uncomment for registration debugging:
    // log_debug("Registering metric: %s (multiplicator=%.2f)", name, multiplicator);
    std::lock_guard<std::mutex> lk(metrics_m);
    std::string s(name);
    MetricEntry &me = metrics[s];
    me.multiplicator = multiplicator;
    me.next_tick = 1.0; // Start sampling on first tick
    // log_debug("Metric registered: %s (total metrics now: %zu)", name, metrics.size());
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
 *   - If found: sets max_samples for ALL source accumulators
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
    
    // Set max_samples for all existing source accumulators
    for (auto& source_acc : it->second.source_accumulators) {
        source_acc.second.set_max_samples(max_samples);
    }
    
    return 0;
}

/*
 * collector_set_output_format - Implementation (see collector.hpp for full docs)
 *
 * PARAMETERS:
 *   format: 0=array (default), 1=nestedJSON (legacy)
 *
 * IMPLEMENTATION DETAILS:
 *   - Validates format is 0 or 1
 *   - Sets global atomic output_format variable
 *   - Takes effect immediately for all subsequent fetch calls
 *
 * RETURN: 0=success, 1=invalid format
 *
 * CALLED BY: main.go main() after loadConfig()
 */
extern "C" int collector_set_output_format(int format) {
    if (format != 0 && format != 1) return 1;
    output_format.store(format);
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
 *     * Checks global output_format (0=array, 1=nestedJSON)
 *     * Computes statistics across all sources
 *     * Generates JSON in selected format
 *     * Copies JSON to result buffer (truncates if too large)
 *     * Ensures null-termination
 *     * Returns 0 (success)
 *
 * OUTPUT FORMATS:
 *   Array format (format=0, default):
 *     [{"source":"_all","avg":45.2,"min":12.3,"max":89.7,"med":43.1,"mod":45,"dev":15.6,"var":243.4,"cnt":120,"last":45.0},
 *      {"source":"C:","avg":42.1,"min":10.0,"max":88.5,"med":40.2,"mod":42,"dev":14.3,"var":204.5,"cnt":60,"last":42.5}]
 *
 *   NestedJSON format (format=1, legacy):
 *     {"metric":"cpu_load","values":{"all":{...},"sources":{"C:":{...}}}}
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
extern "C" int collector_fetch_and_reset_json(const char *name, char *result, unsigned result_len, const char *filter) {
    if (!name || !result) return 1;
    std::string s(name);
    std::lock_guard<std::mutex> lk(metrics_m);
    auto it = metrics.find(s);
    if (it == metrics.end()) return 1; // Metric not found
    
    // Update filter if provided and valid (min 3 chars)
    if (filter && strlen(filter) >= 3) {
        it->second.filter = filter;
        log_debug("Filter set for metric %s: '%s'", name, filter);
    }
    // If no filter set yet, return empty result (no sampling performed)
    else if (it->second.filter.empty()) {
        // No filter set - return empty JSON
        if (output_format.load() == 0) {
            snprintf(result, result_len, "[]");
        } else {
            snprintf(result, result_len, "{\"metric\":\"%s\",\"values\":{\"all\":{\"avg\":null,\"min\":null,\"max\":null,\"med\":null,\"mod\":null,\"dev\":null,\"var\":null,\"cnt\":0}}}", name);
        }
        return 0;
    }
    
    int current_format = output_format.load();
    std::ostringstream o;
    o.precision(6);
    
    // Calculate aggregate "all" statistics across all sources
    uint64_t total_count = 0;
    double total_sum = 0.0;
    double global_min = std::numeric_limits<double>::infinity();
    double global_max = -std::numeric_limits<double>::infinity();
    double last_value = 0.0;
    
    // Collect per-source stats for array format
    struct SourceStats {
        std::string source_id;
        double avg, min, max, med, dev, var, last;
        int mode_val;
        uint64_t cnt;
    };
    std::vector<SourceStats> source_stats_list;
    
    for (auto& source_pair : it->second.source_accumulators) {
        Accumulator& acc = source_pair.second;
        std::lock_guard<std::mutex> acc_lock(acc.m);
        
        total_count += acc.count;
        total_sum += acc.sum;
        if (acc.min < global_min) global_min = acc.min;
        if (acc.max > global_max) global_max = acc.max;
        last_value = acc.last;  // Use incremental last value
        
        // Calculate per-source stats for array format
        if (current_format == 0 && acc.count > 0) {
            SourceStats ss;
            ss.source_id = source_pair.first.empty() ? "default" : source_pair.first;
            ss.avg = acc.sum / (double)acc.count;
            ss.min = acc.min;
            ss.max = acc.max;
            ss.cnt = acc.count;
            ss.last = acc.last;  // Use incremental last value
            
            // Use precomputed median (approximate)
            ss.med = acc.median;
            
            // Mode from prebuilt frequency map
            ss.mode_val = 0;
            uint64_t mode_count = 0;
            for (auto &p : acc.mode_map) {
                if (p.second > mode_count) {
                    mode_count = p.second;
                    ss.mode_val = p.first;
                }
            }
            
            // Variance and stddev for this source
            ss.var = (acc.sum_sq / (double)acc.count) - (ss.avg * ss.avg);
            ss.dev = std::sqrt(ss.var);
            
            source_stats_list.push_back(ss);
        }
    }
    
    // Calculate aggregate statistics
    double global_avg = total_count > 0 ? total_sum / (double)total_count : 0.0;
    double global_median = 0.0;
    int global_mode = 0;
    double global_var = 0.0;
    double global_dev = 0.0;
    
    if (total_count > 0) {
        // Global median: weighted average of per-source medians
        double median_sum = 0.0;
        uint64_t median_weight = 0;
        for (auto& source_pair : it->second.source_accumulators) {
            Accumulator& acc = source_pair.second;
            std::lock_guard<std::mutex> acc_lock(acc.m);
            if (acc.count > 0) {
                median_sum += acc.median * acc.count;
                median_weight += acc.count;
            }
        }
        global_median = median_weight > 0 ? median_sum / median_weight : 0.0;
        
        // Global mode: merge frequency maps from all sources
        std::map<int, uint64_t> merged_mode_map;
        double sum_sq_global = 0.0;
        for (auto& source_pair : it->second.source_accumulators) {
            Accumulator& acc = source_pair.second;
            std::lock_guard<std::mutex> acc_lock(acc.m);
            for (auto &p : acc.mode_map) {
                merged_mode_map[p.first] += p.second;
            }
            sum_sq_global += acc.sum_sq;
        }
        uint64_t mode_count = 0;
        for (auto &p : merged_mode_map) {
            if (p.second > mode_count) {
                mode_count = p.second;
                global_mode = p.first;
            }
        }
        
        // Global variance and stddev
        global_var = (sum_sq_global / (double)total_count) - (global_avg * global_avg);
        global_dev = std::sqrt(global_var);
    }
    
    // ========== FORMAT GENERATION ==========
    
    if (current_format == 0) {
        // ARRAY FORMAT (default, Zabbix-compatible)
        o << "[";
        
        // First entry: aggregate "all" stats
        o << "{\"source\":\"all\",";
        if (total_count == 0) {
            o << "\"avg\":null,\"min\":null,\"max\":null,\"med\":null,\"mod\":null,\"dev\":null,\"var\":null,\"cnt\":0,\"last\":null";
        } else {
            o << "\"avg\":" << global_avg << ",";
            o << "\"min\":" << global_min << ",";
            o << "\"max\":" << global_max << ",";
            o << "\"med\":" << global_median << ",";
            o << "\"mod\":" << global_mode << ",";
            o << "\"dev\":" << global_dev << ",";
            o << "\"var\":" << global_var << ",";
            o << "\"cnt\":" << total_count << ",";
            o << "\"last\":" << last_value;
        }
        o << "}";
        
        // Per-source entries (if multi-source metric)
        for (const auto& ss : source_stats_list) {
            o << ",{\"source\":\"" << ss.source_id << "\",";
            o << "\"avg\":" << ss.avg << ",";
            o << "\"min\":" << ss.min << ",";
            o << "\"max\":" << ss.max << ",";
            o << "\"med\":" << ss.med << ",";
            o << "\"mod\":" << ss.mode_val << ",";
            o << "\"dev\":" << ss.dev << ",";
            o << "\"var\":" << ss.var << ",";
            o << "\"cnt\":" << ss.cnt << ",";
            o << "\"last\":" << ss.last;
            o << "}";
        }
        
        o << "]";
        
    } else {
        // NESTED JSON FORMAT (legacy)
        o << "{\"metric\":\"" << s << "\",\"values\":{";
        
        // Generate "all" aggregate stats
        o << "\"all\":{";
        if (total_count == 0) {
            o << "\"avg\":null,\"min\":null,\"max\":null,\"med\":null,\"mod\":null,\"dev\":null,\"var\":null,\"cnt\":0";
        } else {
            o << "\"avg\":" << global_avg << ",";
            o << "\"min\":" << global_min << ",";
            o << "\"max\":" << global_max << ",";
            o << "\"med\":" << global_median << ",";
            o << "\"mod\":" << global_mode << ",";
            o << "\"dev\":" << global_dev << ",";
            o << "\"var\":" << global_var << ",";
            o << "\"cnt\":" << total_count;
        }
        o << "}";
        
        // Add per-source statistics if multi-source
        if (it->second.source_accumulators.size() > 1 || 
            (it->second.source_accumulators.size() == 1 && !it->second.source_accumulators.begin()->first.empty())) {
            o << ",\"sources\":{";
            bool first_source = true;
            
            for (const auto& ss : source_stats_list) {
                if (!first_source) o << ",";
                first_source = false;
                
                o << "\"" << ss.source_id << "\":{";
                o << "\"avg\":" << ss.avg << ",";
                o << "\"min\":" << ss.min << ",";
                o << "\"max\":" << ss.max << ",";
                o << "\"med\":" << ss.med << ",";
                o << "\"mod\":" << ss.mode_val << ",";
                o << "\"dev\":" << ss.dev << ",";
                o << "\"var\":" << ss.var << ",";
                o << "\"cnt\":" << ss.cnt;
                o << "}";
            }
            o << "}";
        }
        
        o << "}}";
    }
    
    // Reset all accumulators after generating JSON
    for (auto& source_pair : it->second.source_accumulators) {
        Accumulator& acc = source_pair.second;
        std::lock_guard<std::mutex> acc_lock(acc.m);
        acc.count = 0;
        acc.sum = 0.0;
        acc.sum_sq = 0.0;
        acc.min = std::numeric_limits<double>::infinity();
        acc.max = -std::numeric_limits<double>::infinity();
        acc.median = 0.0;
        acc.mode_map.clear();
    }
    
    std::string js = o.str();
    
    if (js.size() + 1 > result_len) {
        // Truncate if buffer too small
        strncpy(result, js.c_str(), result_len - 1);
        result[result_len - 1] = '\0';
    } else {
        strcpy(result, js.c_str());
    }
    return 0;
}
