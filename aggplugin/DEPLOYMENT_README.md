# AggPlugin Deployment Package

## Version 0.1 - November 30, 2025

### Features
- **Configurable Sampling Intervals**: Adjust sampling frequency for different metric types
  - `IntervalCPU`: CPU and Memory metrics (default: 1.0 seconds)
  - `IntervalDisk`: Disk I/O metrics (default: 5.0 seconds)
  - `IntervalService`: Service and Process metrics (default: 20.0 seconds)
- **Improved Plugin Loading**: All measurement plugins load correctly with proper symbol exports
- **Standardized JSON Output**: Source name changed from "_all" to "all"
- **Robust Metric Queries**: Non-parametrized metrics work without explicit filter parameters

### Contents
```
bin/
  aggplugin-agent2.exe          - Zabbix Agent 2 Go wrapper
  libaggcollector.dll           - C++ collector library
  libwinpthread-1.dll           - Threading dependency
  plugins/
    cpu_load_plugin.dll         - CPU load measurement plugin
    disk_stats_plugin.dll       - Disk I/O measurement plugin
    memory_usage_plugin.dll     - Memory usage measurement plugin
    proc_status_plugin.dll      - Process/Service status plugin
conf/
  zabbix_agent2.d/plugins.d/
    aggplugin.conf              - Configuration template
```

### Installation Instructions

#### 1. Stop Zabbix Agent 2
```powershell
Stop-Service "Zabbix Agent 2"
```

#### 2. Backup Current Installation (Optional but Recommended)
```powershell
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
New-Item -ItemType Directory -Path "C:\Zabbix\backup_$timestamp" -Force
Copy-Item C:\Zabbix\bin\aggplugin-agent2.exe "C:\Zabbix\backup_$timestamp\" -ErrorAction SilentlyContinue
Copy-Item C:\Zabbix\bin\libaggcollector.dll "C:\Zabbix\backup_$timestamp\" -ErrorAction SilentlyContinue
Copy-Item C:\Zabbix\bin\plugins\*.dll "C:\Zabbix\backup_$timestamp\" -ErrorAction SilentlyContinue
```

#### 3. Extract and Deploy
Extract the zip to a temporary location, then copy files:

```powershell
# Copy binaries
Copy-Item bin\aggplugin-agent2.exe C:\Zabbix\bin\
Copy-Item bin\libaggcollector.dll C:\Zabbix\bin\
Copy-Item bin\libwinpthread-1.dll C:\Zabbix\bin\

# Copy plugins
Copy-Item bin\plugins\*.dll C:\Zabbix\bin\plugins\

# Copy config (if not already configured)
Copy-Item conf\zabbix_agent2.d\plugins.d\aggplugin.conf C:\zabbix\conf\zabbix_agent2.d\plugins.d\
```

#### 4. Configure Sampling Intervals (Optional)
Edit `C:\zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf`:

```ini
# Sampling Intervals (in seconds)
Plugins.Aggplugin.IntervalCPU=1.0      # CPU/Memory metrics
Plugins.Aggplugin.IntervalDisk=5.0     # Disk I/O metrics
Plugins.Aggplugin.IntervalService=20.0 # Service/Process metrics
```

#### 5. Start Zabbix Agent 2
```powershell
Start-Service "Zabbix Agent 2"
```

#### 6. Verify Installation
```powershell
# Check service status
Get-Service "Zabbix Agent 2"

# Test metric query
C:\Zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin.cpu_load"
```

Expected output:
```json
[{"source":"all","avg":5.2,"min":2.1,"max":8.3,"med":5.0,"mod":5,"dev":1.8,"var":3.2,"cnt":10,"last":5.5}]
```

### Configuration Parameters

#### Sampling Intervals
- **Plugins.Aggplugin.IntervalCPU**: Sampling interval for CPU and memory metrics (seconds)
- **Plugins.Aggplugin.IntervalDisk**: Sampling interval for disk I/O metrics (seconds)
- **Plugins.Aggplugin.IntervalService**: Sampling interval for service/process metrics (seconds)

#### Other Parameters
- **Plugins.Aggplugin.PreloadMetrics**: Comma-separated list of metrics to preload
- **Plugins.Aggplugin.PreloadDelay**: Delay before accepting queries (seconds)
- **Plugins.Aggplugin.MaxSamples**: Maximum samples before auto-reset (per metric)
- **Plugins.Aggplugin.OutputFormat**: JSON format (0=array, 1=nested)

### Available Metrics

#### CPU Metrics (1s interval)
- `aggplugin.cpu_load` - CPU load percentage

#### Memory Metrics (1s interval)
- `aggplugin.mem_free` - Free memory in MB

#### Disk I/O Metrics (5s interval)
- `aggplugin.disk.io.read` - Disk read operations/sec
- `aggplugin.disk.io.write` - Disk write operations/sec
- `aggplugin.storage.queue.length` - Storage queue length
- `aggplugin.vfs.queue.length` - VFS queue length

#### Process/Service Metrics (20s interval)
- `aggplugin.proc.running[<filter>]` - Process count by filter
- `aggplugin.service.status[<filter>]` - Service status by filter

### Troubleshooting

#### Plugins Not Loading
Check logs in `C:\zabbix\log\aggplugin_debug.log` for:
```
Successfully loaded plugin: CPULoad
Successfully loaded plugin: DiskStats
Successfully loaded plugin: MemoryUsage
Successfully loaded plugin: ProcStatus
```

If plugins fail to load, ensure all DLLs are present and not blocked by Windows.

#### Empty Metric Returns
If metrics return `[]`, check that:
1. Service is running: `Get-Service "Zabbix Agent 2"`
2. Collector is sampling: Check `C:\zabbix\log\collector_debug.log`
3. Sufficient time has passed for first samples (wait 1-2 sampling intervals)

#### Performance Tuning
Adjust intervals based on your monitoring needs:
- **High-frequency monitoring**: Reduce intervals (e.g., 0.5s for CPU)
- **Low-frequency monitoring**: Increase intervals (e.g., 30s for services)
- **Resource constraints**: Increase all intervals to reduce CPU usage

### Changes from Previous Version
1. ✅ Added configurable sampling intervals (IntervalCPU, IntervalDisk, IntervalService)
2. ✅ Fixed plugin loading issue (changed from g++ to gcc build)
3. ✅ Changed JSON source name from "_all" to "all"
4. ✅ Fixed empty metric returns for non-parametrized metrics
5. ✅ Updated build script to use gcc with proper export flags

### Support
For issues or questions:
- Check logs: `C:\zabbix\log\aggplugin_debug.log` and `C:\zabbix\log\collector_debug.log`
- GitHub: https://github.com/rheingold/zabbix_addons
- Branch: tmp0.1
