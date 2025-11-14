# Plugin Framework Architecture

## Executive Summary

The aggplugin framework has been transformed from a monolithic metric collection system into an **extensible plugin host** that supports loadable measurement plugins. This architectural enhancement enables users to add custom metrics by simply dropping DLL files into a directory, without modifying core code.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Zabbix Agent2 Process                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │  Go Wrapper (main.go)                                          │ │
│  │  - Zabbix SDK integration                                      │ │
│  │  - Named pipe protocol                                         │ │
│  │  - CGO bridge to C++                                           │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                              │                                        │
│                              ▼ (CGO calls)                            │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │  Collector Core (collector.cpp)                               │ │
│  │  - Background sampling thread                                  │ │
│  │  - Statistics aggregation                                      │ │
│  │  - Metric registration                                         │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                              │                                        │
│         ┌────────────────────┼────────────────────┐                  │
│         ▼                    ▼                    ▼                  │
│  ┌────────────┐  ┌──────────────────────┐  ┌────────────┐          │
│  │ Built-in   │  │ Plugin Integration   │  │ Future     │          │
│  │ Metrics    │  │ Layer                │  │ Extensions │          │
│  │ (CPU, RAM) │  │ (plugin_loader.cpp)  │  │            │          │
│  └────────────┘  └──────────────────────┘  └────────────┘          │
│                              │                                        │
│                              ▼                                        │
│                   ┌──────────────────────┐                           │
│                   │ Plugin API Interface │                           │
│                   │ (measurement_plugin  │                           │
│                   │  _api.h)             │                           │
│                   └──────────────────────┘                           │
│                              │                                        │
│             ┌────────────────┼────────────────┐                      │
│             ▼                ▼                ▼                      │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                │
│  │ disk_stats   │ │ network_     │ │ custom_      │                │
│  │ _plugin.dll  │ │ stats.dll    │ │ sensor.dll   │                │
│  └──────────────┘ └──────────────┘ └──────────────┘                │
│   (Loadable measurement plugins)                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Component Breakdown

### 1. **measurement_plugin_api.h**
**Purpose:** C interface specification for measurement plugins

**Key Types:**
- `metric_value_type_t`: Metric data types (FLOAT, UINT64, STRING, etc.)
- `collect_status_t`: Collection result status (OK, ERROR, NOTSUPPORTED, etc.)
- `measurement_value_t`: Single measurement with optional source ID
- `collection_result_t`: Result structure for plugin collection
- `plugin_info_t`: Plugin metadata (name, version, keys)

**Required Functions (exported by plugin DLL):**
- `plugin_get_info()`: Returns metadata, registers metric keys
- `plugin_init(config)`: Initialize plugin with configuration
- `plugin_collect(results)`: Collect measurements
- `plugin_deinit()`: Clean up resources

**Optional Functions:**
- `plugin_on_reset(key)`: Notification when data is consumed
- `plugin_aggregate_custom(...)`: Custom aggregation logic
- `plugin_on_init_complete()`: Cross-plugin coordination

**Design Philosophy:**
- Simple C interface (no C++ complexity)
- Compatible with Zabbix agent conventions
- Support multi-source metrics (per-CPU, per-disk, etc.)
- Minimal boilerplate (~100 lines typical)

---

### 2. **plugin_loader.cpp**
**Purpose:** Dynamic plugin discovery and loading system

**Key Functions:**
- `plugin_load(dll_path, config)`: Load single DLL, validate API, call init
- `plugin_unload(handle)`: Unload plugin, call deinit
- `plugin_discover_and_load(pattern, config_map)`: Scan directory, load all plugins
- `plugin_collect_all(results)`: Call collect() for all loaded plugins
- `plugin_notify_reset(key)`: Notify plugins when data is consumed
- `parse_plugin_config_sections(config_text)`: Parse `[plugin.*]` sections

**Internal Structures:**
- `plugin_handle_t`: Opaque handle storing DLL handle, function pointers, metadata
- `loaded_plugins`: Vector of all loaded plugin handles

**Features:**
- Wildcard-based discovery (e.g., `plugins/*.dll`)
- Per-plugin configuration sections
- Function validation at load time
- Error handling and logging
- Thread-safe plugin operations

---

### 3. **collector_plugin_integration.cpp**
**Purpose:** Bridge layer between plugins and existing collector

**Key Functions:**
- `collector_plugins_init(path, config, mult)`: Initialize plugin system, load plugins
- `collector_plugins_collect_all()`: Collect from all plugins, update cache
- `collector_plugins_cleanup()`: Unload all plugins
- `collector_plugins_get_cached_value(key, ok)`: Get last collected value

**Integration Strategy:**
Currently provides **bridge layer** without modifying collector.cpp core.

**Future Integration Options:**

**Option 1:** Extend `collector_loop()` (RECOMMENDED)
- Modify `collector.cpp:collector_loop()` to call `plugin_collect_all()`
- Add plugin results to accumulators alongside built-in metrics
- Pros: Single thread, consistent timing
- Cons: Requires collector.cpp modification

**Option 2:** Separate plugin thread
- Create second background thread for plugin collection
- Run parallel to collector_loop()
- Pros: No modification to collector.cpp
- Cons: Two threads, potential timing skew

**Option 3:** Plugin metrics as first-class citizens
- Extend `MetricEntry` with plugin handle pointer
- Call `plugin_collect()` instead of `plugin_sample_numeric()` for plugin metrics
- Store multi-source results in separate accumulators
- Pros: Clean design, full integration
- Cons: Significant refactoring required

---

### 4. **example_plugins/disk_stats_plugin.c**
**Purpose:** Complete, production-ready example plugin

**Demonstrates:**
- Multi-source metrics (per-disk statistics)
- Windows PDH API usage
- Dynamic disk detection (hot-plug support)
- Proper memory management
- Configuration parsing

**Metrics Exposed:**
- `disk.io.read[device]`: Disk read operations/sec per physical disk
- `disk.io.write[device]`: Disk write operations/sec per physical disk

**Source IDs:** `"0"`, `"1"`, `"2"`, ... (disk numbers)

**Build:** `gcc -shared -o disk_stats_plugin.dll disk_stats_plugin.c -lPdh -Wall -O2`

---

## Configuration Format

**aggplugin.conf**:
```ini
[Plugins]
PluginPath=C:\Zabbix\plugins\measurements\*.dll

[plugin.disk_stats]
enabled=1
devices=0,1,2
sample_rate=2.0

[plugin.network_stats]
enabled=1
interfaces=Ethernet,WiFi
```

**Parsing:**
- `[Plugins]` section: Global plugin settings (PluginPath)
- `[plugin.*]` sections: Per-plugin configuration
- Section name matches DLL filename (e.g., `disk_stats.dll` → `[plugin.disk_stats]`)
- Configuration passed to `plugin_init()` as multi-line string

---

## Plugin Lifecycle

```
1. DISCOVERY
   ├─ Scan PluginPath pattern (e.g., *.dll)
   ├─ Find matching DLL files
   └─ Parse [plugin.*] config sections

2. LOADING (per plugin)
   ├─ LoadLibrary(dll_path)
   ├─ GetProcAddress("plugin_get_info")
   ├─ Validate required functions
   ├─ Call plugin_get_info() → register keys
   └─ Call plugin_init(config_section)

3. SAMPLING (continuous loop)
   ├─ Call plugin_collect() for each plugin
   ├─ Plugin returns measurement_value_t array
   ├─ Update metric cache
   └─ Add to accumulators

4. QUERYING (on Zabbix request)
   ├─ Fetch aggregated statistics
   ├─ Call plugin_on_reset(key) (optional)
   └─ Return JSON to Zabbix

5. SHUTDOWN
   ├─ Call plugin_deinit() for each plugin
   ├─ FreeLibrary(dll_handle)
   └─ Clean up resources
```

---

## Multi-Source Metrics

**Concept:** Single metric key with multiple source identifiers

**Example:** Disk I/O plugin
- Metric Key: `disk.io.read`
- Sources: `"sda"`, `"sdb"`, `"sdc"` (disk names)
- Collection: Plugin returns 3 `measurement_value_t` entries

**Zabbix Item Setup:**
```
aggplugin.disk.io.read[sda,avg,60]  → Average for sda over 60s
aggplugin.disk.io.read[sdb,max,60]  → Max for sdb over 60s
aggplugin.disk.io.read[*,avg,60]    → Average for all disks
```

**Implementation:**
```c
// In plugin_collect():
measurement_value_t* vals = malloc(sizeof(measurement_value_t) * 3);
vals[0] = {strdup("sda"), 1234.5, NULL};  // Disk 0
vals[1] = {strdup("sdb"), 567.8, NULL};   // Disk 1
vals[2] = {strdup("sdc"), 890.1, NULL};   // Disk 2

results[0] = {"disk.io.read", COLLECT_OK, 3, vals};
```

**Cache Storage:**
- `disk.io.read.sda` → 1234.5
- `disk.io.read.sdb` → 567.8
- `disk.io.read.sdc` → 890.1

---

## Memory Management

**Allocation Rules:**
1. **Plugin allocates:**
   - `values` array (malloc)
   - `source_id` strings (strdup)
   - `str_value` strings (strdup)

2. **Framework frees:**
   - After processing each `collection_result_t`
   - Frees all `source_id` and `str_value` strings
   - Frees `values` array

**Example (plugin side):**
```c
measurement_value_t* vals = malloc(sizeof(measurement_value_t) * count);
for (int i = 0; i < count; i++) {
    vals[i].source_id = strdup(source_name);  // Framework will free
    vals[i].value = measured_value;
    vals[i].str_value = NULL;
}
results[0].values = vals;  // Framework will free
```

**Example (framework side):**
```cpp
for (size_t i = 0; i < result.value_count; i++) {
    // Process measurement...
    
    // Free plugin-allocated memory
    free((void*)result.values[i].source_id);
    free((void*)result.values[i].str_value);
}
free(result.values);
```

---

## Thread Safety

**Concurrent Calls:**
- `plugin_collect()`: Called from background sampling thread
- `plugin_on_reset()`: Called from query/response thread

**Plugin Responsibility:**
If plugin has shared state accessed by both functions, use mutexes:

```c
static CRITICAL_SECTION lock;

int plugin_init(const char* config) {
    InitializeCriticalSection(&lock);
    return 0;
}

size_t plugin_collect(collection_result_t* results) {
    EnterCriticalSection(&lock);
    // ... access shared state ...
    LeaveCriticalSection(&lock);
    return count;
}

void plugin_on_reset(const char* key) {
    EnterCriticalSection(&lock);
    // ... update shared state ...
    LeaveCriticalSection(&lock);
}

int plugin_deinit() {
    DeleteCriticalSection(&lock);
    return 0;
}
```

---

## Error Handling

**Plugin Errors:**
- Return `COLLECT_ERROR` for temporary failures (network timeout, etc.)
- Return `COLLECT_NOTSUPPORTED` for permanent failures (feature not available)
- Return `COLLECT_TIMEOUT` for collection timeout

**Framework Behavior:**
- `COLLECT_ERROR`: Skip sample, try again next interval
- `COLLECT_NOTSUPPORTED`: Log warning, may disable metric
- `COLLECT_TIMEOUT`: Log warning, try again next interval

**Logging:**
- Plugins use `fprintf(stderr, ...)` for error messages
- Framework captures stderr in debug mode
- All errors logged to Zabbix Agent2 log

---

## Performance Considerations

**Collection Speed:**
- `plugin_collect()` called every sampling interval (typically 1 second)
- Target: < 100ms per plugin
- Avoid blocking I/O, expensive calculations

**Memory Usage:**
- Each sample stored in accumulator: ~8 bytes
- 1000 samples × 8 bytes = 8 KB per metric
- Auto-reset prevents unbounded growth

**Multi-Source Impact:**
- 10 disks × 2 metrics = 20 accumulators
- 1000 samples each = 160 KB total
- Acceptable for typical deployments

---

## Future Enhancements

### 1. Plugin Auto-Discovery
- Watch plugin directory for new DLLs
- Hot-reload plugins without Agent2 restart

### 2. Plugin Dependencies
- Declare dependencies in plugin metadata
- Load plugins in dependency order

### 3. Plugin Marketplace
- Central repository of community plugins
- One-click installation

### 4. Enhanced Aggregation
- Custom percentiles (P50, P95, P99)
- Weighted averages
- Time-series forecasting

### 5. Cross-Plugin Communication
- Shared data structures
- Plugin-to-plugin API

---

## Files Summary

### Core Framework Files
- `cpp_common/measurement_plugin_api.h` - Plugin interface specification (424 lines)
- `cpp_common/plugin_loader.cpp` - Dynamic loading system (369 lines)
- `cpp_common/collector_plugin_integration.cpp` - Integration layer (245 lines)

### Example Files
- `example_plugins/disk_stats_plugin.c` - Complete example plugin (238 lines)
- `build_example_plugin.ps1` - Build script for plugins (141 lines)

### Documentation
- `PLUGIN_DEVELOPMENT.md` - Comprehensive developer guide (721 lines)
- `PLUGIN_ARCHITECTURE.md` - This file (architecture overview)

---

## Quick Start for Plugin Developers

1. **Copy example:** `cp example_plugins/disk_stats_plugin.c my_plugin.c`
2. **Edit:** Modify `plugin_get_info()`, `plugin_init()`, `plugin_collect()`
3. **Build:** `gcc -shared -o my_plugin.dll my_plugin.c -Wall -O2`
4. **Deploy:** Copy DLL to `C:\Zabbix\plugins\measurements\`
5. **Configure:** Add `[plugin.my_plugin]` section to `aggplugin.conf`
6. **Test:** `zabbix_get -k "aggplugin.my.metric[avg,60]"`

See [PLUGIN_DEVELOPMENT.md](PLUGIN_DEVELOPMENT.md) for detailed guide.

---

**End of Architecture Document**
