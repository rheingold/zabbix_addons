/*
 * plugin_common.cpp - Plugin Infrastructure Integration Layer
 * Zabbix Aggplugin v0.1 (tmp0.1) | December 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Provides legacy API functions and bridges metric requests to DLL plugins.
 *   All actual metric implementations have been moved to dynamically loaded DLLs.
 *
 * ARCHITECTURE:
 *   - DLL Plugin System: All metrics provided by external DLL files
 *   - Plugin Discovery: Automatic scanning and registration at startup
 *   - Runtime Sampling: Metrics collected via plugin_sample_from_dll()
 *   
 * RELATION TO OTHER CODE:
 *   - Header: plugin_common.hpp (API definitions)
 *   - DLL Interface: plugin_loader.cpp (plugin_sample_from_dll)
 *   - Used by: collector.cpp (sampling thread)
 *            : agent2_wrapper.cpp, unified_wrapper.cpp (legacy access)
 *
 * REMOVED (now in DLL plugins):
 *   - Built-in CPU sampling (was: GetSystemTimes API)
 *   - Built-in memory sampling (was: GlobalMemoryStatusEx API)
 *   - Platform-specific implementations
 *   - Global state variables for baseline tracking
 */

#include "plugin_common.hpp"  // API declarations for metric sampling functions
#include "plugin_loader.hpp"  // Plugin registry for dynamically loaded DLLs

#include <cstdio>             // snprintf - string formatting for legacy functions
#include <cstring>            // strcpy, strncpy - string operations

// All platform-specific metric sampling has been moved to DLL plugins.
// No global state or built-in implementations remain in this file.

/*
 * All metrics are now provided by dynamically loaded DLL plugins.
 * Built-in implementations removed to eliminate code duplication.
 */

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
    
    // All metrics are now provided by dynamically loaded plugin DLLs
    double plugin_value = plugin_sample_from_dll(metric, ok);
    return plugin_value;  // Returns 0.0 with *ok=0 if plugin not found
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
 * CURRENT STATUS:
 *   NOT IMPLEMENTED - Always returns 0 and writes "[]"
 *   Per-core CPU sampling would require platform-specific DLL plugin
 *
 * FUTURE:
 *   Implement as a DLL plugin using PDH API (Windows) or /proc/stat (Linux)
 */
extern "C" int plugin_sample_cpu_per_core(char *result, unsigned result_len) {
    if (!result || result_len == 0) return 0;
    strncpy(result, "[]", result_len);
    result[result_len - 1] = '\0';
    return 0;  // Not implemented
}
