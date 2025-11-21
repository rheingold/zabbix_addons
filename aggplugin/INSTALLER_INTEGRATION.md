# Aggplugin Installer Integration

## Date: 2025-11-21

## Overview
Successfully integrated aggplugin with the Zabbix Agent installer package (instpackage_my1). The installer now automatically configures aggplugin during Zabbix Agent 2 installation with correct absolute paths.

## Changes Made to Installer Package

### Configuration Template
Added aggplugin configuration to `.baseval.ini` with macro-based paths:

```ini
[%%INSTALL_PATH%%\conf\zabbix_agent2.d\plugins.d\aggplugin.conf]
Plugins.Aggplugin.PluginPath=%%INSTALL_PATH%%\bin\plugins\*.dll
Plugins.Aggplugin.DebugLevel=4
Plugins.Aggplugin.MaxSamples=1000
Plugins.Aggplugin.PreloadMetrics=cpu_load,mem_free,disk.io.read,disk.io.write
Plugins.Aggplugin.PreloadDelay=2
```

### Installer Enhancement
Enhanced `inied.bat` to expand macros in section names, allowing dynamic path resolution based on installation directory.

## Deployment Files Required

For aggplugin to work with the installer, ensure these files are included in the package:

### Binary Files
- `bin/aggplugin-agent2.exe` - External plugin executable
- `bin/aggplugin_agent2.dll` - Agent 2 wrapper
- `bin/libaggcollector.dll` - Core collector library
- `bin/plugins/*.dll` - Measurement plugin DLLs:
  - `cpu_load_plugin.dll`
  - `memory_usage_plugin.dll`
  - `disk_stats_plugin.dll`

### Configuration Files
- `conf/zabbix_agent2.d/plugins.d/aggplugin.conf` - Plugin configuration template

### Template Configuration
The template `aggplugin.conf` should contain:
```ini
### Configuration for Aggplugin

Plugins.Aggplugin.DebugLevel=4
Plugins.Aggplugin.MaxSamples=1000
Plugins.Aggplugin.PreloadMetrics=cpu_load,mem_free,disk.io.read,disk.io.write
Plugins.Aggplugin.PreloadDelay=2
# PluginPath: Will be set by installer
Plugins.Aggplugin.PluginPath=\bin\plugins\*.dll
```

The installer will automatically replace the relative path with the absolute path during installation.

## Build and Deployment Process

### 1. Build
```powershell
.\build_cpp.ps1
```

This creates:
- `build\win\aggplugin-agent2.exe`
- `build\win\aggplugin_agent2.dll`
- `build\win\libaggcollector.dll`
- `build\win\*_plugin.dll` (measurement plugins)
- `build\win\aggplugin.conf` (configuration)

### 2. Deploy to Installer Package
Copy files to installer source:
```
instpackage_my1\zabbix2\win\
├── bin\
│   ├── aggplugin-agent2.exe
│   ├── aggplugin_agent2.dll
│   ├── libaggcollector.dll
│   └── plugins\
│       ├── cpu_load_plugin.dll
│       ├── memory_usage_plugin.dll
│       └── disk_stats_plugin.dll
└── conf\
    └── zabbix_agent2.d\
        └── plugins.d\
            └── aggplugin.conf
```

### 3. Package Installer
Build the self-extracting installer with updated files.

### 4. Install
Run installer - aggplugin will be configured automatically.

## Verification After Installation

### Check Service Status
```powershell
Get-Service "Zabbix Agent 2"
```

### Verify Configuration
```powershell
Get-Content C:\zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf
```

Should show:
```ini
Plugins.Aggplugin.PluginPath=C:\zabbix\bin\plugins\*.dll
```
(Absolute path, not relative)

### Test Metrics
```powershell
# Test connectivity
C:\zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin._internal.test"

# Test metrics
C:\zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin.cpu_load[avg1]"
C:\zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin.mem_free[avg1]"
C:\zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin.disk.io.read[avg1]"
C:\zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin.disk.io.write[avg1]"
```

All should return JSON with aggregated statistics.

## Technical Notes

### Path Requirements
Zabbix Agent 2 **requires absolute paths** for external plugins:
- `Plugins.*.System.Path` (in main config)
- `Plugins.*.PluginPath` (in plugin config)

Relative paths will cause:
- Error: "path is not absolute"
- Service startup failure

### Macro Expansion
The installer uses `%%INSTALL_PATH%%` macro which gets expanded to the actual installation directory (e.g., `C:\zabbix`) during installation.

### Plugin Architecture
- **aggplugin-agent2.exe**: External plugin process (communicates via named pipe)
- **libaggcollector.dll**: Core aggregation engine
- **Measurement plugins**: DLLs that collect raw metrics (cpu, memory, disk)
- **Communication**: Agent ↔ Plugin via `\\.\pipe\agent.plugin.sock`

## Troubleshooting

### Service Won't Start
Check logs:
```powershell
Get-Content C:\zabbix\log\agent2\agent.log.txt -Tail 50
Get-Content C:\zabbix\log\aggplugin_debug.log -Tail 50
```

Look for:
- "path is not absolute" → Configuration has relative paths
- "No DLL plugins found" → PluginPath incorrect or DLLs missing
- "Failed to connect to plugin" → aggplugin-agent2.exe not running

### Metrics Return "Unknown metric"
1. Check if aggplugin-agent2.exe is running:
   ```powershell
   Get-Process aggplugin-agent2 -ErrorAction SilentlyContinue
   ```

2. Check plugin debug log:
   ```powershell
   Get-Content C:\zabbix\log\aggplugin_debug.log
   ```

3. Verify DLL plugins are loaded:
   Look for "Found X plugin DLL(s)" in log (should be 3)

### Reinstallation
1. Stop service
2. Delete C:\zabbix
3. Run installer again
4. Configuration will be automatically applied

## Repository Structure
```
aggplugin/
├── cpp_agent_wrapper/        # Agent 2 plugin wrapper
├── cpp_common/                # Core collector engine
├── build_cpp.ps1             # Build script
├── config_template/          # Config templates
│   └── aggplugin.conf
└── build/
    └── win/                  # Build output
```

## Related Documentation
- See `USAGE.md` for plugin usage and metric descriptions
- See `instpackage_my1/CHANGES_AGGPLUGIN.md` for detailed installer changes
