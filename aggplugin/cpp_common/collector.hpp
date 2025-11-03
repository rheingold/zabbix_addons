/*
 * collector.hpp - Background Metric Aggregation Engine Header
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Provides API for continuous background sampling of system metrics with statistical
 *   aggregation. Runs a dedicated sampling thread that collects metric values at
 *   configurable intervals and computes comprehensive statistics.
 *
 * RELATION TO OTHER CODE:
 *   - Implemented in: collector.cpp
 *   - Used by: collector_shared.cpp (CGO DLL exports)
 *            : main.go (Go wrapper via CGO)
 *            : classic_wrapper.c (Classic Agent)
 *   - Uses: plugin_common.hpp/cpp (metric sampling functions)
 *
 * STATISTICS COMPUTED:
 *   - avg: arithmetic mean of all samples
 *   - min: minimum value observed
 *   - max: maximum value observed  
 *   - med: median (50th percentile, requires sorting)
 *   - mod: mode (most frequent rounded integer value)
 *   - dev: standard deviation (measure of spread)
 *   - var: variance (square of standard deviation)
 *   - cnt: total number of samples collected
 *
 * THREAD SAFETY:
 *   All functions are thread-safe. Multiple threads can call these functions
 *   concurrently without external synchronization.
 */

#pragma once

#include <cstddef>

/*
 * collector_init - Initialize and start the background sampling thread
 *
 * PARAMETERS:
 *   base_interval_seconds: Sampling interval in seconds (e.g., 1.0 for 1-second intervals)
 *                         Must be > 0. Typical values: 0.5 to 5.0 seconds.
 *
 * BEHAVIOR:
 *   - Starts a dedicated background thread that runs until collector_stop() is called
 *   - Thread samples registered metrics at each interval tick
 *   - If already initialized, subsequent calls are ignored (idempotent)
 *
 * THREAD SAFETY: Safe to call from multiple threads (first call wins)
 *
 * CALLED BY: main.go main(), classic_wrapper.c zbx_module_init()
 * 
 * ERRORS: None (no return value). Invalid interval will cause undefined sampling behavior.
 */
extern "C" void collector_init(double base_interval_seconds);

/*
 * collector_stop - Stop the background sampling thread and cleanup
 *
 * BEHAVIOR:
 *   - Signals the sampling thread to terminate
 *   - Blocks until the thread has fully exited
 *   - If not initialized or already stopped, this is a no-op (idempotent)
 *   - After stopping, collector_init() can be called again to restart
 *
 * THREAD SAFETY: Safe to call from multiple threads
 *
 * CALLED BY: main.go defer cleanup, classic_wrapper.c zbx_module_uninit()
 *
 * ERRORS: None (no return value)
 */
extern "C" void collector_stop();

/*
 * collector_register_metric - Register a metric for background sampling
 *
 * PARAMETERS:
 *   name: Metric name string. Must match names in plugin_common.cpp:
 *         - "cpu_load" - System CPU percentage (0-100)
 *         - "mem_free" - Available memory in MB
 *         Must not be NULL. String is copied internally.
 *
 *   multiplicator: Sampling frequency multiplier relative to base_interval_seconds.
 *                  - 1.0 = sample every base interval (e.g., every 1 second)
 *                  - 0.5 = sample twice per interval (e.g., every 0.5 seconds)
 *                  - 2.0 = sample every other interval (e.g., every 2 seconds)
 *                  Must be > 0. Typical value: 1.0
 *
 * BEHAVIOR:
 *   - Registers metric name in internal registry
 *   - If metric already registered, updates its multiplicator
 *   - Metric sampling begins on next collector thread tick
 *   - Initial max_samples is 1000 (change with collector_set_max_samples)
 *
 * RETURN VALUE:
 *   0 on success
 *   1 if name is NULL
 *
 * THREAD SAFETY: Safe to call from multiple threads
 *
 * CALLED BY: main.go main(), classic_wrapper.c zbx_module_init()
 *
 * ERRORS:
 *   Returns 1 if name is NULL. No logging performed.
 */
extern "C" int collector_register_metric(const char *name, double multiplicator);

/*
 * collector_set_max_samples - Configure auto-reset threshold for a metric
 *
 * PARAMETERS:
 *   name: Metric name (must be already registered via collector_register_metric).
 *         Must not be NULL.
 *
 *   max_samples: Maximum number of samples before accumulator auto-resets.
 *                When count reaches this limit, statistics reset to empty state.
 *                Prevents unbounded memory growth for long-running agents.
 *                Must be > 0. Typical values: 100-10000 depending on interval.
 *                Default: 1000
 *
 * BEHAVIOR:
 *   - Updates the max_samples threshold for specified metric
 *   - If metric doesn't exist, it is created with default multiplicator=1.0
 *   - Takes effect immediately (next sample checks the new limit)
 *
 * RETURN VALUE:
 *   0 on success
 *   1 if name is NULL
 *
 * THREAD SAFETY: Safe to call from multiple threads
 *
 * CALLED BY: main.go main()
 *
 * ERRORS:
 *   Returns 1 if name is NULL. No logging performed.
 */
extern "C" int collector_set_max_samples(const char *name, unsigned max_samples);

/*
 * collector_fetch_and_reset_json - Fetch aggregated statistics and reset accumulator
 *
 * PARAMETERS:
 *   name: Metric name (must be registered). Must not be NULL.
 *
 *   result: Output buffer for JSON string (null-terminated).
 *           Must not be NULL. Caller allocates.
 *
 *   result_len: Size of result buffer in bytes. Must be >= ~200 bytes for typical output.
 *               Recommended: 4096 bytes to accommodate per-core data (future feature).
 *
 * BEHAVIOR:
 *   - Locks the metric's accumulator
 *   - Computes all statistics (avg, min, max, median, mode, stddev, variance)
 *   - Generates JSON output string
 *   - Resets accumulator to empty state (count=0, ready for next period)
 *   - Unlocks the accumulator
 *   - This is an atomic fetch-and-reset operation
 *
 * OUTPUT FORMAT:
 *   Success (count > 0):
 *     {"metric":"cpu_load","values":{"all":{"avg":45.2,"min":12.3,"max":89.7,"med":43.1,"mod":45,"dev":15.6,"var":243.4,"cnt":120}}}
 *   
 *   No data (count = 0):
 *     {"metric":"cpu_load","values":{"all":{"avg":null,"min":null,"max":null,"med":null,"mod":null,"dev":null,"var":null,"cnt":0}}}
 *
 * RETURN VALUE:
 *   0 on success (JSON written to result, null-terminated)
 *   1 if name is NULL, metric not found, or result buffer too small
 *
 * THREAD SAFETY: Safe to call from multiple threads (per-metric locking)
 *
 * CALLED BY: main.go Export() for each metric query
 *
 * ERRORS:
 *   Returns 1 on any error. No logging performed. Result buffer may contain partial data.
 *   Common causes:
 *   - name is NULL
 *   - metric not registered (missing collector_register_metric call)
 *   - result buffer too small (increase result_len)
 */
extern "C" int collector_fetch_and_reset_json(const char *name, char *result, unsigned result_len);
