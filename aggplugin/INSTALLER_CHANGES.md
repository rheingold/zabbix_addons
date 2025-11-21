# Installer Package Modifications Summary

## For: instpackage_my1 (Zabbix Agent Installer)

**Note**: The installer package (instpackage_my1) is not tracked in git. This document serves as a record of changes made to support aggplugin automatic configuration.

## Changes Required in Installer Package

If you need to rebuild or modify the installer package, apply these changes:

### File 1: `zabbix2/win/install/.baseval.ini`

**Add this section** at the end of the file:

```ini
[%%INSTALL_PATH%%\conf\zabbix_agent2.d\plugins.d\aggplugin.conf]
;;INST|m|Plugin DLL path (auto-configured)
Plugins.Aggplugin.PluginPath=%%INSTALL_PATH%%\bin\plugins\*.dll
Plugins.Aggplugin.DebugLevel=4
Plugins.Aggplugin.MaxSamples=1000
Plugins.Aggplugin.PreloadMetrics=cpu_load,mem_free,disk.io.read,disk.io.write
Plugins.Aggplugin.PreloadDelay=2
```

**Purpose**: Defines configuration for aggplugin.conf with macro-based absolute paths.

---

### File 2: `zabbix2/win/install/inied.bat`

**Modify line 304** (in the `:op_process` subroutine):

Find this code:
```batch
IF "!line:~-1!"=="]" (
    SET "currentFile=!line:~1,-1!"
    if "!currentFile!"=="default" set "currentfile=%key%"
    ECHO Processing directory: !currentFile!
)
```

**Add these two lines** after `if "!currentFile!"=="default"`:
```batch
IF "!line:~-1!"=="]" (
    SET "currentFile=!line:~1,-1!"
    if "!currentFile!"=="default" set "currentfile=%key%"
    rem Expand macros in section name (e.g., %%INSTALL_PATH%%)
    call :ExpandMacros "!currentFile!" currentFile
    ECHO Processing directory: !currentFile!
)
```

**Purpose**: Enables macro expansion in `.baseval.ini` section names, allowing `%%INSTALL_PATH%%` to be replaced with actual installation path.

---

### File 3: `zabbix2/win/install/install.bat`

**Remove lines 316-324** from the `:CFG_INSTALL` subroutine.

These lines (if present):
```batch
REM Process aggplugin.conf (for Agent 2 only)
if "%agentno%"=="2" (
    pushd "%INSTALL_PATH%"
    set "AGGPLUGIN_CONF=%INSTALL_PATH%\conf\zabbix_agent2.d\plugins.d\aggplugin.conf"
    if exist "!AGGPLUGIN_CONF!" (
        call inied.bat proc "%CURR%\install\.baseval.ini" "!AGGPLUGIN_CONF!" "aggplugin.conf" "##::"
    )
    popd
)
```

**Purpose**: Remove special-case aggplugin handling. With macro expansion in `.baseval.ini`, aggplugin configuration is handled automatically like any other config file.

---

## Files to Include in Installer Package

Make sure these aggplugin files are included in the installer:

```
zabbix2/win/
├── bin/
│   ├── aggplugin-agent2.exe          # External plugin executable
│   ├── aggplugin_agent2.dll          # Agent 2 wrapper
│   ├── libaggcollector.dll           # Core collector library
│   └── plugins/
│       ├── cpu_load_plugin.dll       # CPU monitoring
│       ├── memory_usage_plugin.dll   # Memory monitoring
│       └── disk_stats_plugin.dll     # Disk I/O monitoring
└── conf/
    └── zabbix_agent2.d/
        └── plugins.d/
            └── aggplugin.conf        # Plugin configuration template
```

The `aggplugin.conf` template should contain:
```ini
### Configuration for Aggplugin

Plugins.Aggplugin.DebugLevel=4
Plugins.Aggplugin.MaxSamples=1000
Plugins.Aggplugin.PreloadMetrics=cpu_load,mem_free,disk.io.read,disk.io.write
Plugins.Aggplugin.PreloadDelay=2
# PluginPath: Relative path to measurement plugin DLLs (relative to installation root)
Plugins.Aggplugin.PluginPath=\bin\plugins\*.dll
```

## How It Works After Changes

1. **User runs installer** (interactive or silent mode)
2. **Files copied** to chosen installation path (e.g., `C:\zabbix`)
3. **Configuration phase**: `inied.bat proc` reads `.baseval.ini`
4. **Section processing**:
   - Section name `[%%INSTALL_PATH%%\conf\...\aggplugin.conf]` → `[C:\zabbix\conf\...\aggplugin.conf]`
   - Values with macros also expanded: `%%INSTALL_PATH%%\bin\plugins\*.dll` → `C:\zabbix\bin\plugins\*.dll`
5. **File updated**: `C:\zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf` gets absolute paths
6. **Service installed** and started
7. **Result**: Plugin loads automatically, metrics available immediately

## Testing After Rebuild

After rebuilding the installer with these changes, test:

```powershell
# Run installer
.\zabbix-agent-installer.bat

# Verify config has absolute paths
Get-Content C:\zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf

# Should show:
# Plugins.Aggplugin.PluginPath=C:\zabbix\bin\plugins\*.dll

# Test metrics
C:\zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin._internal.test"
C:\zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin.cpu_load[avg1]"
```

## Related Files

- See `INSTALLER_INTEGRATION.md` for deployment and usage documentation
- Copy `CHANGES_AGGPLUGIN.md` to the installer package root for detailed change documentation

## Version Information

- **Date Modified**: 2025-11-21
- **Tested With**: Zabbix Agent 2.0, Windows 10/11
- **Aggplugin Version**: Compatible with current build
