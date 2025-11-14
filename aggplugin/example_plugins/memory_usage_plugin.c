/*
 * memory_usage_plugin.c - Memory Usage Measurement Plugin
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 14, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Measures available physical memory using Windows GlobalMemoryStatusEx() API.
 *   This is the plugin version of the built-in mem_free metric, demonstrating
 *   how to migrate built-in metrics to the plugin framework.
 *
 * COMPILATION (MinGW64):
 *   gcc -shared -o memory_usage_plugin.dll memory_usage_plugin.c -Wall -O2
 *
 * DEPLOYMENT:
 *   1. Copy memory_usage_plugin.dll to C:\Zabbix\plugins\measurements\
 *   2. Add to aggplugin.conf:
 *      [plugin.memory_usage]
 *      enabled=1
 *
 * METRICS EXPOSED:
 *   - mem_free - Available physical memory in megabytes (MB)
 *
 * NOTES:
 *   - Uses Windows GlobalMemoryStatusEx() API
 *   - Returns available physical memory (not used memory)
 *   - Thread-safe: No static state, API is thread-safe
 *   - Can be extended to return total, used, percent, etc.
 */

#include "../cpp_common/measurement_plugin_api.h"
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* ========================================================================
 * HELPER FUNCTIONS
 * ======================================================================== */

/**
 * Sample available physical memory in megabytes
 * 
 * ALGORITHM:
 *   1. Initialize MEMORYSTATUSEX structure
 *   2. Call GlobalMemoryStatusEx() to get memory info
 *   3. Extract ullAvailPhys (available physical memory in bytes)
 *   4. Convert bytes to megabytes
 * 
 * RETURN: true if successful, false if API call fails
 */
static bool sample_mem_free_mb(double *out_mb) {
    MEMORYSTATUSEX ms;
    memset(&ms, 0, sizeof(ms));
    ms.dwLength = sizeof(ms);  // Required by API
    
    if (!GlobalMemoryStatusEx(&ms)) {
        fprintf(stderr, "MemoryUsagePlugin: GlobalMemoryStatusEx() failed (error %lu)\n",
                GetLastError());
        return false;
    }
    
    // Convert bytes to megabytes
    *out_mb = (double)ms.ullAvailPhys / (1024.0 * 1024.0);
    return true;
}

/* ========================================================================
 * PLUGIN METADATA
 * ======================================================================== */

static const metric_key_info_t plugin_keys[] = {
    {
        "mem_free",
        "Available physical memory in megabytes (MB)",
        METRIC_TYPE_FLOAT,
        "",
        0  // has_multiple_sources = false (system-wide metric)
    }
};

static const plugin_info_t plugin_metadata = {
    "MemoryUsage",
    "1.0",
    "Claude Sonnet 4.5 (AI) + Lukas Plachy (lukas@plachy.eu)",
    sizeof(plugin_keys) / sizeof(metric_key_info_t),
    plugin_keys
};

/* ========================================================================
 * REQUIRED PLUGIN FUNCTIONS
 * ======================================================================== */

/**
 * plugin_get_info - Returns plugin metadata
 */
PLUGIN_EXPORT const plugin_info_t* plugin_get_info() {
    return &plugin_metadata;
}

/**
 * plugin_init - Initialize plugin
 * 
 * No special initialization needed for memory sampling.
 * GlobalMemoryStatusEx() doesn't require setup.
 */
PLUGIN_EXPORT int plugin_init(const char* config_section) {
    printf("MemoryUsagePlugin: Initialized\n");
    
    // Optional: Parse configuration
    // Example: Check if config contains "units=GB" to return gigabytes
    
    return 0;  // Success
}

/**
 * plugin_collect - Collect memory usage measurement
 * 
 * Returns single value (available physical memory in MB).
 */
PLUGIN_EXPORT size_t plugin_collect(collection_result_t* results) {
    double mem_free_mb = 0.0;
    
    if (!sample_mem_free_mb(&mem_free_mb)) {
        // Sampling error
        results[0].key = "mem_free";
        results[0].status = COLLECT_ERROR;
        results[0].value_count = 0;
        results[0].values = NULL;
        return 1;
    }
    
    // Allocate measurement value
    measurement_value_t* value = (measurement_value_t*)malloc(sizeof(measurement_value_t));
    if (!value) {
        fprintf(stderr, "MemoryUsagePlugin: Memory allocation failed\n");
        results[0].key = "mem_free";
        results[0].status = COLLECT_ERROR;
        results[0].value_count = 0;
        results[0].values = NULL;
        return 1;
    }
    
    value->source_id = NULL;  // System-wide metric (no source distinction)
    value->value = mem_free_mb;
    value->str_value = NULL;
    
    // Fill result
    results[0].key = "mem_free";
    results[0].status = COLLECT_OK;
    results[0].value_count = 1;
    results[0].values = value;
    
    return 1;  // One result
}

/**
 * plugin_deinit - Clean up plugin resources
 * 
 * No resources to clean up for memory sampling.
 */
PLUGIN_EXPORT int plugin_deinit() {
    printf("MemoryUsagePlugin: Deinitialized\n");
    return 0;
}

/* ========================================================================
 * OPTIONAL ENHANCEMENTS
 * ======================================================================== */

/*
 * EXTENSION IDEAS:
 * 
 * 1. Multiple metrics in one plugin:
 *    - mem_free: Available memory
 *    - mem_used: Used memory (total - available)
 *    - mem_total: Total physical memory
 *    - mem_percent: Memory usage percentage
 * 
 * 2. Configuration options:
 *    - units=MB|GB|KB: Output unit
 *    - metric=free|used|total|percent: Which metric to return
 * 
 * 3. Extended memory info:
 *    - Virtual memory statistics
 *    - Page file usage
 *    - Memory load percentage
 * 
 * Example multi-metric implementation:
 * 
 * static const metric_key_info_t plugin_keys[] = {
 *     {"mem.free", "Free memory", METRIC_TYPE_FLOAT, "", 0},
 *     {"mem.used", "Used memory", METRIC_TYPE_FLOAT, "", 0},
 *     {"mem.total", "Total memory", METRIC_TYPE_FLOAT, "", 0},
 *     {"mem.percent", "Memory usage %", METRIC_TYPE_FLOAT, "", 0}
 * };
 * 
 * size_t plugin_collect(collection_result_t* results) {
 *     MEMORYSTATUSEX ms;
 *     ms.dwLength = sizeof(ms);
 *     GlobalMemoryStatusEx(&ms);
 *     
 *     double free_mb = ms.ullAvailPhys / (1024.0 * 1024.0);
 *     double total_mb = ms.ullTotalPhys / (1024.0 * 1024.0);
 *     double used_mb = total_mb - free_mb;
 *     double percent = (used_mb / total_mb) * 100.0;
 *     
 *     // Allocate 4 values
 *     measurement_value_t* vals = malloc(sizeof(measurement_value_t) * 4);
 *     
 *     vals[0] = {NULL, free_mb, NULL};
 *     vals[1] = {NULL, used_mb, NULL};
 *     vals[2] = {NULL, total_mb, NULL};
 *     vals[3] = {NULL, percent, NULL};
 *     
 *     results[0] = {"mem.free", COLLECT_OK, 1, &vals[0]};
 *     results[1] = {"mem.used", COLLECT_OK, 1, &vals[1]};
 *     results[2] = {"mem.total", COLLECT_OK, 1, &vals[2]};
 *     results[3] = {"mem.percent", COLLECT_OK, 1, &vals[3]};
 *     
 *     return 4;
 * }
 */
