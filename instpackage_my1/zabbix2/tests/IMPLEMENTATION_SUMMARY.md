# Macro Expansion Architecture - Implementation Summary

## Overview
Successfully implemented parameter-based macro expansion for inied.bat, replacing the environment variable approach with explicit macro maps passed as parameters.

## Changes Made

### 1. Core: win/install/inied.bat
- **Added 7th parameter**: `macroparam` for passing macro definitions
- **New :CollectMacrosFromIni function**: Reads macros from `[inied.bat-macros]` section
- **Rewritten :ExpandMacros function**: 
  - Now accepts macro map as 3rd parameter (pipe-separated NAME=Value pairs)
  - Uses PowerShell for reliable string replacement (handles paths with colons correctly)
  - Expands `%%MACRONAME%%` patterns in both file paths and values
- **op_process enhancement**: 
  - Prepares macros_str from macroparam (supports "auto" keyword or explicit map)
  - Passes macros to ExpandMacros for all path and value expansions

### 2. Interactive Installer: win/install/install.bat
- **Updated CFG_INSTALL**: Passes `INSTALL_PATH=%DSTPATH%` macro to proc operation
- **Updated SILENT_INSTALL**: Passes `INSTALL_PATH=!INSTALL_PATH!` macro to proc operation
- **Parameter order**: `call inied.bat proc "baseval.ini" "target.conf" "default" "##::" "" "INSTALL_PATH=path"`

### 3. Download Installer: win/install/install_with_download.bat
- **Fixed section parameter**: Changed from "##::" to "default" for SET operations
- **Added macro parameter**: Passes `INSTALL_PATH=%CURR%` to proc operation

### 4. Web Bootstrapper: index.html
- **Updated SET calls**: Added "##::" as instnote parameter for update tracking
- **Parameter order comment**: Added clarification of inied.bat parameter order
- **Macro preservation**: Bootstrap SET operations preserve `%%INSTALL_PATH%%` macros in .baseval.ini
- **Flow integration**: Bootstrap → SILENT_INSTALL → proc with macros works correctly

### 5. Unit Tests Created (test-temp/)
- **test-macro-explicit.bat**: Tests explicit macro map (INSTALL_PATH=C:\Zabbix|HOSTNAME=Customer-TestHost)
  - ✅ LogFile macro expanded correctly
  - ✅ Hostname macro expanded correctly  
  - ✅ Source baseval.ini preserved
  - ✅ No unexpanded macros in output

- **test-macro-auto.bat**: Tests "auto" mode reading from [inied.bat-macros] section
  - ✅ LogFile expanded from [inied.bat-macros]
  - ✅ Hostname expanded from [inied.bat-macros]
  - ✅ Section preserved in baseval.ini
  - ✅ No unexpanded macros in output

- **test-bootstrap-cycles.bat**: Tests repeated SET calls don't cause %% explosion
  - ✅ Pattern %%INSTALL_PATH%% preserved after 6 cycles
  - ✅ No 4-percent explosion (%%%%%%%%)
  - ✅ No 8-percent explosion (%%%%%%%%%%%%%%%%)

- **test-full-integration.bat**: Tests complete bootstrap → proc → install flow
  - ✅ ServerActive updated correctly
  - ✅ Hostname updated correctly
  - ✅ LogFile macro expanded to install path
  - ✅ Baseval.ini macros preserved
  - ✅ Config has no unexpanded macros

## Usage

### Explicit Macro Map
```batch
call inied.bat proc "baseval.ini" "target.conf" "default" "##::" "" "INSTALL_PATH=C:\Zabbix|HOSTNAME=Customer-01"
```

### Auto Mode (reads from [inied.bat-macros])
```batch
REM baseval.ini contains:
REM [inied.bat-macros]
REM INSTALL_PATH=C:\Zabbix
REM HOSTNAME=Customer-01

call inied.bat proc "baseval.ini" "target.conf" "default" "##::" "" "auto"
```

### Backwards Compatible (no macros)
```batch
call inied.bat proc "baseval.ini" "target.conf" "default" "##::"
REM Works as before - no macro expansion
```

## Parameter Order Reference
```
inied.bat proc <inifile> <targetfile> <section> <instnote> <unused-section> <macroparam>
                 %2         %3           %4        %5           %6              %7
```

## Key Benefits
1. **No environment variable pollution**: Macros passed explicitly
2. **Handles special characters**: PowerShell replacement handles paths with colons, pipes, etc.
3. **Auto mode**: Can read macros from dedicated [inied.bat-macros] section
4. **No %% explosion**: Conditional doubling only for modified lines prevents exponential growth
5. **Backwards compatible**: Existing callers without macroparam continue to work

## Testing Status
- ✅ Explicit macro map expansion
- ✅ Auto mode macro reading  
- ✅ Bootstrap cycle preservation
- ✅ All installer variants updated (install.bat, install_with_download.bat, index.html)
- ✅ Full end-to-end integration test passes

## Notes
- Macro values can include spaces, backslashes, colons
- Pipe character (|) is the separator - avoid in macro values
- Empty lines preserved throughout processing
- Source .baseval.ini files remain unchanged (macros not expanded in source)
