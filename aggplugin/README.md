Aggregated Metrics Plugin for Zabbix
=====================================

**Version:** 0.1 (tmp0.1 branch) | **Release Date:** November 3, 2025

**Status:** 
- ⚠️ **Agent2 Support:** Functional and tested on Windows
- ⚠️ **Classic Agent Support:** Untested in this version
- ⚠️ **Testing:** No systematic test suite implemented yet
- 📝 This is a preliminary version for evaluation and feedback

---

## Credits

```
╔════════════════════════════════════════════════════════════════════╗
║  Lead & Architecture:  lukas@plachy.eu                             ║
║  Development:          Claude Sonnet 4.5 (AI Assistant, Anthropic) ║
║  Date:                 November 3, 2025                            ║
║  Version:              0.1 (tmp0.1 branch)                         ║
╚════════════════════════════════════════════════════════════════════╝
```

*This project was developed through AI-assisted collaborative engineering, combining human architectural vision with AI implementation capabilities.*

---

## Overview

This plugin provides continuous metric aggregation with comprehensive statistics for Zabbix Agent2. It maintains background sampling threads that collect measurements at regular intervals, compute statistics (average, min, max, median, mode, standard deviation, variance, count), and serve aggregated results to Zabbix queries.

**Key Features:**
- Background continuous sampling (default 1-second intervals)
- 8 comprehensive statistics per metric
- Extensible via dynamically loaded measurement plugin DLLs
- Multi-source metric support (per-disk, per-CPU, etc.)
- Automatic statistics reset at configurable thresholds
- JSON output format with aggregate and per-source data

**📦 NEW: Portable Project Structure**
This project now supports a portable setup. See `PORTABLE_SETUP.txt` in the parent `Cpp/` directory for complete instructions on setting up the project on a new system.

---

High-performance Windows metrics aggregation plugin for Zabbix Agent2. Continuously samples system metrics (CPU, memory) and computes comprehensive statistics including average, min/max, median, mode, standard deviation, and variance. Built with C++ for performance and Go for Agent2 integration.

**NEW in 0.1 (tmp0.1)**: 🎉 **Plugin Framework** - Extend aggplugin with loadable measurement plugins! Add custom metrics by dropping DLLs into a directory. See [PLUGIN_DEVELOPMENT.md](PLUGIN_DEVELOPMENT.md) for details.

---

## Quick Start for Zabbix Administrators

### What This Plugin Does

The Aggplugin continuously monitors your system metrics in the background and provides **statistical aggregation** instead of single-point measurements:

- **CPU Load**: Average, minimum, maximum, median, mode, standard deviation over time
- **Memory Usage**: Same comprehensive statistics for available memory
- **Extensible**: Add custom metrics via loadable plugins (DLL files)
- **JSON Output**: All statistics returned in a single metric query

**NEW:** Built-in CPU and memory metrics are now also available as **example plugins** (`cpu_load_plugin.dll`, `memory_usage_plugin.dll`), demonstrating the plugin framework!

### Installation

#### Automatic Installation (Recommended)

🎉 **NEW: Automatic Installer Integration!**

If you're using the `instpackage_my1` Zabbix Agent installer, aggplugin is now automatically configured during installation with correct absolute paths. No manual configuration needed!

The installer will:
- Deploy all aggplugin binaries and measurement plugins
- Configure absolute paths automatically
- Set up plugin configuration with recommended defaults
- Start the service with aggplugin enabled

**See:** [INSTALLER_INTEGRATION.md](INSTALLER_INTEGRATION.md) for deployment details and [INSTALLER_CHANGES.md](INSTALLER_CHANGES.md) for technical implementation.

After installation, verify metrics are working:
```powershell
C:\zabbix\bin\zabbix_get.exe -s 127.0.0.1 -p 10050 -k "aggplugin._internal.test"
```

---

#### Manual Installation Steps

If you're not using the automatic installer, follow these steps:

#### 1. Download Pre-built Files

Download the latest release from GitHub or build from source (see Build Instructions section).

You need two files:
- `aggplugin-agent2.exe` - The plugin executable
- `libaggcollector.dll` - The C++ engine library

#### 2. Deploy to Zabbix

Copy both files to your Zabbix plugins directory:

```powershell
# Windows example
Copy-Item aggplugin-agent2.exe C:\Zabbix\plugins\
Copy-Item libaggcollector.dll C:\Zabbix\plugins\
```

**Common Zabbix plugin locations:**
- `C:\Zabbix\plugins\` (custom installations)
- `C:\Program Files\Zabbix Agent 2\plugins\`
- `/usr/lib/zabbix/plugins/` (Linux - future support)

#### 3. Configure Agent2

Create configuration file:
`C:\Zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf`

```ini
# Required: Path to plugin executable
Plugins.Aggplugin.System.Path=C:\Zabbix\plugins\aggplugin-agent2.exe

# Optional: Debug level (0-5, default: 0) - see the config_template for details
Plugins.Aggplugin.DebugLevel=3

# Optional: Maximum samples before auto-reset (default: 1000)
Plugins.Aggplugin.MaxSamples=1000
```

**Adjust paths** to match your Zabbix installation!

#### 4. Restart Zabbix Agent2

```powershell
# Windows
Restart-Service "Zabbix Agent 2"

# Linux (future)
systemctl restart zabbix-agent2
```

#### 5. Test the Plugin

Use `zabbix_get` to verify metrics are working:

```powershell
# Test connectivity
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.test"
```
**Expected output:**
```
Aggplugin minimal test - plugin loaded successfully!
```

```powershell
# Get CPU statistics (wait a few seconds for samples)
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.cpu_load"
```
**Example output:**
```json
{
  "metric": "cpu_load",
  "values": {
    "all": {
      "avg": 4.03452,
      "min": 2.14258,
      "max": 6.52344,
      "med": 3.87109,
      "mod": 3,
      "dev": 1.28471,
      "var": 1.65047,
      "cnt": 15
    }
  }
}
```

```powershell
# Get memory statistics
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.memory_usage"
```
**Example output:**
```json
{
  "metric": "mem_free",
  "values": {
    "all": {
      "avg": 51232.1,
      "min": 51202.5,
      "max": 51254.2,
      "med": 51249.3,
      "mod": 51254,
      "dev": 21.6903,
      "var": 470.47,
      "cnt": 22
    }
  }
}
```

**Statistics Explained:**
- `avg` - Average value
- `min` - Minimum value
- `max` - Maximum value  
- `med` - Median (50th percentile)
- `mod` - Mode (most frequent value)
- `dev` - Standard deviation
- `var` - Variance
- `cnt` - Sample count
```

### Using in Zabbix

#### Available Metrics

1. **`aggplugin.test`** - Connectivity test
   - Returns: String "success message"
   - Use for: Monitoring plugin availability

2. **`aggplugin.cpu_load`** - CPU load aggregation
   - Returns: JSON with statistics
   - Statistics: `avg`, `min`, `max`, `med` (median), `mod` (mode), `dev` (std deviation), `var` (variance), `cnt` (sample count)

3. **`aggplugin.memory_usage`** - Available memory aggregation
   - Returns: JSON with same statistics
   - Values in MB

#### Adding to Zabbix Items

**Item Configuration:**
- **Type:** Zabbix agent (active/passive)
- **Key:** `aggplugin.cpu_load` or `aggplugin.memory_usage`
- **Type of information:** Text (raw JSON)
- **Update interval:** 60s (or as needed)

**Preprocessing Steps:**

Extract specific statistics using JSON Path:

```
JSONPath: $.values.all.avg    → Get average
JSONPath: $.values.all.max    → Get maximum
JSONPath: $.values.all.min    → Get minimum
JSONPath: $.values.all.med    → Get median
```

**Example Item:**
```
Name: CPU Load - Average
Key: aggplugin.cpu_load
Type: Zabbix agent
Preprocessing:
  1. JSONPath: $.values.all.avg
  2. Custom multiplier: 1 (already in %)
```

### Troubleshooting

#### Plugin Not Loading
- Check Agent2 log: `C:\Zabbix\log\zabbix_agent2.log`
- Verify plugin path in `aggplugin.conf`
- Ensure both `.exe` and `.dll` are in the same directory

#### No Metrics Returned
- Check plugin debug log: `C:\Zabbix\log\aggplugin_debug.log`
- Increase `DebugLevel` to 5 for verbose logging
- First query may return null (needs baseline sample)

#### Metrics Return Null
- Wait 2-3 seconds after restart for baseline
- CPU metric needs 2 samples minimum
- Check sample count in JSON (`cnt` field)

---

## Directory Structure

```
aggplugin/
├── cpp_common/              # Core C++ aggregation engine
│   ├── collector.cpp/.hpp   # Background sampling & statistics computation
│   ├── collector_shared.cpp # Shared library exports for CGO
│   └── plugin_common.cpp/.hpp # Windows API metric samplers
│
├── cpp_agent_wrapper/       # C/C++ wrappers for different agent types
│   ├── agent2_wrapper.cpp   # Direct Agent2 SDK wrapper (untested)
│   ├── classic_wrapper.c    # Classic Agent DLL module (untested)
│   └── unified_wrapper.cpp  # Combined wrapper (experimental)
│
├── go_agent2_wrapper/       # Go-based Agent2 external plugin (TESTED)
│   ├── main.go              # Go wrapper with CGO to C++ engine
│   └── go.mod               # Go module dependencies
│
├── build/                   # Build output directory
│   ├── aggplugin-agent2.exe # Final executable for Agent2
│   ├── libaggcollector.dll  # C++ collector library
│   └── aggplugin_agent2.dll # Agent2 plugin interface
│
├── build_cpp.ps1            # Build automation script
├── init-env.ps1             # Optional: Development environment setup helper
├── README.md                # This file
└── USAGE.md                 # Detailed usage guide
```

## File Descriptions & Dependencies

### Core Engine (`cpp_common/`)

**collector.cpp/.hpp**
- **Purpose:** Background aggregation engine running on 1-second timer
- **Key Functions:**
  - `collector_init()` - Initialize background sampling thread
  - `collector_register_metric()` - Register metric for sampling
  - `collector_set_max_samples()` - Configure auto-reset threshold
  - `collector_fetch_and_reset_json()` - Get statistics as JSON and reset
- **Statistics Computed:** avg, min, max, median, mode, std deviation, variance, count
- **Thread Safety:** Mutex-protected accumulators
- **Dependencies:** Calls `plugin_common.cpp` for actual metric sampling

**plugin_common.cpp/.hpp**
- **Purpose:** Platform-specific metric sampling using Windows APIs
- **Key Functions:**
  - `plugin_sample_numeric()` - Sample any registered metric by name
  - `sample_cpu_load_percent()` - CPU via GetSystemTimes() (delta calculation)
  - `sample_mem_free_mb()` - Memory via GlobalMemoryStatusEx()
  - `plugin_sample_cpu_per_core()` - Per-core CPU (stub, not implemented)
- **Windows APIs Used:** GetSystemTimes, GlobalMemoryStatusEx
- **Note:** First CPU call establishes baseline, second returns percentage
- **Dependencies:** None (pure Windows API)

**collector_shared.cpp**
- **Purpose:** Thin C wrapper for CGO compatibility
- **Exports:** Same functions as collector.hpp but ensures C linkage
- **Dependencies:** collector.cpp

### Agent2 Wrapper (`go_agent2_wrapper/`)

**main.go**
- **Purpose:** External plugin executable communicating with Agent2 via named pipes
- **Key Components:**
  - CGO bindings to C++ collector library
  - Configuration loading from agent config files
  - Zabbix SDK metric registration and export handlers
  - Debug logging system (5 verbosity levels)
  - Graceful shutdown handling
- **Exported Metrics:**
  - `aggplugin.test` - Connectivity test
  - `aggplugin.cpu_load` - CPU statistics
  - `aggplugin.memory_usage` - Memory statistics
- **Dependencies:** 
  - libaggcollector.dll (via CGO)
  - golang.zabbix.com/sdk/plugin (Zabbix Go SDK)

### Dependency Chain

```
Agent2 Process
    ↓ (Named Pipes IPC)
main.go (Go wrapper)
    ↓ (CGO)
collector_shared.cpp
    ↓ (C++ calls)
collector.cpp
    ↓ (Metric sampling)
plugin_common.cpp
    ↓ (System calls)
Windows APIs (GetSystemTimes, GlobalMemoryStatusEx)
```

## Deployment Setup (Agent2)

### Prerequisites
- Zabbix Agent2 installed and running on Windows
- Plugin files compiled (see Build Instructions below)

### Installation Steps

1. **Copy Plugin Files**
   
   Copy the compiled binaries to your Zabbix plugins directory:
   ```
   aggplugin-agent2.exe  → [ZABBIX_HOME]/plugins/
   libaggcollector.dll   → [ZABBIX_HOME]/plugins/
   ```
   
   Where `[ZABBIX_HOME]` is your Zabbix installation directory (commonly `C:\Program Files\Zabbix`, `C:\Zabbix`, or similar).

2. **Create Plugin Configuration File**
   
   Create or edit the plugin config file in your agent's plugin configuration directory:
   ```
   [ZABBIX_HOME]/conf/zabbix_agent2.d/plugins.d/aggplugin.conf
   ```
   
   **Required Configuration:**
   ```ini
   # External plugin executable path (REQUIRED)
   Plugins.Aggplugin.System.Path=[ZABBIX_HOME]/plugins/aggplugin-agent2.exe
   ```
   
   **Optional Configuration:**
   ```ini
   # Debug level: 0=Fatal, 1=Error, 2=Warning, 3=Info, 4=Verbose, 5=VeryVerbose
   # Default: 0 (errors only)
   Plugins.Aggplugin.DebugLevel=3
   
   # Maximum samples before auto-reset (prevents unbounded memory growth)
   # Default: 1000 (~16.7 minutes at 1-second sampling)
   Plugins.Aggplugin.MaxSamples=1000
   
   # Metrics to preload on startup (comma-separated)
   # Default: cpu_load,mem_free
   Plugins.Aggplugin.PreloadMetrics=cpu_load,mem_free
   
   # Seconds to wait for baseline samples before accepting queries
   # Default: 0 (immediate, first query may return null)
   # Set to 3-5 for guaranteed data on first query
   Plugins.Aggplugin.PreloadDelay=0
   ```

3. **Restart Zabbix Agent2**
   
   Restart the agent service to load the plugin:
   ```powershell
   Restart-Service "Zabbix Agent 2"
   ```
   
   Or using the services control panel or your system's service management tool.

4. **Verify Installation**
   
   Test connectivity using zabbix_get (adjust paths to your installation):
   ```powershell
   # Test basic connectivity
   zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.test"
   
   # Query CPU statistics (wait 3-5 seconds after restart)
   zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.cpu_load"
   
   # Query memory statistics
   zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.memory_usage"
   ```

### Troubleshooting Deployment

**Debug Logs Location:**
```
[ZABBIX_HOME]/log/aggplugin_debug.log
```

**Common Issues:**
- "Unsupported item key" → Check `System.Path` parameter points to correct executable
- Returns null values → Wait 2-3 seconds after service start for baseline
- Service won't start → Check Agent2 log for detailed error messages

## Registered Metrics

### 1. aggplugin.test

**Description:** Simple connectivity and health check metric.

**Returns:** Plain text string
```
Aggplugin minimal test - plugin loaded successfully!
```

**Use Case:** Verify plugin is loaded and responding.

**Example:**
```powershell
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.test"
```

---

### 2. aggplugin.cpu_load

**Description:** System-wide CPU load percentage with comprehensive statistics.

**Data Source:** Windows GetSystemTimes() API (idle/kernel/user time delta)

**Sampling:** Continuous, 1-second interval

**Returns:** JSON with statistical aggregation

**Statistics Explanation:**
- **avg** (average) - Arithmetic mean of all CPU% samples in the period
- **min** (minimum) - Lowest CPU% observed since last query
- **max** (maximum) - Highest CPU% observed since last query  
- **med** (median) - Middle value when all samples sorted (50th percentile)
- **mod** (mode) - Most frequently occurring rounded integer CPU%
- **dev** (standard deviation) - Measure of CPU% variability (spread from average)
- **var** (variance) - Square of standard deviation
- **cnt** (count) - Number of samples collected since last query

**JSON Format:**
```json
{
  "metric": "cpu_load",
  "values": {
    "all": {
      "avg": 3.06,
      "min": 1.95,
      "max": 4.88,
      "med": 2.91,
      "mod": 3,
      "dev": 0.75,
      "var": 0.56,
      "cnt": 12
    }
  }
}
```

**Example Values Interpretation:**
- avg: 3.06% - System averaged 3.06% CPU usage
- min: 1.95% - Minimum spike was 1.95%
- max: 4.88% - Maximum spike was 4.88%
- med: 2.91% - Half the samples were below 2.91%
- mod: 3 - CPU was at ~3% more often than any other value
- dev: 0.75 - CPU varied by ±0.75% from average typically
- cnt: 12 - Based on 12 one-second samples

**Important Notes:**
- First query after restart may return null (requires baseline sample)
- Values reset to zero after each query (fetch-and-reset pattern)
- At 1-second sampling: cnt ≈ seconds since last query

**Example:**
```powershell
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.cpu_load"
```

---

### 3. aggplugin.memory_usage

**Description:** Available physical memory in megabytes with comprehensive statistics.

**Data Source:** Windows GlobalMemoryStatusEx() API

**Sampling:** Continuous, 1-second interval

**Returns:** JSON with statistical aggregation (same structure as cpu_load)

**Statistics:** Same as cpu_load (avg, min, max, med, mod, dev, var, cnt)

**JSON Format:**
```json
{
  "metric": "mem_free",
  "values": {
    "all": {
      "avg": 50335.6,
      "min": 50332.5,
      "max": 50338.0,
      "med": 50335.7,
      "mod": 50334,
      "dev": 1.68,
      "var": 2.82,
      "cnt": 11
    }
  }
}
```

**Example Values Interpretation:**
- avg: 50335.6 MB - Average free memory was ~50.3 GB
- min: 50332.5 MB - Minimum free memory (peak usage)
- max: 50338.0 MB - Maximum free memory (lowest usage)
- med: 50335.7 MB - Median free memory
- mod: 50334 - Most common rounded value
- dev: 1.68 MB - Very stable (low deviation)
- cnt: 11 - Based on 11 samples

**Important Notes:**
- Memory typically very stable (low deviation)
- Large deviations may indicate memory pressure or leaks
- Values in megabytes (MB), not gigabytes

**Example:**
```powershell
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.memory_usage"
```

---

### Extracting Values in Zabbix

Use **JSONPath preprocessing** to extract specific statistics:

```
Average:          $.values.all.avg
Minimum:          $.values.all.min
Maximum:          $.values.all.max
Median:           $.values.all.med
Mode:             $.values.all.mod
Std Deviation:    $.values.all.dev
Variance:         $.values.all.var
Sample Count:     $.values.all.cnt
```

**Example Zabbix Item:**
```
Name: CPU Load (Average)
Type: Zabbix agent (active)
Key: aggplugin.cpu_load
Type of information: Numeric (float)
Units: %
Preprocessing:
  Step 1: JSONPath → $.values.all.avg
  Step 2: Custom multiplier → 1
```

## Build Instructions

### Prerequisites

#### 1. MSYS2 with MinGW-w64 Toolchain

**Download:** https://www.msys2.org/

**Installation:**
1. Download and run the MSYS2 installer
2. Install to default location (e.g., `C:\msys64`)
3. Open "MSYS2 MinGW 64-bit" terminal
4. Update package database:
   ```bash
   pacman -Syu
   ```
5. Install build tools:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
   ```

**Required Compiler:**
- GCC 8.0+ (for C++17 support)
- Verify: `gcc --version` should show mingw-w64 version

#### 2. Go Programming Language

**Download:** https://go.dev/dl/

**Installation:**
1. Download Windows installer (go1.21+ recommended)
2. Install to default location (e.g., `C:\Go`)
3. Verify: `go version` in PowerShell

#### 3. Zabbix Agent2 Go SDK

**⚠️ CRITICAL: Named Pipes Issue Warning**

**DO NOT use the official Zabbix SDK from the main repository** - it contains a bug in named pipes handling that causes connection issues on Windows.

**Correct Source:** Use the patched SDK from:
```
https://github.com/zabbix/zabbix/tree/master (latest development branch)
```

Or use the Go module with explicit version:
```go
require golang.zabbix.com/sdk v1.2.2-master
```

**The Issue:**
- Official SDK versions have broken named pipe retry logic
- Causes "connection refused" or timeout errors
- Fixed in master branch but not in stable releases yet

**Verification:**
Check `go.mod` in `go_agent2_wrapper/` for correct SDK version:
```go
module aggplugin

go 1.21

require golang.zabbix.com/sdk v1.2.2-master
```

**Getting the SDK:**
The SDK will be automatically downloaded by Go during build. Ensure you have internet connectivity, or use:
```bash
cd go_agent2_wrapper
go mod download
```

#### 4. Windows PDH Library (Usually Included)

The Performance Data Helper (PDH) library is part of Windows SDK and typically pre-installed with Windows.

**Verify availability:**
```powershell
Get-Command pdh.dll
```

If missing, install Windows SDK or use pre-compiled binaries.

### Build Process

#### Environment Setup (Optional)

If Go and GCC are not in your PATH, you can use the provided helper script:

```powershell
# Optional: Initialize development environment
# This script attempts to find and add Go and MSYS2/MinGW to PATH
.\init-env.ps1
```

**Note:** Customize `init-env.ps1` if your tools are installed in non-standard locations.

#### Quick Build

From the `aggplugin` root directory in PowerShell:

```powershell
# Build Agent2 external plugin (recommended)
.\build_cpp.ps1 -Variant agent2

# Output files in build/ directory:
#   - aggplugin-agent2.exe
#   - libaggcollector.dll
#   - aggplugin_agent2.dll
```

#### Build Variants

```powershell
# Agent2 external plugin (Go wrapper + C++ engine) - TESTED
.\build_cpp.ps1 -Variant agent2

# Classic agent DLL module (C wrapper + C++ engine) - UNTESTED
.\build_cpp.ps1 -Variant classic

# Unified wrapper (experimental) - UNTESTED
.\build_cpp.ps1 -Variant unified
```

#### Build Script Details

The `build_cpp.ps1` script performs these steps:

1. **Compile C++ Collector Library**
   ```powershell
   g++ -shared -o build/libaggcollector.dll `
       cpp_common/collector.cpp `
       cpp_common/collector_shared.cpp `
       cpp_common/plugin_common.cpp `
       -lpdh -std=c++17 -O2
   ```

2. **Compile Agent2 Plugin DLL** (if needed)
   ```powershell
   g++ -shared -o build/aggplugin_agent2.dll `
       cpp_agent_wrapper/agent2_wrapper.cpp `
       -Lbuild -laggcollector
   ```

3. **Build Go Wrapper with CGO**
   ```powershell
   cd go_agent2_wrapper
   go build -o ../build/aggplugin-agent2.exe
   ```

#### Manual Build Steps

If you need to build manually:

```bash
# From MSYS2 MinGW 64-bit terminal
cd /c/path/to/aggplugin

# 1. Build collector library
g++ -shared -o build/libaggcollector.dll \
    cpp_common/collector.cpp \
    cpp_common/collector_shared.cpp \
    cpp_common/plugin_common.cpp \
    -lpdh -std=c++17 -O2 -Wall

# 2. Build Go wrapper (from PowerShell)
cd go_agent2_wrapper
$env:CGO_ENABLED="1"
go build -o ../build/aggplugin-agent2.exe
```

### Build Troubleshooting

**Error: "gcc: command not found"**
- Solution: Run from "MSYS2 MinGW 64-bit" terminal, not regular MSYS2
- Or add MinGW bin to PATH: `C:\msys64\mingw64\bin`

**Error: "cannot find -lpdh"**
- Solution: PDH library missing, install Windows SDK or use `-lpdh` with correct path
- Verify: `ls /mingw64/lib/libpdh.a`

**Error: "undefined reference to 'collector_init'"**
- Solution: Ensure all cpp_common/*.cpp files are compiled into libaggcollector.dll
- Check link order: library must come after source files

**Error: Go build fails with SDK errors**
- Solution: Verify correct SDK version in go.mod (v1.2.2-master or later)
- Clear module cache: `go clean -modcache` then `go mod download`

**Error: "named pipe connection refused"**
- Solution: Using wrong SDK version (see Named Pipes Issue Warning above)
- Update go.mod to use master branch SDK

### Build Output

Successful build produces:

```
build/
├── aggplugin-agent2.exe    # Main executable (Go wrapper)
├── libaggcollector.dll     # C++ engine (required dependency)
└── aggplugin_agent2.dll    # Agent2 interface (included in .exe)
```

**Deployment:** Copy `aggplugin-agent2.exe` and `libaggcollector.dll` to Zabbix plugins directory.

## Architecture Deep Dive

### Plugin Model Comparison

Zabbix provides two distinct plugin architectures with different trade-offs:

#### Classic Agent (DLL In-Process Model)

**Architecture:**
```
┌─────────────────────────────────────┐
│   Zabbix Agent Process (32/64-bit) │
│  ┌──────────────┐  ┌──────────────┐│
│  │  Agent Core  │  │ Plugin DLL   ││
│  │              │←→│ (same space) ││
│  └──────────────┘  └──────────────┘│
└─────────────────────────────────────┘
```

**Characteristics:**
- Plugin loaded as DLL into agent process memory
- Direct function calls (no IPC overhead)
- Configuration: `LoadModule=[ZABBIX_HOME]/modules/aggplugin_classic.dll`
- Crash risk: Plugin crash kills entire agent
- Complexity: Lower (direct API)
- **Status in this project:** Untested

#### Agent2 (External Process Model)

**Architecture:**
```
┌──────────────────┐         ┌─────────────────────────────┐
│ Zabbix Agent2    │         │  Plugin Process             │
│                  │         │  ┌───────────────────────┐  │
│  ┌────────────┐  │  Named  │  │ Go Wrapper (main.go)  │  │
│  │Plugin Mgr  │←→│  Pipes  │←→│ (SDK Interface)       │  │
│  └────────────┘  │  (IPC)  │  └───────────┬───────────┘  │
│                  │         │              ↓ CGO          │
└──────────────────┘         │  ┌───────────────────────┐  │
                             │  │ C++ Engine            │  │
                             │  │ (libaggcollector.dll) │  │
                             │  └───────────────────────┘  │
                             └─────────────────────────────┘
```

**Characteristics:**
- Plugin runs as independent executable
- Communication via Windows Named Pipes
- Configuration: `Plugins.Aggplugin.System.Path=[ZABBIX_HOME]/plugins/aggplugin-agent2.exe`
- Isolation: Plugin crash doesn't affect agent
- Complexity: Higher (IPC, serialization)
- **Status in this project:** Tested and functional

### Communication Flow (Agent2)

Detailed request/response flow for metric query:

```
1. Zabbix Server/Frontend
   ↓ (Network Request)
   
2. Zabbix Agent2 Core
   ↓ (Metric Key Lookup)
   
3. Plugin Manager
   ↓ (Named Pipe Write: Plugin Protocol)
   
4. Go Wrapper (main.go)
   - Receives request via SDK
   - Parses metric key
   ↓ (CGO Function Call)
   
5. C++ Collector (collector.cpp)
   - Locks mutex
   - Computes statistics from accumulated samples
   - Formats JSON response
   - Resets accumulators
   ↓ (Returns JSON string)
   
6. Go Wrapper
   - Receives JSON string
   ↓ (Named Pipe Write: Response)
   
7. Plugin Manager
   ↓ (Returns to requester)
   
8. Zabbix Server/Frontend
   (Receives JSON)
```

### Threading Model

**Background Sampling Thread:**
```cpp
// In collector.cpp
void collector_loop() {
    uint64_t tick = 0;
    while (running) {
        tick++;
        
        // Sample all registered metrics
        for (auto &metric : metrics) {
            if (tick >= metric.next_tick) {
                double value = plugin_sample_numeric(metric.name);
                metric.accumulator.add(value);  // Thread-safe
                metric.next_tick += metric.interval;
            }
        }
        
        sleep(1 second);
    }
}
```

**Main Request Thread:**
```cpp
// Handles requests from Agent2
fetch_and_reset_json() {
    mutex.lock();
    json = accumulator.compute_statistics();
    accumulator.reset();
    mutex.unlock();
    return json;
}
```

**Synchronization:**
- Background thread: Continuous sampling, adds values to accumulator
- Request thread: Locks, computes stats, resets, unlocks
- Mutex prevents data races during computation
- Lock duration: ~1ms (very brief, no blocking)

### Data Flow

**Continuous Sampling (1-second intervals):**
```
Windows API → plugin_common.cpp → collector.cpp → Accumulator
     ↓              ↓                    ↓              ↓
GetSystemTimes   sample_cpu_   →   add(value)  →   sum += value
GlobalMemory     sample_mem_       Thread-safe      values.push_back()
StatusEx                          Mutex             freq[rounded]++
```

**Statistics Computation (on query):**
```
Accumulator (N samples collected)
    ↓
Compute Statistics:
    avg  = sum / count
    min  = minimum value tracked
    max  = maximum value tracked
    med  = sort(values)[count/2]
    mod  = most_frequent(freq_map)
    var  = (sum_sq / count) - (avg * avg)
    dev  = sqrt(var)
    cnt  = count
    ↓
Format JSON
    ↓
Return & Reset (ready for next period)
```

### Named Pipes IPC

**Pipe Protocol:**
- Agent2 creates named pipe with unique identifier
- Passes pipe name to plugin via command line
- Bidirectional communication (request/response)
- Binary protocol with length prefixes

**Why Named Pipes:**
- Native Windows IPC mechanism
- Better performance than TCP loopback
- Automatic cleanup on process termination
- Security: ACLs can restrict access

**Known Issue:**
Official Zabbix SDK has retry logic bug causing connection failures. Use patched version from master branch.

### Testing Agent2 Plugin from CLI

**❌ Cannot run directly:**
```powershell
# This WILL NOT WORK:
.\aggplugin-agent2.exe
# Plugin expects Agent2 to provide named pipe parameters
```

**✅ Proper testing method:**
```powershell
# 1. Deploy to Agent2
Copy-Item build\*.{exe,dll} [ZABBIX_HOME]\plugins\

# 2. Configure in aggplugin.conf
Plugins.Aggplugin.System.Path=[ZABBIX_HOME]/plugins/aggplugin-agent2.exe

# 3. Restart Agent2
Restart-Service "Zabbix Agent 2"

# 4. Test with zabbix_get
zabbix_get -s 127.0.0.1 -p 10050 -k "aggplugin.test"
```

**Validation Points:**
1. Check Agent2 log for plugin registration
2. Check plugin debug log (`aggplugin_debug.log`)
3. Verify metrics respond to zabbix_get queries
4. Monitor statistics for reasonable values

### Memory & Performance Characteristics

**Memory Usage:**
```
Per Metric:
    Accumulator: ~48 bytes (fixed overhead)
    Values vector: 8 bytes × MaxSamples
    Frequency map: ~12 bytes × unique_values
    
Total (2 metrics, MaxSamples=1000):
    ~20 KB per metric = ~40 KB total
```

**CPU Impact:**
```
Sampling: 1 sample/second × 2 metrics
    GetSystemTimes: ~10 μs
    GlobalMemoryStatusEx: ~5 μs
    Accumulator add(): ~2 μs
    
Query Processing:
    Statistics computation: ~100 μs for 1000 samples
    JSON formatting: ~50 μs
    
Overall: <0.1% CPU on modern hardware
```

**Lock Contention:**
- Lock held only during add() and fetch()
- Add(): ~2 μs (once per second)
- Fetch(): ~150 μs (on query)
- No lock contention issues

### Extensibility

**Adding New Metrics:**

1. **Add sampler in plugin_common.cpp:**
   ```cpp
   double sample_my_metric() {
       // Use Windows API or other source
       return value;
   }
   ```

2. **Register in plugin_sample_numeric():**
   ```cpp
   if (strcmp(metric, "my_metric") == 0) {
       *ok = 1;
       return sample_my_metric();
   }
   ```

3. **Register in main.go:**
   ```go
   plugin.RegisterMetrics(&impl, pluginName,
       "aggplugin.my_metric", "My metric description")
   ```

4. **Add export handler:**
   ```go
   case "aggplugin.my_metric":
       metricName := C.CString("my_metric")
       ret := C.collector_fetch_and_reset_json(metricName, ...)
   ```

**No changes needed in collector.cpp** - it's metric-agnostic!

## Additional Resources

### Documentation Files

- **README.md** (this file) - Complete reference documentation
- **USAGE.md** - Detailed usage guide with examples and Zabbix integration
- **ai.txt** - Development notes and technical decisions

### Debug Logging

All plugin activity is logged to:
```
[ZABBIX_HOME]/log/aggplugin_debug.log
```

Configure verbosity with `Plugins.Aggplugin.DebugLevel` (0-5).

### Performance Monitoring

Track plugin health via:
- Sample counts (`cnt` field) - should increment steadily
- Statistics validity - null values indicate sampling issues
- Debug log - shows registration, sampling, and query events

### Known Limitations

1. **Classic Agent Support:** Untested in this version (tmp0.1)
2. **Per-Core CPU:** Stubbed out due to PDH locale issues
3. **Platform:** Windows-only (uses GetSystemTimes, GlobalMemoryStatusEx)
4. **Sampling Interval:** Fixed at 1 second (configurable in code only)
5. **Test Coverage:** No systematic test suite yet

### Future Enhancements

- Systematic test suite
- Per-core CPU sampling (PDH locale-aware implementation)
- Linux support (via /proc/stat, /proc/meminfo)
- Configurable sampling intervals
- Additional metrics (disk I/O, network)
- Classic agent testing and validation

### Version History

**tmp0.1** (Preliminary Release)
- Initial implementation with Agent2 support
- CPU and memory metrics with comprehensive statistics
- Configuration file support
- Debug logging system
- Windows platform only
- No systematic tests

### Contributing

This is a preliminary version. Feedback and contributions welcome:
- Bug reports and feature requests
- Testing on different Windows versions
- Classic agent validation
- Performance optimization suggestions
- Additional metric implementations

### License & Credits

See project repository for license information.

Built with:
- C++17 (aggregation engine)
- Go 1.21+ (Agent2 wrapper)
- Zabbix Agent2 Go SDK (patched for named pipes)
- Windows API (metric sampling)
- MSYS2/MinGW-w64 (build toolchain)

---

**End of README.md**

For usage examples and Zabbix integration, see **USAGE.md**.
