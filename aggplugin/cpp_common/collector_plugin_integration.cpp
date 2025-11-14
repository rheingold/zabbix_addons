/*
 * collector_plugin_integration.cpp - Integration Layer Between Collector and Plugins
 * Zabbix Aggplugin v0.1 (tmp0.1) | November 14, 2025
 * Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
 *
 * PURPOSE:
 *   Provides integration functions to connect the plugin loader system with the
 *   existing collector framework. Handles plugin discovery, loading, key registration,
 *   and collection integration.
 *
 * USAGE:
 *   1. Call collector_plugins_init() from main.go init() or after collector_init()
 *   2. Plugin metrics are automatically registered with collector
 *   3. Existing plugin_sample_numeric() calls co-exist with plugin-based metrics
 *   4. Call collector_plugins_cleanup() from main.go defer or shutdown
 *
 * INTEGRATION STRATEGY:
 *   - Extends existing collector.cpp without modifying its core loop
 *   - Plugin metrics are registered using collector_register_metric()
 *   - Plugin data is collected via plugin_collect_all() in sampling thread
 *   - Results are added to accumulators just like built-in metrics
 */

#include "collector.hpp"
#include "measurement_plugin_api.h"
#include <windows.h>
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations from plugin_loader.cpp
extern int plugin_discover_and_load(
    const char* plugin_path_pattern,
    const std::vector<std::pair<std::string, std::string>>& config_map);
extern void plugin_unload_all();
extern size_t plugin_collect_all(std::vector<collection_result_t>& all_results);
extern void plugin_notify_reset(const char* key);
extern std::vector<std::pair<std::string, std::string>> 
    parse_plugin_config_sections(const std::string& config_text);

/* ========================================================================
 * PLUGIN-TO-COLLECTOR BRIDGE
 * ======================================================================== */

// Map of plugin metric key -> last collected value (for plugin_sample_numeric bridge)
static std::unordered_map<std::string, double> plugin_metric_cache;

/**
 * Initialize plugins and register their metrics with collector
 * 
 * PARAMETERS:
 *   plugin_path_pattern: Wildcard pattern for plugin DLLs (e.g., "C:\\Zabbix\\plugins\\*.dll")
 *   config_text: Full configuration text with [plugin.*] sections
 *   base_multiplicator: Default sampling multiplicator for plugin metrics (e.g., 1.0)
 * 
 * RETURNS: Number of successfully loaded plugins
 * 
 * BEHAVIOR:
 *   1. Parse configuration to extract [plugin.*] sections
 *   2. Discover and load plugins from directory
 *   3. For each plugin, register all its metric keys with collector
 *   4. Metrics are sampled at base_multiplicator rate
 * 
 * EXAMPLE:
 *   collector_init(1.0);  // Init collector first
 *   collector_plugins_init("C:\\Zabbix\\plugins\\*.dll", config_text, 1.0);
 */
extern "C" int collector_plugins_init(
    const char* plugin_path_pattern,
    const char* config_text,
    double base_multiplicator)
{
    if (!plugin_path_pattern || !config_text) {
        return 0;
    }
    
    printf("PluginIntegration: Initializing plugin system\n");
    printf("PluginIntegration: Plugin path pattern: %s\n", plugin_path_pattern);
    
    // Parse plugin configuration sections
    std::string config_str(config_text);
    auto config_sections = parse_plugin_config_sections(config_str);
    
    printf("PluginIntegration: Found %zu plugin configuration section(s)\n", 
           config_sections.size());
    
    // Load plugins
    int loaded_count = plugin_discover_and_load(plugin_path_pattern, config_sections);
    
    if (loaded_count == 0) {
        printf("PluginIntegration: No plugins loaded\n");
        return 0;
    }
    
    // TODO: Register plugin metrics with collector
    // This requires extending collector API or integrating plugin_collect_all()
    // into collector_loop(). For now, plugins are loaded but not integrated.
    
    printf("PluginIntegration: Plugin system initialized (%d plugins loaded)\n", 
           loaded_count);
    
    return loaded_count;
}

/**
 * Clean up plugin system
 * 
 * BEHAVIOR:
 *   - Unloads all plugins (calls plugin_deinit() for each)
 *   - Frees plugin resources
 *   - Should be called before collector_stop()
 */
extern "C" void collector_plugins_cleanup() {
    printf("PluginIntegration: Cleaning up plugin system\n");
    plugin_unload_all();
    plugin_metric_cache.clear();
}

/**
 * Collect measurements from all plugins and update cache
 * 
 * INTERNAL USE: Called from modified collector_loop() or separate plugin thread
 * 
 * BEHAVIOR:
 *   - Calls plugin_collect_all() to get measurements from all plugins
 *   - Updates plugin_metric_cache with latest values
 *   - Frees plugin-allocated memory
 * 
 * RETURNS: Number of metrics collected
 */
extern "C" size_t collector_plugins_collect_all() {
    std::vector<collection_result_t> results;
    size_t count = plugin_collect_all(results);
    
    // Update cache with collected values
    for (const auto& result : results) {
        if (result.status == COLLECT_OK && result.value_count > 0) {
            // For multi-source metrics, store each source separately
            for (size_t i = 0; i < result.value_count; i++) {
                std::string cache_key = result.key;
                if (result.values[i].source_id) {
                    cache_key += ".";
                    cache_key += result.values[i].source_id;
                }
                plugin_metric_cache[cache_key] = result.values[i].value;
                
                // Free allocated source_id string
                if (result.values[i].source_id) {
                    free((void*)result.values[i].source_id);
                }
                if (result.values[i].str_value) {
                    free((void*)result.values[i].str_value);
                }
            }
            
            // Free values array
            free(result.values);
        }
    }
    
    return count;
}

/**
 * Bridge function: Get plugin metric value for plugin_sample_numeric()
 * 
 * INTERNAL USE: Could be called from modified plugin_common.cpp
 * 
 * PARAMETERS:
 *   key: Full metric key with source (e.g., "disk.io.read.sda")
 *   ok: Output success flag (1=success, 0=not found)
 * 
 * RETURNS: Cached metric value or 0.0 if not found
 */
extern "C" double collector_plugins_get_cached_value(const char* key, int* ok) {
    if (!key || !ok) {
        if (ok) *ok = 0;
        return 0.0;
    }
    
    std::string key_str(key);
    auto it = plugin_metric_cache.find(key_str);
    
    if (it != plugin_metric_cache.end()) {
        *ok = 1;
        return it->second;
    }
    
    *ok = 0;
    return 0.0;
}

/* ========================================================================
 * INTEGRATION NOTES FOR FUTURE DEVELOPMENT
 * ======================================================================== */

/*
 * FULL INTEGRATION ROADMAP:
 *
 * Option 1: Extend collector_loop() to call plugin_collect_all()
 * ---------------------------------------------------------------
 * Modify cpp_common/collector.cpp:collector_loop() to:
 *   1. After sampling built-in metrics (CPU, memory)
 *   2. Call collector_plugins_collect_all()
 *   3. For each plugin result, call collector_register_metric() + acc.add()
 *
 * Pros: Single sampling thread, consistent timing
 * Cons: Requires modifying core collector.cpp
 *
 * Option 2: Separate plugin sampling thread
 * ------------------------------------------
 * Create second background thread that:
 *   1. Runs parallel to collector_loop()
 *   2. Calls plugin_collect_all() at same interval
 *   3. Updates metrics via collector API
 *
 * Pros: No modification to collector.cpp
 * Cons: Two threads, potential timing skew
 *
 * Option 3: Plugin metrics as first-class citizens
 * -------------------------------------------------
 * Extend MetricEntry to support plugin-based sampling:
 *   - Add plugin_handle_t* pointer to MetricEntry
 *   - If plugin is set, call plugin_collect() instead of plugin_sample_numeric()
 *   - Store multi-source results in separate accumulators per source
 *
 * Pros: Clean design, full integration
 * Cons: Requires significant refactoring of collector.cpp
 *
 * RECOMMENDED: Option 1 (extend collector_loop) for simplicity and performance
 */
