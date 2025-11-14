# Plugin API Quick Reference

## Minimal Plugin Template

```c
#include "../cpp_common/measurement_plugin_api.h"

// 1. Define your metric keys
static const metric_key_info_t keys[] = {
    {"my.metric", "My metric description", METRIC_TYPE_FLOAT, "", 0}
};

static const plugin_info_t info = {
    "MyPlugin", "1.0", "Author Name", 1, keys
};

// 2. Required: Get plugin info
PLUGIN_EXPORT const plugin_info_t* plugin_get_info() {
    return &info;
}

// 3. Required: Initialize
PLUGIN_EXPORT int plugin_init(const char* config) {
    // Setup code here
    return 0; // 0 = success, non-zero = error
}

// 4. Required: Collect measurements
PLUGIN_EXPORT size_t plugin_collect(collection_result_t* results) {
    // Allocate values array
    measurement_value_t* vals = malloc(sizeof(measurement_value_t));
    vals[0].source_id = NULL;   // No source distinction
    vals[0].value = 42.0;        // Your measurement
    vals[0].str_value = NULL;
    
    // Fill result
    results[0].key = "my.metric";
    results[0].status = COLLECT_OK;
    results[0].value_count = 1;
    results[0].values = vals;
    
    return 1; // Number of results
}

// 5. Required: Cleanup
PLUGIN_EXPORT int plugin_deinit() {
    // Cleanup code here
    return 0;
}
```

## Build & Deploy

```powershell
# Build
gcc -shared -o my_plugin.dll my_plugin.c -Wall -O2

# Deploy
Copy-Item my_plugin.dll C:\Zabbix\plugins\measurements\

# Configure (aggplugin.conf)
[Plugins]
PluginPath=C:\Zabbix\plugins\measurements\*.dll

[plugin.my_plugin]
enabled=1

# Test
zabbix_get -s 127.0.0.1 -k "aggplugin.my.metric[avg,60]"
```

## Multi-Source Example

```c
// For per-disk, per-CPU, per-interface metrics
size_t plugin_collect(collection_result_t* results) {
    // Allocate for 3 sources
    measurement_value_t* vals = malloc(sizeof(measurement_value_t) * 3);
    
    vals[0].source_id = strdup("disk0");
    vals[0].value = 123.4;
    vals[0].str_value = NULL;
    
    vals[1].source_id = strdup("disk1");
    vals[1].value = 567.8;
    vals[1].str_value = NULL;
    
    vals[2].source_id = strdup("disk2");
    vals[2].value = 901.2;
    vals[2].str_value = NULL;
    
    results[0].key = "disk.io.read";
    results[0].status = COLLECT_OK;
    results[0].value_count = 3;
    results[0].values = vals;
    
    return 1;
}
```

## Data Types

```c
// Metric value types
METRIC_TYPE_FLOAT    // Double precision float (most common)
METRIC_TYPE_UINT64   // 64-bit unsigned integer
METRIC_TYPE_STRING   // Short text string
METRIC_TYPE_TEXT     // Long text (not aggregated)
METRIC_TYPE_LOG      // Log entry (not aggregated)

// Collection status
COLLECT_OK           // Success
COLLECT_ERROR        // Temporary failure (retry next time)
COLLECT_NOTSUPPORTED // Permanent failure (not available)
COLLECT_TIMEOUT      // Collection timed out
```

## Common Windows APIs

```c
// PDH (Performance Data Helper)
#include <pdh.h>
// Link: -lPdh
PDH_HQUERY query;
PdhOpenQuery(NULL, 0, &query);
PdhAddEnglishCounter(query, "\\Counter\\Path", 0, &counter);
PdhCollectQueryData(query);
PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &value);

// WMI (Windows Management Instrumentation)
#include <wbemidl.h>
// Link: -lole32 -loleaut32

// WTS (Terminal Services)
#include <wtsapi32.h>
// Link: -lWtsapi32
```

## Error Handling

```c
size_t plugin_collect(collection_result_t* results) {
    if (check_failed()) {
        // Temporary error
        results[0].key = "my.metric";
        results[0].status = COLLECT_ERROR;
        results[0].value_count = 0;
        results[0].values = NULL;
        
        // Log to stderr (captured by aggplugin)
        fprintf(stderr, "MyPlugin: Collection failed (error %d)\n", GetLastError());
        
        return 1;
    }
    
    // Success case...
}
```

## Thread Safety

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

int plugin_deinit() {
    DeleteCriticalSection(&lock);
    return 0;
}
```

## Configuration Parsing

```c
// Config format: key=value\nkey2=value2\n...
int plugin_init(const char* config) {
    // Simple substring search
    if (strstr(config, "enabled=1")) {
        // Plugin is enabled
    }
    
    // Extract numeric value
    double rate = 1.0;
    if (strstr(config, "sample_rate=")) {
        sscanf(strstr(config, "sample_rate="), "sample_rate=%lf", &rate);
    }
    
    // Extract string list
    if (strstr(config, "devices=")) {
        char devices[256];
        sscanf(strstr(config, "devices="), "devices=%255s", devices);
        // Parse comma-separated: "0,1,2"
    }
    
    return 0;
}
```

## Memory Rules

**Plugin allocates:**
- `values` array (malloc)
- `source_id` strings (strdup)
- `str_value` strings (strdup)

**Framework frees:**
- Everything after processing

**Example:**
```c
// Plugin side
measurement_value_t* vals = malloc(sizeof(measurement_value_t));
vals[0].source_id = strdup("disk0");  // Framework will free
vals[0].value = 123.4;
vals[0].str_value = NULL;
```

## Troubleshooting

**Plugin not loading:**
```powershell
# Check DLL exports
dumpbin /exports my_plugin.dll | Select-String "plugin_"

# Check dependencies
dumpbin /dependents my_plugin.dll

# Check architecture (must be x64)
dumpbin /headers my_plugin.dll | Select-String "machine"
```

**No metrics returned:**
- Check `results[].status` is `COLLECT_OK`
- Check `results[].value_count > 0`
- Check `results[].key` matches registered key
- Add debug logging: `fprintf(stderr, "Debug: %s\n", message);`

**Crashes:**
- Check NULL pointers before dereferencing
- Ensure `malloc()` succeeded
- Don't access freed memory
- Use `strdup()` for strings, not stack buffers

## Best Practices

✅ **DO:**
- Keep `plugin_collect()` fast (< 100ms)
- Return `COLLECT_ERROR` for temporary failures
- Free all allocated memory in `plugin_deinit()`
- Use `fprintf(stderr, ...)` for logging
- Check `malloc()` return values

❌ **DON'T:**
- Block on network I/O in `plugin_collect()`
- Throw exceptions (C plugins should use return codes)
- Cache source list if sources can change (USB drives, etc.)
- Return stack-allocated strings in `source_id`
- Assume config is always non-NULL

## Documentation

- **Full Developer Guide:** [PLUGIN_DEVELOPMENT.md](PLUGIN_DEVELOPMENT.md)
- **Architecture Overview:** [PLUGIN_ARCHITECTURE.md](PLUGIN_ARCHITECTURE.md)
- **API Header:** `cpp_common/measurement_plugin_api.h`
- **Example Plugin:** `example_plugins/disk_stats_plugin.c`

---

**Need help?** Check the example plugin or create an issue in the repository.
