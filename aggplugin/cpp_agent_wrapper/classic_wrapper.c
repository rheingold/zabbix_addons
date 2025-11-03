/*
 * classic_wrapper.c - Zabbix Classic Agent Loadable Module Wrapper
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Implements the Classic Zabbix Agent loadable module API (zbx_module_*).
 *   Integrates the C++ collector engine with Zabbix Classic Agent (zabbix_agentd).
 *
 * RELATION TO OTHER CODE:
 *   - Uses: collector.hpp (collector_* functions via extern declarations)
 *         : collector.cpp (linked statically or via DLL)
 *         : plugin_common.cpp (metric sampling, linked statically)
 *   - Loaded by: Zabbix Classic Agent via LoadModule directive
 *
 * DEPLOYMENT:
 *   1. Build: build_cpp.ps1 -Variant classic → build/aggplugin_classic.dll
 *   2. Config: zabbix_agentd.conf:
 *      LoadModule=C:\Zabbix\plugins\aggplugin_classic.dll
 *   3. Runtime: Agent loads DLL, calls zbx_module_init()
 *   4. Queries: Agent calls metric_cpu_load() or metric_mem_free() on each request
 *
 * ZABBIX CLASSIC AGENT MODULE API:
 *   Required functions:
 *   - zbx_module_api_version(): Return API version number
 *   - zbx_module_item_list(): Return array of ZBX_METRIC structures
 *   - zbx_module_init(): Initialize module (called once at agent startup)
 *   - zbx_module_uninit(): Cleanup module (called once at agent shutdown)
 *
 * METRICS PROVIDED:
 *   - plugintest1.cpu_load: CPU load aggregation (JSON with statistics)
 *   - plugintest1.mem_free: Memory usage aggregation (JSON with statistics)
 *
 * BUILD STATUS:
 *   Code complete, untested. Classic Agent deployment requires:
 *   - Zabbix agent classic headers (zabbixlib/include/module.h)
 *   - Proper DLL dependencies resolution
 *   - Testing with actual Zabbix Classic Agent
 *
 * THREAD MODEL:
 *   - Main thread: Zabbix agent
 *   - Background thread: Collector sampling thread (created in zbx_module_init)
 *   - Metric callbacks: Called by agent on each query (thread-safe via collector API)
 */

#ifdef __cplusplus
extern "C" {
#endif

// INCLUDE DOCUMENTATION:

#include <string.h>           // strlen, strcpy - string operations
#include <stdlib.h>           // malloc, free - memory allocation (not used currently)

// ZABBIX CLASSIC AGENT HEADERS:
// These headers define the loadable module API (ZBX_METRIC, zbx_module_*, API version)
// Headers are expected in ../../../zabbixlib/include/ (relative to build location)
// If not found, fallback definitions are provided below for compilation/analysis
#if defined(ZABBIX_CLASSIC_AGENT)
#  if defined(__has_include)
#    if __has_include("../../../zabbixlib/include/module.h")
#      include "../../../zabbixlib/include/sysinc.h"     // Zabbix system includes
#      include "../../../zabbixlib/include/module.h"     // ZBX_METRIC, zbx_module_* API
#      define HAVE_ZABBIX_HEADERS 1
#    endif
#  endif
#endif

// FALLBACK DEFINITIONS:
// If Zabbix headers not found, define minimal API for compilation
// This allows static analysis and compilation without full Zabbix SDK
#ifndef HAVE_ZABBIX_HEADERS
/*
 * ZBX_METRIC - Zabbix metric definition structure (FALLBACK)
 * 
 * MEMBERS:
 *   key: Metric key string (e.g., "plugintest1.cpu_load")
 *   function: Callback function pointer for metric sampling
 *   flags: Metric flags (0 for basic metrics)
 */
typedef struct {
    const char *key;
    int (*function)(const char *params, unsigned flags, char *result, unsigned result_len);
    unsigned flags;
} ZBX_METRIC;

/*
 * Zabbix Classic Agent module API function prototypes (FALLBACK)
 * These are the required entry points that Zabbix agent looks for when loading a module.
 */
int zbx_module_api_version(void);
ZBX_METRIC *zbx_module_item_list(void);
int zbx_module_init(void);
int zbx_module_uninit(void);
#endif // HAVE_ZABBIX_HEADERS
#endif // fallback guard

// COLLECTOR API DECLARATIONS:
// External C functions from collector.cpp (linked into this DLL)
/*
 * collector_init - Initialize background sampling thread
 * PARAMETERS: base_interval_seconds - sampling interval (e.g., 1.0)
 * CALLED BY: zbx_module_init()
 */
extern void collector_init(double base_interval_seconds);

/*
 * collector_stop - Stop background sampling thread
 * CALLED BY: zbx_module_uninit()
 */
extern void collector_stop();

/*
 * collector_register_metric - Register metric for continuous sampling
 * PARAMETERS:
 *   name - metric name (e.g., "cpu_load")
 *   multiplicator - sampling frequency multiplier (1.0 = every interval)
 * RETURN: 0=success, 1=error
 * CALLED BY: zbx_module_init()
 */
extern int collector_register_metric(const char *name, double multiplicator);

/*
 * collector_fetch_and_reset_json - Fetch aggregated statistics and reset
 * PARAMETERS:
 *   name - metric name
 *   result - output buffer (caller-allocated)
 *   result_len - buffer size
 * RETURN: 0=success, 1=error
 * CALLED BY: metric_cpu_load(), metric_mem_free()
 */
extern int collector_fetch_and_reset_json(const char *name, char *result, unsigned result_len);

// METRIC CALLBACK FUNCTIONS:
// These are called by Zabbix agent when a metric is queried

/*
 * metric_cpu_load - Zabbix callback for plugintest1.cpu_load metric
 *
 * PURPOSE:
 *   Fetches aggregated CPU load statistics from collector and returns as JSON string.
 *
 * PARAMETERS:
 *   params: Metric parameters from Zabbix query (unused, can be NULL)
 *   flags: Zabbix query flags (unused)
 *   result: Output buffer for metric value (caller-allocated)
 *   result_len: Size of result buffer in bytes
 *
 * BEHAVIOR:
 *   - Calls collector_fetch_and_reset_json("cpu_load", ...)
 *   - Returns JSON with 8 statistics: avg, min, max, med, mod, dev, var, cnt
 *   - Resets accumulator after fetch (next query starts new aggregation period)
 *
 * RETURN VALUE:
 *   0: Success, result contains JSON string
 *   1: Failure, result contains empty string
 *
 * OUTPUT EXAMPLE:
 *   {"metric":"cpu_load","values":{"all":{"avg":45.2,"min":12.3,"max":89.7,"med":43.1,"mod":45,"dev":15.6,"var":243.4,"cnt":120}}}
 *
 * CALLED BY: Zabbix agent when processing item query
 *
 * ERRORS:
 *   Returns 1 if collector_fetch_and_reset_json fails:
 *   - Metric not registered (missing collector_register_metric call)
 *   - Buffer too small
 *   - Collector not initialized
 */
int metric_cpu_load(const char *params, unsigned flags, char *result, unsigned result_len) {
    if (collector_fetch_and_reset_json("cpu_load", result, result_len) == 0) 
        return 0; // Success
    
    // Failure: return empty string
    if (result_len > 0) result[0] = '\0';
    return 1;
}

/*
 * metric_mem_free - Zabbix callback for plugintest1.mem_free metric
 *
 * PURPOSE:
 *   Fetches aggregated memory usage statistics from collector and returns as JSON string.
 *
 * PARAMETERS:
 *   params: Metric parameters from Zabbix query (unused, can be NULL)
 *   flags: Zabbix query flags (unused)
 *   result: Output buffer for metric value (caller-allocated)
 *   result_len: Size of result buffer in bytes
 *
 * BEHAVIOR:
 *   - Calls collector_fetch_and_reset_json("mem_free", ...)
 *   - Returns JSON with 8 statistics in MB: avg, min, max, med, mod, dev, var, cnt
 *   - Resets accumulator after fetch (next query starts new aggregation period)
 *
 * RETURN VALUE:
 *   0: Success, result contains JSON string
 *   1: Failure, result contains empty string
 *
 * OUTPUT EXAMPLE:
 *   {"metric":"mem_free","values":{"all":{"avg":4096.5,"min":3850.2,"max":4200.8,"med":4100.3,"mod":4096,"dev":85.4,"var":7293.2,"cnt":60}}}
 *
 * CALLED BY: Zabbix agent when processing item query
 *
 * ERRORS:
 *   Returns 1 if collector_fetch_and_reset_json fails (same as metric_cpu_load)
 */
int metric_mem_free(const char *params, unsigned flags, char *result, unsigned result_len) {
    if (collector_fetch_and_reset_json("mem_free", result, result_len) == 0) 
        return 0; // Success
    
    // Failure: return empty string
    if (result_len > 0) result[0] = '\0';
    return 1;
}

// METRIC REGISTRATION:
// Static array of metrics provided by this module
// Sentinel entry (NULL, NULL, 0) marks end of array

/*
 * item_list - Array of metrics provided by this module
 * 
 * STRUCTURE:
 *   Each ZBX_METRIC entry contains:
 *   - key: Metric key string visible to Zabbix
 *   - function: Callback function pointer
 *   - flags: Metric flags (0 = basic metric)
 *
 * REGISTERED METRICS:
 *   - plugintest1.cpu_load: CPU aggregation (metric_cpu_load callback)
 *   - plugintest1.mem_free: Memory aggregation (metric_mem_free callback)
 *
 * CALLED BY: zbx_module_item_list() to expose metrics to Zabbix agent
 */
static ZBX_METRIC item_list[] = {
    {"plugintest1.cpu_load", metric_cpu_load, 0},
    {"plugintest1.mem_free", metric_mem_free, 0},
    {NULL, NULL, 0}  // Sentinel
};

// ZABBIX MODULE API IMPLEMENTATIONS:

/*
 * zbx_module_api_version - Return Zabbix module API version
 *
 * PURPOSE:
 *   Tells Zabbix agent which API version this module implements.
 *   Agent verifies compatibility before loading module.
 *
 * RETURN VALUE:
 *   20201106: API version for Zabbix 6.x (approximate)
 *
 * CALLED BY: Zabbix agent during module loading
 *
 * NOTE:
 *   Actual API version should match target Zabbix version.
 *   This value is approximate and may need adjustment based on target agent version.
 */
int zbx_module_api_version(void) {
    return 20201106; // Zabbix 6.x API version (approximate)
}

/*
 * zbx_module_item_list - Return array of metrics provided by module
 *
 * PURPOSE:
 *   Exposes the list of metrics to Zabbix agent for registration.
 *
 * RETURN VALUE:
 *   Pointer to static item_list array (ZBX_METRIC*)
 *
 * CALLED BY: Zabbix agent during module initialization
 *
 * NOTE:
 *   Array must be null-terminated (sentinel entry with NULL key)
 */
ZBX_METRIC *zbx_module_item_list(void) {
    return item_list;
}

/*
 * zbx_module_init - Initialize module (called once at agent startup)
 *
 * PURPOSE:
 *   Sets up the collector engine and registers metrics for background sampling.
 *
 * BEHAVIOR:
 *   1. Initialize collector with 1-second sampling interval
 *   2. Register "cpu_load" metric (multiplicator 1.0 = every second)
 *   3. Register "mem_free" metric (multiplicator 1.0 = every second)
 *   4. Background sampling thread starts immediately
 *
 * RETURN VALUE:
 *   0: Success, module initialized
 *   Non-zero: Failure (not currently implemented, always returns 0)
 *
 * CALLED BY: Zabbix agent after loading DLL
 *
 * ERRORS:
 *   Currently no error handling. If collector_init or collector_register_metric fail,
 *   module continues loading but metrics may not work.
 *   
 *   Symptoms of initialization failure:
 *   - Metrics return empty results
 *   - No samples collected
 *   - Check agent log for errors (if logging implemented)
 */
int zbx_module_init(void) {
    // Start collector with base interval = 1 second
    collector_init(1.0);
    
    // Register metrics for background sampling
    collector_register_metric("cpu_load", 1.0);   // Sample CPU every second
    collector_register_metric("mem_free", 1.0);   // Sample memory every second
    
    return 0; // Success
}

/*
 * zbx_module_uninit - Cleanup module (called once at agent shutdown)
 *
 * PURPOSE:
 *   Stops the collector sampling thread and cleans up resources.
 *
 * BEHAVIOR:
 *   - Calls collector_stop() to terminate background thread
 *   - Blocks until thread has fully exited
 *   - All accumulated samples are lost (not persisted)
 *
 * RETURN VALUE:
 *   0: Success, cleanup complete
 *   Non-zero: Failure (not currently implemented, always returns 0)
 *
 * CALLED BY: Zabbix agent before unloading DLL
 *
 * THREAD SAFETY:
 *   Safe to call even if collector was never initialized (collector_stop is idempotent)
 */
int zbx_module_uninit(void) {
    collector_stop();
    return 0; // Success
}

#ifdef __cplusplus
}
#endif
