# Zabbix Aggplugin - Measurement Plugins

Standalone project for building custom measurement plugin DLLs for the Zabbix Aggplugin framework.

## Overview

This project contains example measurement plugins that demonstrate how to extend Zabbix Aggplugin with custom system metrics. Each plugin is a standalone DLL that implements the Measurement Plugin API.

**CRITICAL:** Plugins return **current instantaneous values only**. The aggplugin collector handles all statistics computation (avg, min, max, median, mode, stddev, variance, count).

## Included Plugins

### 1. CPU Load Plugin (`cpu_load_plugin.dll`)
- **Metric:** `aggplugin.cpu_load`
- **Returns:** Current CPU utilization percentage (0-100)
- **Implementation:** Windows GetSystemTimes() API with delta calculation
- **Type:** Single-source metric

### 2. Memory Usage Plugin (`memory_usage_plugin.dll`)
- **Metric:** `aggplugin.mem_free`
- **Returns:** Current available physical memory in megabytes
- **Implementation:** Windows GlobalMemoryStatusEx() API
- **Type:** Single-source metric

### 3. Disk Stats Plugin (`disk_stats_plugin.dll`)
- **Metrics:** 
  - `aggplugin.disk.io.read` - Current disk read operations/sec
  - `aggplugin.disk.io.write` - Current disk write operations/sec
- **Returns:** Current I/O rate for each physical disk
- **Implementation:** Windows PDH (Performance Data Helper) API
- **Type:** Multi-source metric (separate value per disk)

## Building Plugins

### Windows (MinGW64/MSYS2)

```powershell
# Build all plugins
.\build.ps1

# Build specific plugin
gcc -shared -o cpu_load_plugin.dll cpu_load_plugin.c -Wall -O2
gcc -shared -o memory_usage_plugin.dll memory_usage_plugin.c -Wall -O2
gcc -shared -o disk_stats_plugin.dll disk_stats_plugin.c -lPdh -Wall -O2
```

### Linux (GCC)

```bash
# Build all plugins
./build.sh

# Build specific plugin
gcc -shared -fPIC -o cpu_load_plugin.so cpu_load_plugin.c -Wall -O2
gcc -shared -fPIC -o memory_usage_plugin.so memory_usage_plugin.c -Wall -O2
```

## Deployment

1. Copy built DLL files to your Zabbix plugins directory:
   - Windows: `C:\Zabbix\plugins\measurements\`
   - Linux: `/usr/local/zabbix/plugins/measurements/`

2. Configure aggplugin to load plugins in `aggplugin.conf`:
   ```ini
   Plugins.Aggplugin.PluginPath=C:\Zabbix\plugins\measurements\*.dll
   
   [cpu_load_plugin.dll]
   MetricKeys=cpu_load
   
   [memory_usage_plugin.dll]
   MetricKeys=mem_free
   
   [disk_stats_plugin.dll]
   MetricKeys=disk.io.read,disk.io.write
   ```

3. Restart Zabbix Agent2

## Measurement Plugin API

All plugins must implement the following functions (defined in `measurement_plugin_api.h`):

### Required Functions

#### `plugin_get_info()`
Returns plugin metadata including name, version, and metric keys.

```c
const plugin_info_t* plugin_get_info() {
    static const metric_key_info_t keys[] = {
        {"metric_name", "Description", METRIC_TYPE_FLOAT, NULL, 0}
    };
    static const plugin_info_t info = {
        "PluginName", "1.0", "Author", 1, keys
    };
    return &info;
}
```

#### `plugin_init(const char* config_section)`
Initialize plugin resources. Return 0 on success, -1 on error.

#### `plugin_collect(collection_result_t* results)`
Collect current measurements. Return number of metrics collected.

**IMPORTANT:** Return CURRENT values only, not statistics.

```c
size_t plugin_collect(collection_result_t* results) {
    // Single-source example
    results[0].values = malloc(sizeof(measurement_value_t));
    results[0].values[0].source_id = NULL;  // NULL for single-source
    results[0].values[0].value = get_current_cpu_load();  // CURRENT value
    results[0].value_count = 1;
    results[0].status = COLLECT_OK;
    
    return 1;  // Number of metrics
}
```

#### `plugin_deinit()`
Clean up plugin resources. Return 0 on success.

### Optional Functions

- `plugin_on_reset(const char* key)` - Called when metric is reset
- `plugin_aggregate_custom(...)` - Custom aggregation logic
- `plugin_on_init_complete()` - Post-initialization hook

## Multi-Source Metrics

For metrics with multiple sources (e.g., per-disk, per-CPU):

1. Set `has_multiple_sources = 1` in metric_key_info_t
2. In `plugin_collect()`, return CURRENT value for each source
3. Allocate measurement_value_t array with one entry per source
4. Set unique `source_id` for each value (e.g., "0", "1", "disk0", "core0")

**Example (disk plugin with 2 disks):**

```c
size_t plugin_collect(collection_result_t* results) {
    int disk_count = 2;
    
    // Allocate array for all sources
    results[0].values = malloc(sizeof(measurement_value_t) * disk_count);
    results[0].value_count = disk_count;
    
    // Return CURRENT value for each disk
    results[0].values[0].source_id = strdup("0");
    results[0].values[0].value = get_current_disk_io(0);  // Disk 0 NOW
    
    results[0].values[1].source_id = strdup("1");
    results[0].values[1].value = get_current_disk_io(1);  // Disk 1 NOW
    
    results[0].status = COLLECT_OK;
    return 1;  // Number of metrics (not sources!)
}
```

### How Aggplugin Handles Multi-Source Metrics

1. **Plugin responsibility:** Return current values
2. **Collector responsibility:** Everything else

**Data Flow:**

```
Plugin (every 1 second):
   Returns: {source_id: "0", value: 42.5}, {source_id: "1", value: 28.3}

Collector:
   Accumulates values in separate accumulators per source
   source "0" accumulator: [42.5, 43.1, 41.8, ...]
   source "1" accumulator: [28.3, 29.1, 27.5, ...]

Zabbix Query (after 120 samples):
   Returns JSON with computed statistics:
```

```json
{
  "metric": "disk.io.read",
  "values": {
    "all": {
      "avg": 250.5,
      "min": 100,
      "max": 400,
      "med": 240,
      "mod": 250,
      "dev": 85.2,
      "var": 7259,
      "cnt": 120
    },
    "sources": {
      "0": {
        "avg": 150.2,
        "min": 50,
        "max": 300,
        "med": 145,
        "mod": 150,
        "dev": 62.3,
        "var": 3881,
        "cnt": 120
      },
      "1": {
        "avg": 100.3,
        "min": 50,
        "max": 100,
        "med": 98,
        "mod": 100,
        "dev": 15.2,
        "var": 231,
        "cnt": 120
      }
    }
  }
}
```

**Key Points:**
- Plugin returns **current values** (simple floats)
- Collector computes **statistics** (8 fields: avg, min, max, med, mod, dev, var, cnt)
- Collector provides **aggregate** across all sources ("all")
- Collector provides **per-source** statistics ("sources")

## Creating Custom Plugins

1. Copy one of the example plugins as a template
2. Modify `plugin_get_info()` with your metric information
3. Implement `plugin_init()` to initialize resources
4. Implement `plugin_collect()` to gather **current** measurements
5. Implement `plugin_deinit()` to clean up
6. Compile as shared library (.dll on Windows, .so on Linux)
7. Deploy and configure as shown above

## Memory Management

**Plugin allocates:**
- measurement_value_t arrays (via malloc)
- source_id strings (via strdup/malloc)
- str_value strings if using string metrics

**Aggplugin frees:**
- All measurement_value_t arrays after collection
- All source_id strings
- All str_value strings

**No memory leaks:** Clean separation of responsibilities.

## API Reference

See `measurement_plugin_api.h` for complete API documentation including:
- Data structures (plugin_info_t, collection_result_t, measurement_value_t)
- Metric types (FLOAT, UINT64, STRING)
- Collection status codes (COLLECT_OK, COLLECT_ERROR, COLLECT_NO_DATA)
- Memory management guidelines

## Platform Support

- **Windows:** Fully supported with MinGW64/MSYS2
- **Linux:** Planned (API is platform-agnostic, implementations need porting)

## Testing

Query metrics to verify statistics computation:

```bash
# Query metric (shows computed statistics)
zabbix_get -s localhost -p 10050 -k aggplugin.cpu_load
zabbix_get -s localhost -p 10050 -k aggplugin.disk.io.read

# Expected output format:
# {"values":{"all":{"avg":42.5,"min":30,"max":55,...}}}
```

## License

Part of the Zabbix Aggplugin project.

## Author

Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
