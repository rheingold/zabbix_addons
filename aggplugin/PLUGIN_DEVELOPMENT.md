# Plugin Development Guide

## Overview

The aggplugin framework now supports **loadable measurement plugins** - lightweight DLLs that extend the system with custom metrics without modifying core code. This guide explains how to create, build, and deploy measurement plugins.

---

## What is a Measurement Plugin?

A measurement plugin is a Windows DLL that:
- Implements a simple C API (`measurement_plugin_api.h`)
- Collects specific metrics (disk I/O, network stats, hardware sensors, etc.)
- Returns measurements to the aggplugin framework for aggregation
- Can expose multiple metric keys with multiple sources per key

**Benefits:**
- **Modular**: Add new metrics by dropping DLLs into a directory
- **Simple**: Minimal boilerplate, ~100 lines for typical plugin
- **Safe**: Plugins run in separate DLLs with isolated state
- **Dynamic**: Supports hot-pluggable devices (USB drives, CPUs, etc.)

---

## Quick Start

### 1. Create Your Plugin

Create `my_plugin.c`:

```c
#include "../cpp_common/measurement_plugin_api.h"
#include <stdio.h>

// Define metric keys
static const metric_key_info_t plugin_keys[] = {
    {"my.metric", "My custom metric", METRIC_TYPE_FLOAT, "", 0}
};

static const plugin_info_t plugin_metadata = {
    "MyPlugin", "1.0", "Your Name", 1, plugin_keys
};

// Required: Return plugin metadata
PLUGIN_EXPORT const plugin_info_t* plugin_get_info() {
    return &plugin_metadata;
}

// Required: Initialize plugin
PLUGIN_EXPORT int plugin_init(const char* config_section) {
    printf("MyPlugin: Initialized\n");
    return 0;  // Success
}

// Required: Collect measurements
PLUGIN_EXPORT size_t plugin_collect(collection_result_t* results) {
    // Allocate measurement value
    measurement_value_t* values = malloc(sizeof(measurement_value_t));
    values[0].source_id = NULL;  // Single-source metric
    values[0].value = 42.0;      // Your measurement
    values[0].str_value = NULL;
    
    // Fill result
    results[0].key = "my.metric";
    results[0].status = COLLECT_OK;
    results[0].value_count = 1;
    results[0].values = values;
    
    return 1;  // Number of results
}

// Required: Clean up plugin
PLUGIN_EXPORT int plugin_deinit() {
    printf("MyPlugin: Deinitialized\n");
    return 0;
}
```

### 2. Compile Your Plugin

Using MinGW64:

```powershell
gcc -shared -o my_plugin.dll my_plugin.c -Wall -O2
```

With additional Windows APIs (PDH, WMI, etc.):

```powershell
gcc -shared -o my_plugin.dll my_plugin.c -lPdh -lWtsapi32 -Wall -O2
```

### 3. Deploy Your Plugin

1. Copy `my_plugin.dll` to: `C:\Zabbix\plugins\measurements\`
2. Edit `aggplugin.conf`:

```ini
[Plugins]
PluginPath=C:\Zabbix\plugins\measurements\*.dll

[plugin.my_plugin]
enabled=1
# Plugin-specific configuration (optional)
interval=5.0
```

3. Restart Zabbix Agent2

### 4. Test Your Plugin

```powershell
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.my.metric[avg,60]"
```

---

## Plugin API Reference

### Required Functions

Every plugin **must** export these four functions:

#### `plugin_get_info()`
```c
PLUGIN_EXPORT const plugin_info_t* plugin_get_info();
```
- **When called**: Once during plugin loading
- **Returns**: Pointer to static `plugin_info_t` structure
- **Purpose**: Register metric keys and plugin metadata

#### `plugin_init(config_section)`
```c
PLUGIN_EXPORT int plugin_init(const char* config_section);
```
- **When called**: Once after plugin is loaded
- **Parameters**: Configuration string from `[plugin.name]` section
- **Returns**: 0 on success, non-zero on error
- **Purpose**: Initialize resources (open handles, allocate memory, etc.)

#### `plugin_collect(results)`
```c
PLUGIN_EXPORT size_t plugin_collect(collection_result_t* results);
```
- **When called**: Periodically (every sampling interval) from background thread
- **Parameters**: Pre-allocated array for results (one slot per registered key)
- **Returns**: Number of results filled
- **Purpose**: Collect current measurements

**Memory management**:
- Caller allocates `results` array
- Plugin allocates `values` array (caller frees it)
- Plugin allocates string fields (caller frees them)

#### `plugin_deinit()`
```c
PLUGIN_EXPORT int plugin_deinit();
```
- **When called**: Once during shutdown
- **Returns**: 0 on success, non-zero on error (logged but not fatal)
- **Purpose**: Clean up resources

---

### Optional Functions

These functions are optional but provide advanced features:

#### `plugin_on_reset(key)`
```c
PLUGIN_EXPORT void plugin_on_reset(const char* key);
```
- **When called**: After aggregated data is fetched and reset
- **Purpose**: Notify plugin that data was consumed (useful for counter resets)

#### `plugin_aggregate_custom(...)`
```c
PLUGIN_EXPORT int plugin_aggregate_custom(
    const char* key,
    const char* source_id,
    const double* values,
    size_t count,
    void* result
);
```
- **When called**: During aggregation (computing statistics)
- **Returns**: 0 to use plugin aggregation, non-zero for default
- **Purpose**: Override default aggregation logic (percentiles, weighted avg, etc.)
- **Warning**: Advanced feature, most plugins should use default aggregation

#### `plugin_on_init_complete()`
```c
PLUGIN_EXPORT void plugin_on_init_complete();
```
- **When called**: After all plugins are initialized
- **Purpose**: Cross-plugin coordination

---

## Data Types

### `metric_value_type_t`
Defines the type of metric value:
- `METRIC_TYPE_FLOAT` - Floating point number (most common)
- `METRIC_TYPE_UINT64` - Unsigned 64-bit integer
- `METRIC_TYPE_STRING` - Short text string
- `METRIC_TYPE_TEXT` - Long text (not aggregated)
- `METRIC_TYPE_LOG` - Log entry (not aggregated)

### `collect_status_t`
Collection result status:
- `COLLECT_OK` - Success
- `COLLECT_NOTSUPPORTED` - Metric not supported on this system
- `COLLECT_ERROR` - Collection failed (temporary error)
- `COLLECT_TIMEOUT` - Collection timed out

### `measurement_value_t`
Single measurement with optional source identifier:
```c
typedef struct {
    const char* source_id;    // Source ID (NULL for single-source)
    double value;             // Numeric value
    const char* str_value;    // String value (optional)
} measurement_value_t;
```

**Single-source example** (total system memory):
```c
measurement_value_t val;
val.source_id = NULL;  // No source distinction
val.value = 16384.0;   // Total MB
val.str_value = NULL;
```

**Multi-source example** (per-disk I/O):
```c
measurement_value_t vals[3];
vals[0] = {"sda", 1234.5, NULL};  // Disk 0
vals[1] = {"sdb", 567.8, NULL};   // Disk 1
vals[2] = {"sdc", 890.1, NULL};   // Disk 2
```

### `collection_result_t`
Result for a single metric key:
```c
typedef struct {
    const char* key;          // Metric key (must match registered key)
    collect_status_t status;  // Collection status
    size_t value_count;       // Number of values (0 if error)
    measurement_value_t* values; // Array of values
} collection_result_t;
```

---

## Multi-Source Metrics

Plugins can return multiple values per metric (one per source). This is useful for:
- Per-CPU metrics (one value per core)
- Per-disk metrics (one value per physical disk)
- Per-network interface metrics

**Example**: Disk I/O plugin exposing `disk.io.read[device]`

```c
// In plugin_get_info():
static const metric_key_info_t keys[] = {
    {"disk.io.read", "Disk reads/sec", METRIC_TYPE_UINT64, "device", 1}
    //                                                               ^^^ has_multiple_sources
};

// In plugin_collect():
measurement_value_t* vals = malloc(sizeof(measurement_value_t) * 3);
vals[0] = {strdup("sda"), 1234.0, NULL};
vals[1] = {strdup("sdb"), 567.0, NULL};
vals[2] = {strdup("sdc"), 890.0, NULL};

results[0] = {"disk.io.read", COLLECT_OK, 3, vals};
```

**Zabbix item configuration**:
```
aggplugin.disk.io.read[sda,avg,60]  # Average reads/sec for sda over 60s
aggplugin.disk.io.read[sdb,max,60]  # Max reads/sec for sdb over 60s
```

---

## Configuration

Plugins can receive custom configuration from `aggplugin.conf`:

**aggplugin.conf**:
```ini
[plugin.disk_stats]
enabled=1
devices=0,1,2
sample_rate=2.0
exclude_removable=true
```

**In plugin code**:
```c
PLUGIN_EXPORT int plugin_init(const char* config_section) {
    // config_section contains:
    // "enabled=1\ndevices=0,1,2\nsample_rate=2.0\nexclude_removable=true"
    
    // Parse configuration (use your preferred method)
    if (strstr(config_section, "sample_rate=")) {
        sscanf(strstr(config_section, "sample_rate="), "sample_rate=%lf", &rate);
    }
    
    return 0;
}
```

---

## Example: Disk Stats Plugin

See `example_plugins/disk_stats_plugin.c` for a complete, production-ready example demonstrating:
- Multi-source metrics (per-disk statistics)
- Windows PDH API usage
- Dynamic disk detection
- Proper memory management
- Configuration parsing

## Example: Built-in Metrics as Plugins

The CPU and memory metrics that were previously built into aggplugin have been converted to example plugins:

### CPU Load Plugin (`cpu_load_plugin.c`)
- **Metric:** `cpu_load` - System-wide CPU utilization (0-100%)
- **API:** Windows GetSystemTimes()
- **Features:** Delta calculation, baseline establishment
- **Size:** ~221 KB compiled

### Memory Usage Plugin (`memory_usage_plugin.c`)
- **Metric:** `mem_free` - Available physical memory (MB)
- **API:** Windows GlobalMemoryStatusEx()
- **Features:** Simple stateless sampling
- **Size:** ~221 KB compiled

### Disk Stats Plugin (`disk_stats_plugin.c`)
- **Metrics:** `disk.io.read[device]`, `disk.io.write[device]`
- **API:** Windows PDH (Performance Data Helper)
- **Features:** Multi-source (per-disk), dynamic detection
- **Size:** ~227 KB compiled

**Build All:**
```powershell
cd example_plugins
..\build_example_plugin.ps1  # Builds all 3 plugins
```

**Build**:
```powershell
cd example_plugins

# Build all plugins at once
..\build_example_plugin.ps1

# Or build individually
..\build_example_plugin.ps1 -PluginName cpu_load_plugin
..\build_example_plugin.ps1 -PluginName memory_usage_plugin
..\build_example_plugin.ps1 -PluginName disk_stats_plugin
```

**Deploy**:
```powershell
Copy-Item disk_stats_plugin.dll C:\Zabbix\plugins\measurements\
```

**Configure** (`aggplugin.conf`):
```ini
[plugin.disk_stats]
enabled=1
```

**Test**:
```powershell
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.disk.io.read[0,avg,60]"
```

---

## Best Practices

### 1. Keep Collection Fast
`plugin_collect()` is called from a background thread every sampling interval (typically 1 second). **Keep it under 100ms**:
- ✅ Read `/proc` files, Windows performance counters
- ✅ Query WMI with short timeout
- ❌ Don't block on network I/O
- ❌ Don't do expensive calculations (pre-compute in `plugin_init()`)

### 2. Handle Errors Gracefully
Return `COLLECT_ERROR` for temporary failures, `COLLECT_NOTSUPPORTED` for permanent ones:
```c
if (GetDiskStats(&stats) != 0) {
    results[0].status = COLLECT_ERROR;  // Try again next time
    results[0].value_count = 0;
    results[0].values = NULL;
    return 1;
}
```

### 3. Free Memory Correctly
Plugin allocates, caller frees:
```c
// Plugin allocates:
measurement_value_t* vals = malloc(sizeof(measurement_value_t) * count);
vals[0].source_id = strdup("source1");  // Caller will free this

// Caller frees:
for (size_t i = 0; i < count; i++) {
    free((void*)vals[i].source_id);
}
free(vals);
```

### 4. Support Dynamic Sources
If your metric has sources that can appear/disappear (USB drives, hot-plug CPUs):
- Return current source count in each `plugin_collect()` call
- Don't cache source list in `plugin_init()`

```c
// BAD: Cached at init
static int disk_count = detect_disks();  // What if USB drive is added?

// GOOD: Detect on each collection
size_t plugin_collect(collection_result_t* results) {
    int disk_count = detect_disks_now();  // Fresh count
    // ...
}
```

### 5. Log Errors to stderr
Aggplugin captures stderr output in debug mode:
```c
if (pdh_status != ERROR_SUCCESS) {
    fprintf(stderr, "DiskStatsPlugin: PDH error 0x%lx\n", pdh_status);
}
```

---

## Thread Safety

- **`plugin_collect()`**: Called from background sampling thread
- **`plugin_on_reset()`**: Called from query/response thread
- If both access shared state, use mutexes

Example:
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

## Troubleshooting

### Plugin Not Loaded
**Symptom**: No log message "Loaded plugin 'PluginName'"

**Causes**:
1. DLL not in plugin directory → Check `PluginPath` in config
2. Missing dependencies → Run `dumpbin /dependents my_plugin.dll`
3. Wrong architecture → Ensure DLL is 64-bit (matches Agent2)
4. Missing exports → Run `dumpbin /exports my_plugin.dll`

**Debug**:
```powershell
# Check DLL architecture
dumpbin /headers my_plugin.dll | Select-String "machine"

# Check exports
dumpbin /exports my_plugin.dll | Select-String "plugin_"
```

### Plugin Init Failed
**Symptom**: "plugin_init() failed for 'PluginName'"

**Causes**:
1. Configuration parsing error
2. Required system resource unavailable
3. Permissions issue

**Fix**: Add debug logging in `plugin_init()`:
```c
int plugin_init(const char* config_section) {
    fprintf(stderr, "MyPlugin: init called with config: %s\n", config_section);
    
    if (InitResource() != 0) {
        fprintf(stderr, "MyPlugin: Failed to init resource (error %d)\n", GetLastError());
        return -1;
    }
    
    fprintf(stderr, "MyPlugin: Successfully initialized\n");
    return 0;
}
```

### No Metrics Returned
**Symptom**: `zabbix_get` returns empty or error

**Causes**:
1. `plugin_collect()` returns 0 or error status
2. Metric key mismatch (typo in `results[].key`)
3. Collection timeout (slow API call)

**Debug**:
```c
size_t plugin_collect(collection_result_t* results) {
    fprintf(stderr, "MyPlugin: collect called\n");
    
    // ... collect data ...
    
    fprintf(stderr, "MyPlugin: Returning %d results\n", count);
    for (size_t i = 0; i < count; i++) {
        fprintf(stderr, "  [%zu] key=%s status=%d value_count=%zu\n",
                i, results[i].key, results[i].status, results[i].value_count);
    }
    
    return count;
}
```

---

## API Version Compatibility

Current API version: **1.0** (tmp0.1)

The plugin API is designed to be stable, but may evolve. Future versions will maintain backward compatibility with v1.0 plugins by:
- Keeping required functions unchanged
- Adding new optional functions (old plugins still work)
- Using version negotiation in `plugin_info_t`

---

## Need Help?

- **Examples**: See `example_plugins/disk_stats_plugin.c`
- **API Reference**: `cpp_common/measurement_plugin_api.h` (heavily commented)
- **Issues**: Create issue in repository

---

**Happy plugin development!** 🚀
