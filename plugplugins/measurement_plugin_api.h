/*
 * measurement_plugin_api.h - Measurement Plugin Interface Specification
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 14, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Defines the C API for loadable measurement plugins. Measurement plugins are
 *   lightweight DLLs that implement specific metric collection (e.g., disk I/O,
 *   network stats, custom hardware sensors). The aggplugin framework loads these
 *   plugins dynamically and aggregates their measurements over time.
 *
 * PLUGIN WORKFLOW:
 *   1. Aggplugin scans configured plugin directory for DLLs
 *   2. Loads each DLL and calls plugin_get_info() to register metrics
 *   3. Calls plugin_init() once at startup
 *   4. Calls plugin_collect() periodically to gather measurements
 *   5. Aggregates results using built-in or plugin-provided aggregation
 *   6. Calls plugin_deinit() on shutdown
 *
 * DESIGN PRINCIPLES:
 *   - Simple C interface (no C++ complexity)
 *   - Compatible with Zabbix agent datatypes and conventions
 *   - Support multiple keys per plugin
 *   - Handle dynamic sources (e.g., CPU cores, disks appearing/disappearing)
 *   - Minimal boilerplate for plugin authors
 *   - Optional custom aggregation override
 *
 * THREAD SAFETY:
 *   - plugin_collect() may be called from background sampling thread
 *   - plugin_on_reset() may be called from query thread
 *   - Plugins must handle concurrent calls if needed
 *
 * EXAMPLE PLUGIN:
 *   See: examples/disk_stats_plugin.c
 */

#ifndef MEASUREMENT_PLUGIN_API_H
#define MEASUREMENT_PLUGIN_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* ========================================================================
 * DATA TYPES - Aligned with Zabbix Agent conventions
 * ======================================================================== */

/**
 * Metric value types (compatible with Zabbix agent item types)
 */
typedef enum {
    METRIC_TYPE_FLOAT   = 0,  // Floating point number
    METRIC_TYPE_UINT64  = 1,  // Unsigned 64-bit integer
    METRIC_TYPE_STRING  = 2,  // Text string
    METRIC_TYPE_TEXT    = 3,  // Long text (not aggregated)
    METRIC_TYPE_LOG     = 4   // Log entry (not aggregated)
} metric_value_type_t;

/**
 * Collection result status
 */
typedef enum {
    COLLECT_OK          = 0,  // Success
    COLLECT_NOTSUPPORTED= 1,  // Metric not supported on this system
    COLLECT_ERROR       = 2,  // Collection failed (temporary)
    COLLECT_TIMEOUT     = 3   // Collection timed out
} collect_status_t;

/**
 * Single measurement value with source identifier
 * 
 * For metrics with multiple sources (e.g., per-CPU, per-disk):
 * - source_id: Unique identifier for the source (e.g., "cpu0", "sda")
 * - value: The measured value
 * 
 * For single-source metrics, source_id can be NULL or empty string.
 */
typedef struct {
    const char* source_id;    // Source identifier (optional, can be NULL)
    double value;             // Numeric value (for FLOAT/UINT64 types)
    const char* str_value;    // String value (for STRING/TEXT/LOG types, optional)
} measurement_value_t;

/**
 * Collection result for a single metric key
 * 
 * A plugin's plugin_collect() function returns one of these per metric.
 */
typedef struct {
    const char* key;          // Metric key (must match registered key)
    collect_status_t status;  // Collection status
    size_t value_count;       // Number of values returned (0 if status != OK)
    measurement_value_t* values; // Array of measured values (can have multiple sources)
} collection_result_t;

/* ========================================================================
 * PLUGIN METADATA
 * ======================================================================== */

/**
 * Metric key information - describes a single metric exposed by plugin
 */
typedef struct {
    const char* key;          // Metric key (e.g., "disk.io.read[*]")
    const char* description;  // Human-readable description
    metric_value_type_t type; // Value type
    const char* parameters;   // Parameter specification (e.g., "device" for disk[device])
    int has_multiple_sources; // 1 if metric has multiple sources (per-cpu, per-disk, etc.)
} metric_key_info_t;

/**
 * Plugin information structure - returned by plugin_get_info()
 */
typedef struct {
    const char* name;         // Plugin name (e.g., "DiskStats")
    const char* version;      // Plugin version (e.g., "1.0")
    const char* author;       // Plugin author
    size_t key_count;         // Number of metric keys exposed
    const metric_key_info_t* keys; // Array of metric keys
} plugin_info_t;

/* ========================================================================
 * REQUIRED PLUGIN FUNCTIONS (must be exported by DLL)
 * ======================================================================== */

/**
 * plugin_get_info - Returns plugin metadata and registers metric keys
 * 
 * WHEN CALLED: Once during plugin loading
 * RETURN: Pointer to static plugin_info_t structure
 * 
 * Example:
 *   static metric_key_info_t my_keys[] = {
 *       {"disk.io.read", "Disk read operations/sec", METRIC_TYPE_UINT64, "device", 1},
 *       {"disk.io.write", "Disk write operations/sec", METRIC_TYPE_UINT64, "device", 1}
 *   };
 *   static plugin_info_t info = {"DiskStats", "1.0", "author@example.com", 2, my_keys};
 *   return &info;
 */
typedef const plugin_info_t* (*plugin_get_info_func)();

/**
 * plugin_init - Initialize plugin
 * 
 * WHEN CALLED: Once after plugin is loaded
 * PARAMETERS:
 *   config_section: Plugin-specific configuration string from aggplugin.conf
 *                   Format: key=value pairs, one per line
 * RETURN: 0 on success, non-zero on failure
 * 
 * Example config section in aggplugin.conf:
 *   [plugin.diskstats]
 *   devices=sda,sdb,sdc
 *   interval=1.0
 */
typedef int (*plugin_init_func)(const char* config_section);

/**
 * plugin_collect - Collect measurements for all registered keys
 * 
 * WHEN CALLED: Periodically (every sampling interval) from background thread
 * PARAMETERS:
 *   results: Output array (pre-allocated by caller, one per registered key)
 * RETURN: Number of results filled (should equal key_count)
 * 
 * NOTES:
 *   - Called from background sampling thread
 *   - Should complete quickly (< 100ms typical)
 *   - Can return multiple values per key (for multi-source metrics)
 *   - Caller owns memory for results array
 *   - Plugin must allocate values array (caller will free it)
 * 
 * Example:
 *   // For multi-source metric (per-disk):
 *   measurement_value_t* vals = malloc(sizeof(measurement_value_t) * 3);
 *   vals[0] = {"sda", 1234.5, NULL};
 *   vals[1] = {"sdb", 567.8, NULL};
 *   vals[2] = {"sdc", 890.1, NULL};
 *   results[0] = {"disk.io.read", COLLECT_OK, 3, vals};
 */
typedef size_t (*plugin_collect_func)(collection_result_t* results);

/**
 * plugin_deinit - Clean up plugin resources
 * 
 * WHEN CALLED: Once during shutdown
 * RETURN: 0 on success, non-zero on error (logged but not fatal)
 */
typedef int (*plugin_deinit_func)();

/* ========================================================================
 * OPTIONAL PLUGIN FUNCTIONS (may be exported by DLL)
 * ======================================================================== */

/**
 * plugin_on_reset - Called when aggregated data is fetched and reset
 * 
 * WHEN CALLED: After aggplugin returns aggregated metrics to Zabbix
 * PARAMETERS:
 *   key: The metric key that was reset
 * 
 * NOTES:
 *   - Optional: Plugin can implement if it needs to know when data is consumed
 *   - Called from query thread (different from plugin_collect thread)
 *   - Useful for counter resets, rate calculations, etc.
 * 
 * Example use case:
 *   Plugin tracks cumulative disk I/O bytes. On reset, it can update
 *   its baseline for rate calculations.
 */
typedef void (*plugin_on_reset_func)(const char* key);

/**
 * plugin_aggregate_custom - Override default aggregation logic
 * 
 * WHEN CALLED: During aggregation (when computing statistics)
 * PARAMETERS:
 *   key: Metric key
 *   source_id: Source identifier (NULL for single-source metrics)
 *   values: Array of collected values over time
 *   count: Number of values
 *   result: Output buffer for aggregated statistics (avg/min/max/etc.)
 * RETURN: 0 to use plugin aggregation, non-zero to use default aggregation
 * 
 * NOTES:
 *   - ADVANCED FEATURE: Most plugins should use default aggregation
 *   - Useful for custom statistics (e.g., percentiles, weighted averages)
 *   - Leave undefined or return non-zero to use default aggregation
 *   - Must be thread-safe (called from query thread)
 * 
 * WARNING: Experimental feature - API may change
 * RECOMMENDED: Comment out in example plugins unless specifically needed
 */
typedef int (*plugin_aggregate_custom_func)(
    const char* key,
    const char* source_id,
    const double* values,
    size_t count,
    void* result  // Actually aggr_statistics_t*, but avoid circular dependency
);

/**
 * plugin_on_init_complete - Called after all plugins are initialized
 * 
 * WHEN CALLED: Once after all plugins have been loaded and initialized
 * 
 * NOTES:
 *   - Optional: Plugin can implement if it needs to coordinate with other plugins
 *   - All plugins' plugin_init() have completed successfully at this point
 */
typedef void (*plugin_on_init_complete_func)();

/* ========================================================================
 * HELPER MACROS FOR PLUGIN IMPLEMENTATION
 * ======================================================================== */

/**
 * Export plugin function with proper DLL export decorations
 * 
 * Usage in plugin DLL:
 *   PLUGIN_EXPORT const plugin_info_t* plugin_get_info() { ... }
 */
#ifdef _WIN32
    #define PLUGIN_EXPORT __declspec(dllexport)
#else
    #define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

/**
 * Define plugin metadata (convenience macro)
 * 
 * Example:
 *   PLUGIN_DEFINE_KEYS_BEGIN
 *       PLUGIN_KEY("disk.io.read", "Disk reads/sec", METRIC_TYPE_UINT64, "device", 1)
 *       PLUGIN_KEY("disk.io.write", "Disk writes/sec", METRIC_TYPE_UINT64, "device", 1)
 *   PLUGIN_DEFINE_KEYS_END(DiskStats)
 */
#define PLUGIN_DEFINE_KEYS_BEGIN \
    static const metric_key_info_t plugin_keys[] = {

#define PLUGIN_KEY(k, desc, t, params, multi) \
    {k, desc, t, params, multi},

#define PLUGIN_DEFINE_KEYS_END(name) \
    }; \
    static const plugin_info_t plugin_metadata = { \
        #name, "1.0", "Generated", \
        sizeof(plugin_keys) / sizeof(metric_key_info_t), \
        plugin_keys \
    };

/* ========================================================================
 * PLUGIN DISCOVERY AND LOADING (used by aggplugin framework)
 * ======================================================================== */

/**
 * Plugin handle (opaque structure for loaded plugin)
 */
typedef struct plugin_handle_s plugin_handle_t;

/**
 * Load plugin from DLL file
 * 
 * PARAMETERS:
 *   dll_path: Path to plugin DLL
 *   config_section: Configuration string for this plugin (from aggplugin.conf)
 * RETURN: Plugin handle on success, NULL on failure
 * 
 * NOTE: This function is part of the aggplugin framework, not implemented by plugins
 */
plugin_handle_t* plugin_load(const char* dll_path, const char* config_section);

/**
 * Unload plugin and free resources
 * 
 * PARAMETERS:
 *   handle: Plugin handle from plugin_load()
 * 
 * NOTE: This function is part of the aggplugin framework, not implemented by plugins
 */
void plugin_unload(plugin_handle_t* handle);

/**
 * Get plugin info from loaded plugin
 * 
 * PARAMETERS:
 *   handle: Plugin handle
 * RETURN: Plugin info structure
 */
const plugin_info_t* plugin_get_info_loaded(plugin_handle_t* handle);

/**
 * Collect measurements from loaded plugin
 * 
 * PARAMETERS:
 *   handle: Plugin handle
 *   results: Output array (pre-allocated)
 * RETURN: Number of results filled
 */
size_t plugin_collect_loaded(plugin_handle_t* handle, collection_result_t* results);

#ifdef __cplusplus
}
#endif

#endif /* MEASUREMENT_PLUGIN_API_H */
