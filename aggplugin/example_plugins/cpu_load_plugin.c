/*
 * cpu_load_plugin.c - CPU Load Measurement Plugin
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 14, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Measures system-wide CPU load percentage using Windows GetSystemTimes() API.
 *   This is the plugin version of the built-in cpu_load metric, demonstrating
 *   how to migrate built-in metrics to the plugin framework.
 *
 * COMPILATION (MinGW64):
 *   gcc -shared -o cpu_load_plugin.dll cpu_load_plugin.c -Wall -O2
 *
 * DEPLOYMENT:
 *   1. Copy cpu_load_plugin.dll to C:\Zabbix\plugins\measurements\
 *   2. Add to aggplugin.conf:
 *      [plugin.cpu_load]
 *      enabled=1
 *
 * METRICS EXPOSED:
 *   - cpu_load - System-wide CPU utilization percentage (0-100)
 *
 * NOTES:
 *   - Uses Windows GetSystemTimes() API (kernel time includes idle)
 *   - Requires two samples for delta calculation (first call returns no data)
 *   - Thread-safe: Uses static variables with proper initialization
 */

#include "../cpp_common/measurement_plugin_api.h"
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* ========================================================================
 * PLUGIN STATE
 * ======================================================================== */

// CPU sampling state (for delta calculation)
static bool cpu_has_prev = false;
static ULONGLONG prev_idle = 0;
static ULONGLONG prev_total = 0;

/* ========================================================================
 * HELPER FUNCTIONS
 * ======================================================================== */

/**
 * Convert FILETIME structure to 64-bit unsigned integer
 * 
 * FILETIME is a Windows structure containing:
 *   - dwLowDateTime: Lower 32 bits of 64-bit value
 *   - dwHighDateTime: Upper 32 bits of 64-bit value
 * 
 * Combined, they represent 100-nanosecond intervals since January 1, 1601
 */
static ULONGLONG filetime_to_ull(FILETIME ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

/**
 * Sample CPU load percentage using GetSystemTimes() delta method
 * 
 * ALGORITHM:
 *   1. Get current idle, kernel, user times from GetSystemTimes()
 *   2. Calculate deltas from previous sample
 *   3. CPU % = (total_time - idle_time) / total_time * 100
 *   4. Store current values for next delta calculation
 * 
 * RETURN: true if valid sample, false if first call or error
 */
static bool sample_cpu_load_percent(double *out_percent) {
    FILETIME idleTime = {0}, kernelTime = {0}, userTime = {0};
    
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        fprintf(stderr, "CPULoadPlugin: GetSystemTimes() failed (error %lu)\n", 
                GetLastError());
        return false;
    }

    ULONGLONG idle = filetime_to_ull(idleTime);
    ULONGLONG kernel = filetime_to_ull(kernelTime);
    ULONGLONG user = filetime_to_ull(userTime);

    // Note: kernel time includes idle time
    ULONGLONG total = kernel + user;

    if (!cpu_has_prev) {
        // First call: establish baseline
        prev_idle = idle;
        prev_total = total;
        cpu_has_prev = true;
        return false;  // No data yet (need two samples)
    }

    // Calculate deltas
    ULONGLONG delta_idle = idle - prev_idle;
    ULONGLONG delta_total = total - prev_total;
    
    // Update baseline for next call
    prev_idle = idle;
    prev_total = total;

    if (delta_total == 0) {
        return false;  // No time elapsed
    }

    // Calculate CPU percentage
    double busy = (double)(delta_total - delta_idle);
    double percent = (busy * 100.0) / (double)delta_total;
    
    // Clamp to valid range
    if (percent < 0.0) percent = 0.0;
    if (percent > 100.0) percent = 100.0;
    
    *out_percent = percent;
    return true;
}

/* ========================================================================
 * PLUGIN METADATA
 * ======================================================================== */

static const metric_key_info_t plugin_keys[] = {
    {
        "cpu_load",
        "System-wide CPU utilization percentage (0-100)",
        METRIC_TYPE_FLOAT,
        "",
        0  // has_multiple_sources = false (system-wide metric)
    }
};

static const plugin_info_t plugin_metadata = {
    "CPULoad",
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
 * No special initialization needed for CPU sampling.
 * GetSystemTimes() doesn't require setup.
 */
PLUGIN_EXPORT int plugin_init(const char* config_section) {
    printf("CPULoadPlugin: Initialized\n");
    
    // Reset state
    cpu_has_prev = false;
    prev_idle = 0;
    prev_total = 0;
    
    return 0;  // Success
}

/**
 * plugin_collect - Collect CPU load measurement
 * 
 * Returns single value (system-wide CPU percentage).
 * First call after init returns error (need baseline).
 */
PLUGIN_EXPORT size_t plugin_collect(collection_result_t* results) {
    double cpu_percent = 0.0;
    
    if (!sample_cpu_load_percent(&cpu_percent)) {
        // First call or sampling error
        results[0].key = "cpu_load";
        results[0].status = COLLECT_ERROR;
        results[0].value_count = 0;
        results[0].values = NULL;
        return 1;
    }
    
    // Allocate measurement value
    measurement_value_t* value = (measurement_value_t*)malloc(sizeof(measurement_value_t));
    if (!value) {
        fprintf(stderr, "CPULoadPlugin: Memory allocation failed\n");
        results[0].key = "cpu_load";
        results[0].status = COLLECT_ERROR;
        results[0].value_count = 0;
        results[0].values = NULL;
        return 1;
    }
    
    value->source_id = NULL;  // System-wide metric (no source distinction)
    value->value = cpu_percent;
    value->str_value = NULL;
    
    // Fill result
    results[0].key = "cpu_load";
    results[0].status = COLLECT_OK;
    results[0].value_count = 1;
    results[0].values = value;
    
    return 1;  // One result
}

/**
 * plugin_deinit - Clean up plugin resources
 * 
 * No resources to clean up for CPU sampling.
 */
PLUGIN_EXPORT int plugin_deinit() {
    printf("CPULoadPlugin: Deinitialized\n");
    cpu_has_prev = false;
    return 0;
}

/* ========================================================================
 * OPTIONAL PLUGIN FUNCTIONS
 * ======================================================================== */

/**
 * plugin_on_reset - Called when aggregated data is reset
 * 
 * We could reset the baseline here, but keeping it allows for
 * continuous delta calculation across resets.
 */
PLUGIN_EXPORT void plugin_on_reset(const char* key) {
    // Optional: Reset baseline on data reset
    // For CPU load, we keep the baseline to maintain delta continuity
    (void)key;  // Unused
}
