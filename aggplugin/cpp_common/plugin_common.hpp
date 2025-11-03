/*
 * plugin_common.hpp - Platform-Specific Metric Sampling Header
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Defines API for platform-specific system metric sampling (CPU, memory, etc.).
 *   Provides low-level metric collection that is called by the collector engine
 *   for continuous background aggregation.
 *
 * RELATION TO OTHER CODE:
 *   - Implemented in: plugin_common.cpp (Windows-specific using Win32 APIs)
 *   - Used by: collector.cpp (calls plugin_sample_numeric from sampling thread)
 *            : agent2_wrapper.cpp, unified_wrapper.cpp (legacy direct metric functions)
 *
 * PLATFORM SUPPORT:
 *   - Windows: Full implementation using GetSystemTimes(), GlobalMemoryStatusEx()
 *   - Linux: Not implemented (returns dummy values for testing)
 *   - Future: Per-core CPU via PDH API (currently stubbed)
 *
 * METRICS SUPPORTED:
 *   - "cpu_load": System-wide CPU percentage (0-100)
 *   - "mem_free": Available physical memory (MB)
 *
 * THREAD SAFETY:
 *   All functions are thread-safe. Multiple threads can call concurrently.
 */

#pragma once

#include <cstddef>

/*
 * plugin_sample_numeric - Sample a system metric by name
 *
 * PARAMETERS:
 *   metric: Metric name string. Supported values:
 *           - "cpu_load": Overall system CPU usage percentage (0-100)
 *           - "mem_free": Available physical memory in megabytes
 *           Must not be NULL.
 *
 *   ok: Output parameter for success/failure indication (int* must not be NULL)
 *       - Set to 1 on successful sample
 *       - Set to 0 on failure (unsupported metric, API error, first CPU sample baseline)
 *
 * RETURN VALUE:
 *   Metric value (double):
 *   - "cpu_load": percentage 0.0-100.0 (may exceed 100 on multicore systems in some calculations)
 *   - "mem_free": megabytes (e.g., 4096.5)
 *   - If *ok==0, return value is undefined (typically 0.0)
 *
 * BEHAVIOR:
 *   - Validates metric name
 *   - Calls platform-specific sampling function
 *   - Sets *ok based on sampling success
 *   - Returns sampled value
 *
 * SPECIAL CASES:
 *   CPU sampling requires two calls to establish baseline (GetSystemTimes delta method):
 *   - First call: Establishes baseline, returns *ok=0
 *   - Second+ calls: Returns actual percentage, *ok=1
 *
 * THREAD SAFETY: Safe (platform APIs are thread-safe, static variables use proper locking)
 *
 * CALLED BY: collector.cpp collector_loop() for each metric sample
 *
 * ERRORS:
 *   *ok=0 indicates failure. Causes:
 *   - metric is NULL or unrecognized
 *   - Platform API call failed (GetSystemTimes, GlobalMemoryStatusEx)
 *   - First CPU sample (baseline establishment)
 *   No logging performed.
 */
extern "C" double plugin_sample_numeric(const char *metric, int *ok);

/*
 * plugin_sample_cpu_per_core - Sample per-core CPU loads (FUTURE FEATURE)
 *
 * PARAMETERS:
 *   result: Output buffer for JSON array (caller-allocated)
 *   result_len: Buffer size in bytes
 *
 * OUTPUT FORMAT (when implemented):
 *   JSON array: [{"core":0,"value":12.34},{"core":1,"value":23.45},...]
 *
 * RETURN VALUE:
 *   Number of cores sampled (> 0 on success, 0 on failure or not implemented)
 *
 * CURRENT STATUS:
 *   NOT IMPLEMENTED - Always returns 0 (false)
 *   TODO: Use Windows PDH (Performance Data Helper) API
 *   Challenge: PDH counter names are locale-dependent
 *
 * THREAD SAFETY: Safe (currently no-op)
 *
 * CALLED BY: Accumulator::fetch_and_reset_json() when metric=="cpu_load"
 *
 * ERRORS:
 *   Returns 0 (not implemented). No logging performed.
 */
extern "C" int plugin_sample_cpu_per_core(char *result, unsigned result_len);

/*
 * plugin_get_cpu_load - Get current CPU load as string (LEGACY)
 *
 * PURPOSE:
 *   Legacy direct metric access for Zabbix module interface.
 *   Returns current CPU sample (not aggregated).
 *
 * RETURN VALUE:
 *   Static string buffer containing CPU percentage formatted as "%.2f"
 *   Example: "45.23" or "0.00" if no data available
 *   Buffer is reused on each call (not thread-safe for return value usage)
 *
 * BEHAVIOR:
 *   - Calls plugin_sample_numeric("cpu_load", &ok)
 *   - Formats value to static buffer
 *   - Returns pointer to static buffer
 *
 * THREAD SAFETY:
 *   Sampling is safe, but return value points to static buffer (not thread-safe)
 *
 * CALLED BY: agent2_wrapper.cpp, unified_wrapper.cpp Zabbix module functions
 *
 * USAGE NOTE:
 *   For continuous aggregation, use collector API instead of this direct function.
 */
extern "C" const char* plugin_get_cpu_load();

/*
 * plugin_get_memory_usage - Get current memory usage as string (LEGACY)
 *
 * PURPOSE:
 *   Legacy direct metric access for Zabbix module interface.
 *   Returns current memory sample (not aggregated).
 *
 * RETURN VALUE:
 *   Static string buffer containing available memory in MB formatted as "%.2f"
 *   Example: "4096.50" or "0.00" if no data available
 *   Buffer is reused on each call (not thread-safe for return value usage)
 *
 * BEHAVIOR:
 *   - Calls plugin_sample_numeric("mem_free", &ok)
 *   - Formats value to static buffer
 *   - Returns pointer to static buffer
 *
 * THREAD SAFETY:
 *   Sampling is safe, but return value points to static buffer (not thread-safe)
 *
 * CALLED BY: agent2_wrapper.cpp, unified_wrapper.cpp Zabbix module functions
 *
 * USAGE NOTE:
 *   For continuous aggregation, use collector API instead of this direct function.
 */
extern "C" const char* plugin_get_memory_usage();
