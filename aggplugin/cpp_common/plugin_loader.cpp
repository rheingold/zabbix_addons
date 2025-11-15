/*
 * plugin_loader.cpp - Measurement Plugin Loader Implementation
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 15, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 */

#include "plugin_loader.hpp"
#include "collector.hpp"  // For collector_register_metric()
#include "measurement_plugin_api.h"
#include <map>
#include <string>
#include <mutex>
#include <cstring>
#include <cstdlib>

// Platform-specific includes for dynamic loading
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

// Plugin registry entry
struct PluginEntry {
    void* dll_handle;  // HMODULE on Windows, void* on Linux
    plugin_collect_func_t collect_func;
    size_t key_index;  // Which key in the plugin's array this metric corresponds to
};

// Global plugin registry: metric_key → plugin info
static std::map<std::string, PluginEntry> plugin_registry;
static std::mutex plugin_registry_mutex;

/*
 * register_measurement_plugin - Implementation (see plugin_loader.hpp for full docs)
 */
extern "C" int register_measurement_plugin(
    const char* metric_key,
    void* dll_handle,
    plugin_collect_func_t collect_func,
    size_t key_index
) {
    if (!metric_key) return 1;
    
    std::lock_guard<std::mutex> lock(plugin_registry_mutex);
    
    std::string key(metric_key);
    if (plugin_registry.find(key) != plugin_registry.end()) {
        return 1;  // Already registered
    }
    
    PluginEntry entry;
    entry.dll_handle = dll_handle;
    entry.collect_func = collect_func;
    entry.key_index = key_index;
    
    plugin_registry[key] = entry;
    
    // CRITICAL: Register with collector for background sampling!
    // multiplicator=1.0 means sample every base_interval (1 second)
    collector_register_metric(metric_key, 1.0);
    
    return 0;
}

/*
 * unregister_all_plugins - Implementation (see plugin_loader.hpp for full docs)
 */
extern "C" void unregister_all_plugins() {
    std::lock_guard<std::mutex> lock(plugin_registry_mutex);
    plugin_registry.clear();
}

/*
 * plugin_sample_from_dll - Implementation (see plugin_loader.hpp for full docs)
 */
double plugin_sample_from_dll(const char* metric_key, int* ok) {
    *ok = 0;
    if (!metric_key) return 0.0;
    
    std::lock_guard<std::mutex> lock(plugin_registry_mutex);
    
    std::string key(metric_key);
    auto it = plugin_registry.find(key);
    if (it == plugin_registry.end()) {
        return 0.0;  // Not a plugin metric
    }
    
    PluginEntry& entry = it->second;
    
    // Get the plugin_collect function from the DLL
    typedef size_t (*plugin_collect_func)(void*);
    plugin_collect_func collect = nullptr;
    
    #ifdef _WIN32
        collect = (plugin_collect_func)GetProcAddress((HMODULE)entry.dll_handle, "plugin_collect");
    #else
        collect = (plugin_collect_func)dlsym(entry.dll_handle, "plugin_collect");
    #endif
    
    if (!collect) {
        return 0.0;  // Function not found in DLL
    }
    
    // Allocate results array (assume max 10 keys per plugin for simplicity)
    collection_result_t results[10];
    memset(results, 0, sizeof(results));
    
    // Call plugin_collect from DLL
    size_t result_count = 0;
    try {
        result_count = collect(results);
    } catch (...) {
        return 0.0;  // Plugin crashed or threw exception
    }
    
    if (result_count == 0) {
        return 0.0;
    }
    
    // Find the result for our key_index
    for (size_t i = 0; i < result_count && i < 10; i++) {
        if (results[i].status != COLLECT_OK) continue;
        if (results[i].value_count == 0) continue;
        
        // Assume this is our key (in order)
        if (i == entry.key_index) {
            // For now, just return first value - multi-source support needs collector changes
            // TODO: Return all values with source_ids to collector for proper multi-source aggregation
            if (results[i].values) {
                *ok = 1;
                double value = results[i].values[0].value;
                
                // Free source_id strings and values array
                for (size_t v = 0; v < results[i].value_count; v++) {
                    if (results[i].values[v].source_id) {
                        free((void*)results[i].values[v].source_id);
                    }
                }
                free(results[i].values);
                
                return value;
            }
        }
    }
    
    return 0.0;
}

/*
 * plugin_sample_multi_from_dll - Implementation (see plugin_loader.hpp for full docs)
 */
multi_source_result_t* plugin_sample_multi_from_dll(
    const char* metric_key,
    size_t* result_count,
    int* ok
) {
    *ok = 0;
    *result_count = 0;
    if (!metric_key) return nullptr;
    
    std::lock_guard<std::mutex> lock(plugin_registry_mutex);
    
    std::string key(metric_key);
    auto it = plugin_registry.find(key);
    if (it == plugin_registry.end()) {
        return nullptr;  // Not a plugin metric
    }
    
    PluginEntry& entry = it->second;
    
    // Get the plugin_collect function from the DLL
    typedef size_t (*plugin_collect_func)(void*);
    plugin_collect_func collect = nullptr;
    
    #ifdef _WIN32
        collect = (plugin_collect_func)GetProcAddress((HMODULE)entry.dll_handle, "plugin_collect");
    #else
        collect = (plugin_collect_func)dlsym(entry.dll_handle, "plugin_collect");
    #endif
    
    if (!collect) {
        return nullptr;  // Function not found in DLL
    }
    
    // Allocate results array (assume max 10 keys per plugin for simplicity)
    collection_result_t results[10];
    memset(results, 0, sizeof(results));
    
    // Call plugin_collect from DLL
    size_t plugin_result_count = 0;
    try {
        plugin_result_count = collect(results);
    } catch (...) {
        return nullptr;  // Plugin crashed or threw exception
    }
    
    if (plugin_result_count == 0) {
        return nullptr;
    }
    
    // Find the result for our key_index
    for (size_t i = 0; i < plugin_result_count && i < 10; i++) {
        if (results[i].status != COLLECT_OK) continue;
        if (results[i].value_count == 0) continue;
        
        // Check if this is our key
        if (i == entry.key_index) {
            if (!results[i].values) continue;
            
            // Allocate return array
            multi_source_result_t* ret = (multi_source_result_t*)malloc(
                sizeof(multi_source_result_t) * results[i].value_count);
            
            if (!ret) {
                // Cleanup and fail
                for (size_t v = 0; v < results[i].value_count; v++) {
                    if (results[i].values[v].source_id) {
                        free((void*)results[i].values[v].source_id);
                    }
                }
                free(results[i].values);
                return nullptr;
            }
            
            // Copy all values with source IDs
            for (size_t v = 0; v < results[i].value_count; v++) {
                ret[v].value = results[i].values[v].value;
                
                // Transfer ownership of source_id (or create default)
                if (results[i].values[v].source_id) {
                    ret[v].source_id = (char*)results[i].values[v].source_id;
                } else {
                    // Single-source metric: use empty string
                    ret[v].source_id = (char*)malloc(1);
                    ret[v].source_id[0] = '\0';
                }
            }
            
            // Free the original values array (source_ids transferred)
            free(results[i].values);
            
            *result_count = results[i].value_count;
            *ok = 1;
            return ret;
        }
    }
    
    return nullptr;
}
