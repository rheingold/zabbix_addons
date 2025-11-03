# Aggplugin Usage Guide

## Quick Start

### 1. Build the Plugin

```powershell
# Navigate to the aggplugin source directory
cd path\to\aggplugin
.\build_cpp.ps1 -Variant agent2
```

This creates:
- `build\aggplugin-agent2.exe` - Go wrapper executable
- `build\libaggcollector.dll` - C++ aggregation engine
- `build\aggplugin_agent2.dll` - Agent2 plugin interface

### 2. Deploy Files

```powershell
Copy-Item build\aggplugin-agent2.exe C:\Zabbix\plugins\ -Force
Copy-Item build\libaggcollector.dll C:\Zabbix\plugins\ -Force
```

### 3. Configure Agent2

Create or edit `C:\zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf`:

```ini
# Required: External plugin executable path
Plugins.Aggplugin.System.Path=C:\Zabbix\plugins\aggplugin-agent2.exe

# Optional: Debug level (0=Fatal, 1=Error, 2=Warning, 3=Info, 4=Verbose, 5=VeryVerbose)
Plugins.Aggplugin.DebugLevel=3

# Optional: Maximum samples before auto-reset (default: 1000)
#Plugins.Aggplugin.MaxSamples=1000

# Optional: Metrics to preload on startup (default: cpu_load,mem_free)
#Plugins.Aggplugin.PreloadMetrics=cpu_load,mem_free

# Optional: Seconds to wait for baseline samples (default: 0)
#Plugins.Aggplugin.PreloadDelay=0
```

### 4. Restart Agent

```powershell
Restart-Service "Zabbix Agent 2"
```

### 5. Test Metrics

```powershell
# Test connectivity
& "C:\zabbix\bin\zabbix_get.exe" -s 127.0.0.1 -p 10050 -k "aggplugin.test"

# Get CPU statistics
& "C:\zabbix\bin\zabbix_get.exe" -s 127.0.0.1 -p 10050 -k "aggplugin.cpu_load"

# Get memory statistics
& "C:\zabbix\bin\zabbix_get.exe" -s 127.0.0.1 -p 10050 -k "aggplugin.memory_usage"
```

## Understanding the Output

### JSON Format

```json
{
  "metric": "cpu_load",
  "values": {
    "all": {
      "avg": 3.06,    // Average of all samples
      "min": 1.95,    // Minimum value observed
      "max": 4.88,    // Maximum value observed
      "med": 2.91,    // Median (50th percentile)
      "mod": 3,       // Mode (most frequent rounded value)
      "dev": 0.75,    // Standard deviation
      "var": 0.56,    // Variance (dev²)
      "cnt": 12       // Number of samples
    }
  }
}
```

### Statistics Explained

- **avg** (average): Arithmetic mean of all samples
- **min** (minimum): Lowest value recorded in the period
- **max** (maximum): Highest value recorded in the period
- **med** (median): Middle value when all samples are sorted
- **mod** (mode): Most frequently occurring rounded integer value
- **dev** (standard deviation): Measure of data spread (how much values vary from average)
- **var** (variance): Square of standard deviation
- **cnt** (count): Total number of samples collected

### First Query Behavior

**Important**: The first query after service start may return null values with `cnt:0`. This is expected because:
1. CPU monitoring requires a baseline sample (first call establishes reference)
2. Wait 2-3 seconds for the collector to gather initial samples
3. Subsequent queries will return valid statistics

Example:
```powershell
# First query (immediately after restart)
{"metric":"cpu_load","values":{"all":{"avg":null,"min":null,"max":null,"med":null,"mod":null,"dev":null,"var":null,"cnt":0}}}

# Second query (after 5 seconds)
{"metric":"cpu_load","values":{"all":{"avg":3.06,"min":1.95,"max":4.88,"med":2.91,"mod":3,"dev":0.75,"var":0.56,"cnt":12}}}
```

## Configuration Parameters

### DebugLevel
Controls log verbosity in `C:\Zabbix\log\aggplugin_debug.log`:
- `0` = Fatal errors only
- `1` = Errors
- `2` = Warnings
- `3` = Info (recommended for troubleshooting)
- `4` = Verbose
- `5` = Very verbose (includes named pipe details)

### MaxSamples
Maximum number of samples to collect before auto-resetting the accumulator:
- Default: `1000`
- At 1-second sampling interval: ~16.7 minutes
- Prevents unbounded memory growth
- Auto-reset is transparent (no data loss between queries)

### PreloadMetrics
Comma-separated list of metrics to initialize on startup:
- Default: `cpu_load,mem_free`
- Ensures metrics are ready when first queried
- Recommended to keep default value

### PreloadDelay
Seconds to wait after startup before accepting queries:
- Default: `0` (immediate)
- Set to `3-5` if you want guaranteed samples on first query
- Useful for automated monitoring systems

## Troubleshooting

### Check Service Status
```powershell
Get-Service "Zabbix Agent 2"
```

### View Debug Logs
```powershell
Get-Content C:\Zabbix\log\aggplugin_debug.log -Tail 50
```

### Common Issues

1. **"Unsupported item key"**
   - Check `System.Path` parameter in config
   - Verify executable exists at specified path
   - Restart Agent2 after config changes

2. **Returns null values**
   - Wait 2-3 seconds after service start
   - Check debug log for sampling errors
   - Verify Windows permissions (must read performance counters)

3. **Service won't start**
   - Check Agent2 log: `C:\Zabbix\log\zabbix_agent2.log`
   - Verify DLL dependencies (libaggcollector.dll in same folder)
   - Ensure no other instance is running

## Performance Considerations

- **Sampling Interval**: Fixed at 1 second (configurable in code)
- **Memory Usage**: ~8 bytes per sample × MaxSamples per metric
- **CPU Impact**: Minimal (<0.1% for 2 metrics at 1-second interval)
- **Auto-Reset**: Automatically clears old samples when MaxSamples reached

## Integration with Zabbix Server

### Create Items

In Zabbix UI, create items with keys:
- `aggplugin.cpu_load`
- `aggplugin.memory_usage`

### Extract Values

Use JSONPath preprocessing to extract specific statistics:
- Average: `$.values.all.avg`
- Minimum: `$.values.all.min`
- Maximum: `$.values.all.max`
- Median: `$.values.all.med`
- Mode: `$.values.all.mod`
- Std Dev: `$.values.all.dev`
- Variance: `$.values.all.var`
- Count: `$.values.all.cnt`

### Example Item Configuration

```
Name: CPU Load (Average)
Type: Zabbix agent (active)
Key: aggplugin.cpu_load
Type of information: Numeric (float)
Preprocessing:
  1. JSONPath: $.values.all.avg
  2. Custom multiplier: 1
```

## Development

### Rebuild After Changes

```powershell
Stop-Service "Zabbix Agent 2"
.\build_cpp.ps1 -Variant agent2
Copy-Item build\*.{exe,dll} C:\Zabbix\plugins\ -Force
Start-Service "Zabbix Agent 2"
Start-Sleep 3
& "C:\zabbix\bin\zabbix_get.exe" -s 127.0.0.1 -p 10050 -k "aggplugin.test"
```

### Debug Mode

Set `DebugLevel=5` for maximum verbosity, then monitor:
```powershell
Get-Content C:\Zabbix\log\aggplugin_debug.log -Wait
```

### Code Structure

```
aggplugin/
├── cpp_common/
│   ├── collector.cpp/.hpp      - Aggregation engine
│   └── plugin_common.cpp/.hpp  - Windows API samplers
├── go_agent2_wrapper/
│   └── main.go                 - Agent2 Go wrapper with CGO
└── build_cpp.ps1               - Build automation script
```
