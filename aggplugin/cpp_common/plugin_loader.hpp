/*
 * plugin_loader.hpp - Measurement Plugin Loader Interface
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 15, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Provides C++ interface for registering and calling dynamically loaded measurement
 *   plugin DLLs. Integrates with the collector to enable periodic sampling of plugin
 *   metrics alongside built-in metrics.
 *
 * ARCHITECTURE:
 *   Go (main.go) → LoadLibrary/GetProcAddress → Registers plugin with C++ via CGO
 *   → C++ collector calls plugin_sample_from_dll() → Calls plugin DLL's plugin_collect()
 *
 * RELATION TO OTHER CODE:
 *   - Implemented in: plugin_loader.cpp
 *   - Used by: plugin_common.cpp (plugin_sample_numeric checks plugin registry)
 *            : main.go (registers plugins via register_measurement_plugin)
 */

#pragma once

#include <cstdint>
#include <cstddef>

/*
 * multi_source_result_t - Result structure for multi-source metric collection
 *
 * PURPOSE:
 *   Enables a single metric to return multiple values from different sources
 *   (e.g., disk.io.read returns one value per physical disk)
 *
 * MEMBERS:
 *   source_id: Identifier for this source (e.g., "0", "1" for disks, "core0", "core1" for CPUs)
 *              NULL for single-source metrics
 *              Caller must free() after use
 *   
 *   value: The measured value for this source
 *
 * LIFECYCLE:
 *   - Allocated by plugin_sample_multi_from_dll()
 *   - Caller must free source_id strings and array after use
 */
struct multi_source_result_t {
    char* source_id;  // Source identifier (NULL for single-source)
    double value;     // Measured value
};

// Plugin API function pointer types (matching measurement_plugin_api.h)
typedef size_t (*plugin_collect_func_t)(void* results);

/*
 * register_measurement_plugin - Register a loaded measurement plugin with the collector
 *
 * PARAMETERS:
 *   metric_key: The metric key name (e.g., "cpu_load", "mem_free")
 *   dll_handle: The DLL handle from LoadLibrary/dlopen
 *   collect_func: Function pointer to plugin's plugin_collect() function
 *   key_index: Index of this key in the plugin's key array (0-based)
 *
 * BEHAVIOR:
 *   - Stores plugin information in internal registry
 *   - plugin_sample_numeric() will check this registry before using built-in metrics
 *
 * RETURN VALUE:
 *   0 on success
 *   1 if metric_key is NULL or already registered
 *
 * THREAD SAFETY: Must be called before collector sampling starts (not thread-safe)
 *
 * CALLED BY: main.go via CGO after loading plugin DLLs
 */
extern "C" int register_measurement_plugin(
    const char* metric_key,
    void* dll_handle,
    plugin_collect_func_t collect_func,
    size_t key_index
);

/*
 * unregister_all_plugins - Clear all registered plugins
 *
 * BEHAVIOR:
 *   - Clears internal plugin registry
 *   - Does NOT unload DLLs (caller's responsibility)
 *
 * THREAD SAFETY: Must be called after collector stops (not thread-safe)
 *
 * CALLED BY: main.go via CGO during shutdown
 */
extern "C" void unregister_all_plugins();

/*
 * plugin_sample_from_dll - Sample a metric from a registered plugin DLL
 *
 * PARAMETERS:
 *   metric_key: The metric key name
 *   ok: Output parameter (1=success, 0=failure)
 *
 * RETURN VALUE:
 *   Measured value from plugin (if *ok==1)
 *   0.0 (if *ok==0, indicating error or metric not found)
 *
 * BEHAVIOR:
 *   - Looks up plugin by metric_key
 *   - Calls plugin's plugin_collect() function
 *   - Extracts value for the specific key_index
 *   - For multi-source metrics, returns only FIRST value (data loss!)
 *   - Returns COLLECT_ERROR if plugin returns non-OK status
 *
 * DEPRECATED: Use plugin_sample_multi_from_dll() for multi-source metrics
 *
 * THREAD SAFETY: Safe (uses mutex-protected registry)
 *
 * CALLED BY: plugin_common.cpp plugin_sample_numeric()
 */
double plugin_sample_from_dll(const char* metric_key, int* ok);

/*
 * plugin_sample_multi_from_dll - Sample multi-source metric from plugin DLL
 *
 * PARAMETERS:
 *   metric_key: The metric key name (e.g., "disk.io.read")
 *   result_count: Output parameter for number of sources returned
 *   ok: Output parameter (1=success, 0=failure)
 *
 * RETURN VALUE:
 *   Pointer to array of multi_source_result_t structures (NULL if *ok==0)
 *   Caller must free() each source_id and the array itself
 *
 * BEHAVIOR:
 *   - Looks up plugin by metric_key
 *   - Calls plugin's plugin_collect() function
 *   - Extracts ALL values for the specific key_index
 *   - Allocates multi_source_result_t array with source_ids
 *   - Returns NULL if plugin not found or collection failed
 *
 * EXAMPLE:
 *   size_t count;
 *   int ok;
 *   multi_source_result_t* results = plugin_sample_multi_from_dll("disk.io.read", &count, &ok);
 *   if (ok) {
 *       for (size_t i = 0; i < count; i++) {
 *           printf("Disk %s: %f\n", results[i].source_id, results[i].value);
 *           free(results[i].source_id);
 *       }
 *       free(results);
 *   }
 *
 * THREAD SAFETY: Safe (uses mutex-protected registry)
 *
 * CALLED BY: collector.cpp collector_loop() for metric sampling
 */
multi_source_result_t* plugin_sample_multi_from_dll(
    const char* metric_key,
    size_t* result_count,
    int* ok
);
