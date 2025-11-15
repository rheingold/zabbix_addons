# Portable Project Notes

This project now supports flexible directory structures for better portability.

## Zabbix Headers (Optional)

The build system will automatically detect zabbixlib in multiple locations:
- `../../../zabbixlib/include` (3 levels up: Cpp/zabbix/aggplugin → Cpp/zabbixlib)
- `../../../../zabbixlib/include` (4 levels up: alternative structure)

**NOTE:** Zabbix headers are OPTIONAL. They're only needed for:
- Building classic agent DLL (`-Variant classic`)
- Building agent2 DLL (`-Variant agent2`)

The Go external plugin (`-Variant agent2`, default) does NOT require Zabbix headers.

## Complete Setup Instructions

See: `../../PORTABLE_SETUP.txt` (in parent Cpp/ directory)

This file contains:
- Complete installation instructions for new systems
- Tool download links (MSYS2, Go, Git)
- Zabbix header setup options
- Build verification steps
- Deployment instructions
- Troubleshooting guide

## Quick Start (Already Have Tools)

If you already have MSYS2/MinGW-w64 and Go installed:

```powershell
# Build everything
.\build_cpp.ps1 -Variant agent2

# Deploy to Zabbix
Copy-Item build\aggplugin-agent2.exe C:\Zabbix\plugins\ -Force
Copy-Item build\libaggcollector.dll C:\Zabbix\plugins\ -Force

# Configure (create aggplugin.conf)
# See config_template/aggplugin.conf

# Restart Agent
Restart-Service "Zabbix Agent 2"
```

## Portable Package

To create a distributable package (no source):

```powershell
# Create package directory
New-Item -ItemType Directory -Force -Path portable-package

# Copy binaries
Copy-Item build\aggplugin-agent2.exe portable-package\
Copy-Item build\libaggcollector.dll portable-package\

# Copy config template
Copy-Item config_template\aggplugin.conf portable-package\

# Copy documentation
Copy-Item README.md portable-package\
Copy-Item USAGE.md portable-package\

# Create ZIP
Compress-Archive -Path portable-package\* -DestinationPath zabbix-aggplugin-portable.zip
```

The resulting ZIP contains everything needed to deploy (no compilation required on target system).
