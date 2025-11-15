/*
 * plugin_common.cpp - Platform-Specific System Metric Sampling Implementation
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Implements low-level system metric sampling using platform-specific APIs.
 *   Provides the data source for the collector engine's continuous aggregation.
 *
 * ARCHITECTURE:
 *   Windows Implementation:
 *     - CPU: GetSystemTimes() API with delta calculation method
 *     - Memory: GlobalMemoryStatusEx() API for memory stats
 *     - Per-core CPU: PDH API (planned, not yet implemented)
 *   
 *   Non-Windows Implementation:
 *     - Returns dummy values for testing/compilation
 *     - TODO: Linux implementation via /proc/stat, /proc/meminfo
 *
 * RELATION TO OTHER CODE:
 *   - Header: plugin_common.hpp (API definitions)
 *   - Used by: collector.cpp (calls plugin_sample_numeric from sampling thread)
 *            : agent2_wrapper.cpp, unified_wrapper.cpp (legacy direct access)
 *
 * CPU SAMPLING METHOD (Windows):
 *   GetSystemTimes() returns cumulative CPU time counters:
 *     - idleTime: Time spent idle (100ns units)
 *     - kernelTime: Time spent in kernel mode (includes idle)
 *     - userTime: Time spent in user mode
 *   
 *   Algorithm:
 *     1. Convert FILETIME structures to 64-bit integers
 *     2. Calculate total = kernel + user (kernel already includes idle)
 *     3. On first call: Store baseline (idle, total), return failure
 *     4. On subsequent calls:
 *        - Calculate deltas: delta_idle, delta_total
 *        - CPU% = (delta_total - delta_idle) / delta_total * 100
 *        - Update baseline for next call
 *   
 *   Accuracy: System-wide CPU percentage, averaged over time between calls
 *
 * MEMORY SAMPLING METHOD (Windows):
 *   GlobalMemoryStatusEx() returns MEMORYSTATUSEX structure:
 *     - ullAvailPhys: Available physical memory in bytes
 *   Convert to megabytes: bytes / (1024.0 * 1024.0)
 *
 * THREAD SAFETY:
 *   - Global state variables (cpu_has_prev, prev_idle, prev_total) are not protected
 *   - Assumes single-threaded sampling (collector thread only)
 *   - If multiple threads sample CPU, results will be inconsistent
 *   - TODO: Add mutex protection for multi-threaded CPU sampling
 */

// INCLUDE DOCUMENTATION:

#include "plugin_common.hpp"  // API declarations for metric sampling functions
#include "plugin_loader.hpp"  // Plugin registry for dynamically loaded DLLs

// C++ Standard Library includes:
#include <string>             // std::string - metric name comparison
#include <cstdio>             // snprintf - string formatting for legacy functions
#include <cstring>            // strcpy, strncpy - string operations
#include <vector>             // std::vector<double> - per-core CPU data (future feature)

// Platform-specific includes:
#ifdef _WIN32
#  include <windows.h>        // Windows API: GetSystemTimes(), GlobalMemoryStatusEx(), FILETIME
#  include <pdh.h>            // Performance Data Helper (PDH) API for per-core CPU (not yet used)
#  include <pdhmsg.h>         // PDH message definitions
#  pragma comment(lib, "pdh.lib")  // Link PDH library
#endif

// GLOBAL STATE VARIABLES (Windows CPU sampling):

#ifdef _WIN32
/*
 * cpu_has_prev - Flag indicating if CPU baseline has been established
 * TYPE: bool
 * DEFAULT: false
 * USAGE:
 *   - Set to true after first GetSystemTimes() call
 *   - Checked to determine if delta calculation is possible
 * THREAD SAFETY: NOT thread-safe (assumes single sampling thread)
 */
static bool cpu_has_prev = false;

/*
 * prev_idle - Previous idle time counter for delta calculation
 * TYPE: ULONGLONG (64-bit unsigned integer)
 * UNIT: 100-nanosecond intervals (Windows FILETIME units)
 * USAGE:
 *   - Updated on each successful CPU sample
 *   - Used to calculate delta_idle for CPU percentage
 * THREAD SAFETY: NOT thread-safe (assumes single sampling thread)
 */
static ULONGLONG prev_idle = 0;

/*
 * prev_total - Previous total CPU time counter for delta calculation
 * TYPE: ULONGLONG (64-bit unsigned integer)
 * UNIT: 100-nanosecond intervals (Windows FILETIME units)
 * FORMULA: total = kernelTime + userTime
 * USAGE:
 *   - Updated on each successful CPU sample
 *   - Used to calculate delta_total for CPU percentage
 * THREAD SAFETY: NOT thread-safe (assumes single sampling thread)
 */
static ULONGLONG prev_total = 0;

// INTERNAL HELPER FUNCTIONS (Windows-specific):

/*
 * filetime_to_ull - Convert Windows FILETIME to 64-bit integer
 *
 * PURPOSE:
 *   Windows FILETIME is a structure with two 32-bit values (dwLowDateTime, dwHighDateTime).
 *   This function combines them into a single 64-bit value for arithmetic operations.
 *
 * PARAMETERS:
 *   ft: Windows FILETIME structure (const FILETIME&)
 *
 * RETURN VALUE:
 *   64-bit unsigned integer representing 100-nanosecond intervals since January 1, 1601
 *
 * ALGORITHM:
 *   Shift high 32 bits left, OR with low 32 bits
 *
 * CALLED BY: sample_cpu_load_percent()
 */
static inline ULONGLONG filetime_to_ull(const FILETIME &ft)
{
    return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

/*
 * sample_cpu_load_percent - Sample overall system CPU usage percentage
 *
 * PURPOSE:
 *   Queries Windows GetSystemTimes() API and calculates CPU percentage using
 *   delta method (difference between consecutive samples).
 *
 * PARAMETERS:
 *   out_percent: Output parameter for CPU percentage (double&)
 *                Range: 0.0 to 100.0
 *
 * RETURN VALUE:
 *   true: Sample successful, out_percent contains valid percentage
 *   false: Sample failed or baseline not yet established
 *
 * BEHAVIOR:
 *   First call:
 *     - Calls GetSystemTimes() to get idle, kernel, user times
 *     - Stores values in prev_idle, prev_total
 *     - Sets cpu_has_prev = true
 *     - Returns false (no percentage yet, baseline established)
 *   
 *   Subsequent calls:
 *     - Calls GetSystemTimes() to get new idle, kernel, user times
 *     - Calculates deltas: delta_idle, delta_total
 *     - Computes CPU% = (delta_total - delta_idle) / delta_total * 100
 *     - Clamps result to 0-100 range
 *     - Updates prev_idle, prev_total for next call
 *     - Returns true with percentage in out_percent
 *
 * WINDOWS API:
 *   GetSystemTimes(FILETIME *idle, FILETIME *kernel, FILETIME *user)
 *   - idle: Time system spent idle
 *   - kernel: Time spent in kernel mode (includes idle time)
 *   - user: Time spent in user mode
 *   - Returns FALSE on error
 *
 * CALLED BY: plugin_sample_numeric() when metric == "cpu_load"
 *
 * THREAD SAFETY: NOT thread-safe (uses static variables without locking)
 *
 * ERRORS:
 *   Returns false if:
 *   - GetSystemTimes() fails (API error)
 *   - First call (baseline establishment)
 *   - delta_total == 0 (no time elapsed between samples)
 */
static bool sample_cpu_load_percent(double &out_percent)
{
    FILETIME idleTime{}, kernelTime{}, userTime{};
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return false; // Windows API call failed

    ULONGLONG idle = filetime_to_ull(idleTime);
    ULONGLONG kernel = filetime_to_ull(kernelTime);
    ULONGLONG user = filetime_to_ull(userTime);

    // Note: kernel time includes idle time, total = kernel + user
    ULONGLONG total = kernel + user;

    if (!cpu_has_prev) {
        // First call: establish baseline
        prev_idle = idle;
        prev_total = total;
        cpu_has_prev = true;
        return false; // No data yet (need two samples for delta)
    }

    // Calculate deltas
    ULONGLONG delta_idle = idle - prev_idle;
    ULONGLONG delta_total = total - prev_total;
    
    // Update baseline for next call
    prev_idle = idle;
    prev_total = total;

    if (delta_total == 0)
        return false; // No time elapsed (should not happen in practice)

    // Calculate CPU percentage: busy_time / total_time * 100
    double busy = static_cast<double>(delta_total - delta_idle);
    out_percent = (busy * 100.0) / static_cast<double>(delta_total);
    
    // Clamp to valid range (shouldn't exceed 100 for system-wide, but defensive)
    if (out_percent < 0.0) out_percent = 0.0;
    if (out_percent > 100.0) out_percent = 100.0;
    
    return true;
}

/*
 * sample_cpu_load_per_core - Sample per-core CPU loads (NOT IMPLEMENTED)
 *
 * PURPOSE:
 *   Would sample CPU load for each processor core individually using PDH API.
 *
 * PARAMETERS:
 *   out_cores: Output vector for per-core percentages (std::vector<double>&)
 *              Index 0 = core 0, index 1 = core 1, etc.
 *
 * RETURN VALUE:
 *   true: Successfully sampled all cores, out_cores filled
 *   false: Not implemented or error
 *
 * CURRENT STATUS:
 *   Always returns false (not implemented)
 *
 * IMPLEMENTATION PLAN:
 *   1. Initialize PDH query on first call
 *   2. Add counters for each processor core
 *      Problem: Counter names are locale-dependent
 *      English: "\\Processor(0)\\% Processor Time"
 *      Czech: "\\Procesor(0)\\% času procesoru" (example)
 *   3. Query PDH for current values
 *   4. Store in out_cores vector
 *
 * CHALLENGES:
 *   - PDH counter names vary by Windows locale
 *   - Must use PdhLookupPerfNameByIndex() or detect locale
 *   - Requires PDH query handle lifecycle management
 *
 * CALLED BY: plugin_sample_cpu_per_core() API function
 *
 * TODO: Implement using PDH API with locale-aware counter paths
 */
static bool sample_cpu_load_per_core(std::vector<double> &out_cores) {
    out_cores.clear();
    return false; // Not implemented yet
}

/*
 * sample_mem_free_mb - Sample available physical memory in megabytes
 *
 * PURPOSE:
 *   Queries Windows GlobalMemoryStatusEx() API for memory statistics.
 *
 * PARAMETERS:
 *   out_mb: Output parameter for available memory in MB (double&)
 *
 * RETURN VALUE:
 *   true: Sample successful, out_mb contains available memory
 *   false: Sample failed (API error)
 *
 * BEHAVIOR:
 *   - Initializes MEMORYSTATUSEX structure (dwLength must be set)
 *   - Calls GlobalMemoryStatusEx()
 *   - Extracts ullAvailPhys (available physical memory in bytes)
 *   - Converts to MB: bytes / (1024.0 * 1024.0)
 *   - Stores result in out_mb
 *
 * WINDOWS API:
 *   GlobalMemoryStatusEx(MEMORYSTATUSEX *lpBuffer)
 *   - Returns TRUE on success, FALSE on failure
 *   - Fills lpBuffer->ullAvailPhys with available physical memory (bytes)
 *   - Also provides: total physical, total virtual, available virtual, etc.
 *
 * CALLED BY: plugin_sample_numeric() when metric == "mem_free"
 *
 * THREAD SAFETY: Safe (no static state, API is thread-safe)
 *
 * ERRORS:
 *   Returns false if GlobalMemoryStatusEx() fails (rare, API error)
 */
static bool sample_mem_free_mb(double &out_mb)
{
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms); // Required by API
    if (!GlobalMemoryStatusEx(&ms))
        return false; // Windows API call failed
    
    // Convert bytes to megabytes
    out_mb = static_cast<double>(ms.ullAvailPhys) / (1024.0 * 1024.0);
    return true;
}
#endif // _WIN32

// API FUNCTION IMPLEMENTATIONS:
// See plugin_common.hpp for full API documentation

/*
 * plugin_sample_numeric - Implementation (see plugin_common.hpp for full docs)
 *
 * IMPLEMENTATION DETAILS:
 *   - Validates metric name (not NULL)
 *   - Converts to std::string for comparison
 *   - Platform-specific branches:
 *     Windows: Calls sample_cpu_load_percent() or sample_mem_free_mb()
 *     Non-Windows: Returns dummy values for testing
 *   - Sets *ok based on sampling success
 *
 * CALLED BY: collector.cpp collector_loop() for each metric sample
 *          : plugin_get_cpu_load(), plugin_get_memory_usage() (legacy)
 *
 * SPECIAL BEHAVIOR:
 *   CPU sampling: First call returns *ok=0 (baseline), subsequent calls return *ok=1
 *   Memory sampling: Always returns *ok=1 (no baseline needed)
 *
 * ERRORS:
 *   *ok=0 on:
 *   - metric is NULL
 *   - Unsupported metric name
 *   - Platform API failure
 *   - First CPU sample (baseline establishment)
 */
extern "C" double plugin_sample_numeric(const char *metric, int *ok) {
    *ok = 0; // Default: failure
    if (!metric) return 0.0;
    
    // FIRST: Check if this metric is provided by a loaded plugin DLL
    double plugin_value = plugin_sample_from_dll(metric, ok);
    if (*ok) {
        return plugin_value;  // Plugin handled it successfully
    }
    
    // FALLBACK: Use built-in metric implementations
    std::string m(metric);

#ifdef _WIN32
    if (m == "cpu_load" || m == "_internal.cpu_load") {
        double pct = 0.0;
        if (sample_cpu_load_percent(pct)) {
            *ok = 1;
            return pct; // Percentage 0.0-100.0
        }
        return 0.0; // First sample (baseline) or API error
    }
    if (m == "mem_free" || m == "_internal.mem_free") {
        double mb = 0.0;
        if (sample_mem_free_mb(mb)) {
            *ok = 1;
            return mb; // Megabytes of available physical memory
        }
        return 0.0; // API error
    }
#else
    // Non-Windows fallback: Return dummy values for testing/compilation
    if (m == "cpu_load" || m == "_internal.cpu_load") { *ok = 1; return 10.0; }
    if (m == "mem_free" || m == "_internal.mem_free") { *ok = 1; return 2048.0; }
#endif

    return 0.0; // Unsupported metric
}

// LEGACY FUNCTIONS: String-based metric access for Zabbix module interface
// These use static buffers and are not thread-safe for return value usage

/*
 * cpu_load_buffer - Static buffer for CPU load string formatting
 * TYPE: char[64]
 * USAGE: Stores formatted CPU percentage string (e.g., "45.23")
 * THREAD SAFETY: NOT thread-safe (shared static buffer)
 */
static char cpu_load_buffer[64];

/*
 * memory_usage_buffer - Static buffer for memory usage string formatting
 * TYPE: char[64]
 * USAGE: Stores formatted memory value string (e.g., "4096.50")
 * THREAD SAFETY: NOT thread-safe (shared static buffer)
 */
static char memory_usage_buffer[64];

/*
 * plugin_get_cpu_load - Implementation (see plugin_common.hpp for full docs)
 *
 * IMPLEMENTATION DETAILS:
 *   - Calls plugin_sample_numeric("cpu_load", &ok)
 *   - Formats result to static buffer using snprintf("%.2f")
 *   - Returns pointer to static buffer
 *   - If sampling fails (*ok==0), returns "0.00"
 *
 * CALLED BY: agent2_wrapper.cpp, unified_wrapper.cpp Zabbix module functions
 *
 * LEGACY NOTE:
 *   This function is for direct metric access without aggregation.
 *   For continuous aggregation, use collector API instead.
 */
extern "C" const char* plugin_get_cpu_load() {
    int ok = 0;
    double value = plugin_sample_numeric("cpu_load", &ok);
    if (ok) {
        snprintf(cpu_load_buffer, sizeof(cpu_load_buffer), "%.2f", value);
    } else {
        strcpy(cpu_load_buffer, "0.00");
    }
    return cpu_load_buffer;
}

/*
 * plugin_get_memory_usage - Implementation (see plugin_common.hpp for full docs)
 *
 * IMPLEMENTATION DETAILS:
 *   - Calls plugin_sample_numeric("mem_free", &ok)
 *   - Formats result to static buffer using snprintf("%.2f")
 *   - Returns pointer to static buffer
 *   - If sampling fails (*ok==0), returns "0.00"
 *
 * CALLED BY: agent2_wrapper.cpp, unified_wrapper.cpp Zabbix module functions
 *
 * LEGACY NOTE:
 *   This function is for direct metric access without aggregation.
 *   For continuous aggregation, use collector API instead.
 */
extern "C" const char* plugin_get_memory_usage() {
    int ok = 0;
    double value = plugin_sample_numeric("mem_free", &ok);
    if (ok) {
        snprintf(memory_usage_buffer, sizeof(memory_usage_buffer), "%.2f", value);
    } else {
        strcpy(memory_usage_buffer, "0.00");
    }
    return memory_usage_buffer;
}

/*
 * plugin_sample_cpu_per_core - Implementation (see plugin_common.hpp for full docs)
 *
 * IMPLEMENTATION DETAILS:
 *   - Validates result buffer (not NULL, len > 0)
 *   - Calls sample_cpu_load_per_core() (currently returns false - not implemented)
 *   - If successful: Builds JSON array [{"core":N,"value":X.XX},...]
 *   - If failed: Returns empty JSON array "[]"
 *   - Ensures null-termination
 *
 * CURRENT STATUS:
 *   Always returns 0 and writes "[]" (per-core not implemented)
 *
 * CALLED BY: Accumulator::fetch_and_reset_json() when metric=="cpu_load"
 *
 * FUTURE:
 *   Will return number of cores and fill result with JSON array
 */
extern "C" int plugin_sample_cpu_per_core(char *result, unsigned result_len) {
    if (!result || result_len == 0) return 0;
    
#ifdef _WIN32
    std::vector<double> cores;
    if (!sample_cpu_load_per_core(cores) || cores.empty()) {
        // Not implemented or no data: return empty array
        strncpy(result, "[]", result_len);
        result[result_len - 1] = '\0';
        return 0;
    }
    
    // Build JSON array: [{"core":0,"value":12.34},{"core":1,"value":23.45},...]
    std::string json = "[";
    for (size_t i = 0; i < cores.size(); i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"core\":%zu,\"value\":%.2f}", i, cores[i]);
        if (i > 0) json += ",";
        json += buf;
    }
    json += "]";
    
    strncpy(result, json.c_str(), result_len);
    result[result_len - 1] = '\0';
    return static_cast<int>(cores.size());
#else
    // Non-Windows: return empty array
    strncpy(result, "[]", result_len);
    result[result_len - 1] = '\0';
    return 0;
#endif
}
