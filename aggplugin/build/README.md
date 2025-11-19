# Build Directory Structure

This directory contains compiled binaries organized by platform.

## Directory Layout

```
build/
├── win/                    # Windows binaries
│   ├── aggplugin-agent2.exe
│   ├── libaggcollector.dll
│   ├── aggplugin_agent2.dll
│   ├── aggplugin_classic.dll
│   ├── aggplugin.conf
│   └── test_multisource.ps1
├── lin/                    # Linux binaries (future)
│   └── (empty - Linux support planned)
├── plugins/                # Measurement plugins (platform-specific)
│   ├── cpu_load_plugin.dll (Windows)
│   ├── memory_usage_plugin.dll
│   └── disk_stats_plugin.dll
├── aggplugin.conf         # Configuration template
└── test_multisource.ps1   # Test script

```

## Platform-Specific Files

### Windows (win/)
- **aggplugin-agent2.exe** - Main executable (Go wrapper with CGO)
- **libaggcollector.dll** - C++ collector engine (required dependency)
- **aggplugin_agent2.dll** - Agent2 interface (embedded in .exe)
- **aggplugin_classic.dll** - Classic agent loadable module
- **aggplugin.conf** - Configuration template for Windows paths
- **test_multisource.ps1** - PowerShell test script

### Linux (lin/)
- Not yet implemented
- Will contain: aggplugin-agent2, libaggcollector.so, build.sh, etc.

### Measurement Plugins (plugins/)
- Platform-specific DLLs/SOs for custom metrics
- Dynamically loaded at runtime
- Windows: .dll files
- Linux: .so files (future)

## Deployment

### Windows
```powershell
# Copy main plugin
Copy-Item win\aggplugin-agent2.exe C:\Zabbix\plugins\
Copy-Item win\libaggcollector.dll C:\Zabbix\plugins\

# Copy measurement plugins
Copy-Item plugins\*.dll C:\Zabbix\plugins\measurements\

# Configure
Copy-Item win\aggplugin.conf C:\Zabbix\conf\zabbix_agent2.d\plugins.d\

# Restart Agent
Restart-Service "Zabbix Agent 2"
```

### Linux (Future)
```bash
# Copy main plugin
cp lin/aggplugin-agent2 /usr/local/zabbix/plugins/
cp lin/libaggcollector.so /usr/local/zabbix/plugins/

# Copy measurement plugins
cp plugins/*.so /usr/local/zabbix/plugins/measurements/

# Configure
cp lin/aggplugin.conf /etc/zabbix/zabbix_agent2.d/plugins.d/

# Restart Agent
systemctl restart zabbix-agent2
```

## Build Process

Windows binaries are built using:
```powershell
.\build_cpp.ps1 -Variant agent2
```

This creates all binaries in the `win/` subdirectory.

## Notes

- Configuration files and test scripts are duplicated in platform-specific directories for convenience
- The `plugins/` directory contains measurement plugins that work across platforms (with appropriate extension)
- HTML documentation (if any) should be placed at the build/ root level, not in platform subdirectories
