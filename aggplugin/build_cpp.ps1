<#
.SYNOPSIS
    Build script for Zabbix Aggplugin (Windows with MSYS2/mingw64)

.DESCRIPTION
    Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
    Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu

.USAGE
    .\build_cpp.ps1 -Variant classic
    .\build_cpp.ps1 -Variant agent2
#>

param(
    [ValidateSet("classic","agent2","both")]
    [string]$Variant = "classic",
    
    [switch]$Deploy
)

Write-Host "Building C++ plugin variant(s): $Variant"

# Try to find and add MSYS2 mingw64 to PATH if not already available
$gccFound = $false
try {
    gcc --version | Out-Null
    $gccFound = $true
    Write-Host "GCC found in PATH" -ForegroundColor Green
} catch {
    # Try common MSYS2 installation locations
    $msysPaths = @("C:\msys64\mingw64\bin", "C:\msys32\mingw64\bin")
    foreach ($msysPath in $msysPaths) {
        if (Test-Path $msysPath) {
            $env:PATH = "$msysPath;" + $env:PATH
            Write-Host "Added MSYS2 MinGW to PATH: $msysPath" -ForegroundColor Cyan
            $gccFound = $true
            break
        }
    }
}

if (-not $gccFound) {
    Write-Host "ERROR: GCC not found. Please install MSYS2 or add MinGW to PATH." -ForegroundColor Red
    Write-Host "Download from: https://www.msys2.org/" -ForegroundColor Yellow
    exit 1
}

# Ensure build directories exist
$buildDir = Join-Path $PSScriptRoot "build"
$buildWinDir = Join-Path $buildDir "win"
$buildLinDir = Join-Path $buildDir "lin"
if (-not (Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }
if (-not (Test-Path $buildWinDir)) { New-Item -ItemType Directory -Path $buildWinDir | Out-Null }
if (-not (Test-Path $buildLinDir)) { New-Item -ItemType Directory -Path $buildLinDir | Out-Null }

Push-Location -Path (Join-Path $PSScriptRoot "cpp_agent_wrapper")
try {
    $commonPath = Join-Path $PSScriptRoot "cpp_common"
    
    # Detect zabbixlib location (portable: Cpp/zabbixlib preferred)
    # From cpp_agent_wrapper: ../../../zabbixlib (aggplugin → zabbix → Cpp)
    $zabbixInclude = ""
    if (Test-Path "$PWD/../../../zabbixlib/include") {
        # Preferred location: Cpp/zabbixlib (neighbor to Cpp/zabbix)
        $zabbixInclude = "-I`"$PWD/../../../zabbixlib/include`""
        Write-Host "Using zabbixlib from: Cpp/zabbixlib/include"
    } elseif (Test-Path "$PWD/../../zabbixlib/include") {
        # Alternative: inside aggplugin directory
        $zabbixInclude = "-I`"$PWD/../../zabbixlib/include`""
        Write-Host "Using zabbixlib from: aggplugin/zabbixlib/include"
    } else {
        Write-Warning "zabbixlib headers not found (optional for Go external plugin)"
    }
    
    if ($Variant -eq 'classic' -or $Variant -eq 'both') {
        Write-Host "Building classic agent DLL..."
        g++ -shared -o "../build/win/aggplugin_classic.dll" unified_wrapper.cpp "$commonPath/plugin_common.cpp" "$commonPath/plugin_loader.cpp" "$commonPath/collector.cpp" $zabbixInclude -I"$commonPath" -DZABBIX_CLASSIC_AGENT -static-libgcc -static-libstdc++ -lpdh
        if ($LASTEXITCODE -ne 0) { throw "classic build failed" }
        Write-Host "Created build/win/aggplugin_classic.dll"
    }

    if ($Variant -eq 'agent2' -or $Variant -eq 'both') {
        Write-Host "Building agent2 plugin DLL (requires agent2 headers/libs)..."
        g++ -shared -o "../build/win/aggplugin_agent2.dll" unified_wrapper.cpp "$commonPath/plugin_common.cpp" "$commonPath/collector.cpp" "$commonPath/plugin_loader.cpp" $zabbixInclude -I"$commonPath" -DZABBIX_AGENT2 -static-libgcc -static-libstdc++ -lpdh
        if ($LASTEXITCODE -ne 0) { Write-Warning "agent2 build failed (likely missing SDK headers/libs)." }
        else { Write-Host "Created build/win/aggplugin_agent2.dll" }
    }

    # Build collector shared library for CGO
    Write-Host "Building collector shared library for Go CGO..."
    g++ -shared -o "../build/win/libaggcollector.dll" "$commonPath/collector_shared.cpp" "$commonPath/collector.cpp" "$commonPath/plugin_common.cpp" "$commonPath/plugin_loader.cpp" -I"$commonPath" -static-libgcc -static-libstdc++ -lpdh
    if ($LASTEXITCODE -ne 0) { Write-Warning "collector shared library build failed." }
    else { 
        Write-Host "Created build/win/libaggcollector.dll" 
        
        # Copy MinGW pthread DLL dependency
        $pthreadDll = "C:\msys64\mingw64\bin\libwinpthread-1.dll"
        if (Test-Path $pthreadDll) {
            Copy-Item $pthreadDll "../build/win/" -Force
            Write-Host "Copied build/win/libwinpthread-1.dll (required runtime dependency)"
        }
    }
}
finally { Pop-Location }

# Build Go Agent2 wrapper
if ($Variant -eq 'agent2' -or $Variant -eq 'both') {
    Write-Host "`nBuilding Go Agent2 wrapper executable..."
    Push-Location -Path (Join-Path $PSScriptRoot "go_agent2_wrapper")
    try {
        go build -o "..\build\win\aggplugin-agent2.exe"
        if ($LASTEXITCODE -ne 0) { Write-Warning "Go Agent2 wrapper build failed." }
        else { Write-Host "Created build/win/aggplugin-agent2.exe" }
    }
    finally { Pop-Location }
}

# Build example measurement plugin DLLs (if example_plugins directory exists)
if (Test-Path "example_plugins") {
    Write-Host "`nBuilding example measurement plugins..."
    $examplePlugins = @("cpu_load_plugin", "memory_usage_plugin", "disk_stats_plugin")
    
    foreach ($plugin in $examplePlugins) {
        $sourceFile = "example_plugins\$plugin.c"
        $outputFile = "$buildWinDir\$plugin.dll"
        
        if (Test-Path $sourceFile) {
            Write-Host "Building $plugin.dll..."
            g++ -shared -o $outputFile $sourceFile `
                -I"cpp_common" `
                -I"..\zabbixlib\include" `
                -lpdh `
                -static-libgcc `
                -static-libstdc++
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "Created $outputFile" -ForegroundColor Green
            } else {
                Write-Host "Failed to build $plugin.dll" -ForegroundColor Red
            }
        }
    }
}

# Deployment section

if ($Deploy -and (Test-Path "C:\Zabbix")) {
    Write-Host "`nDeploying to Zabbix standard directories..."
    
    # Classic Agent DLL deployment
    if (Test-Path "$buildWinDir\aggplugin_classic.dll") {
        Copy-Item "$buildWinDir\aggplugin_classic.dll" "C:\Zabbix\modules\" -Force
        Write-Host "Deployed: C:\Zabbix\modules\aggplugin_classic.dll"
    }
    
    # Agent2 Plugin deployment  
    if (Test-Path "$buildWinDir\aggplugin-agent2.exe") {
        Copy-Item "$buildWinDir\aggplugin-agent2.exe" "C:\Zabbix\bin\" -Force
        Copy-Item "$buildWinDir\libaggcollector.dll" "C:\Zabbix\bin\" -Force
        Copy-Item "$buildWinDir\libwinpthread-1.dll" "C:\Zabbix\bin\" -Force -ErrorAction SilentlyContinue
        
        Write-Host "Deployed: C:\Zabbix\bin\aggplugin-agent2.exe"
        Write-Host "Deployed: C:\Zabbix\bin\libaggcollector.dll"
        Write-Host "Deployed: C:\Zabbix\bin\libwinpthread-1.dll"
        
        # Deploy measurement plugin DLLs
        $pluginDlls = Get-ChildItem "$buildWinDir\*_plugin.dll" -ErrorAction SilentlyContinue
        if ($pluginDlls) {
            New-Item -ItemType Directory -Path "C:\Zabbix\bin\plugins" -Force | Out-Null
            foreach ($dll in $pluginDlls) {
                Copy-Item $dll.FullName "C:\Zabbix\bin\plugins\" -Force
                Write-Host "Deployed: C:\Zabbix\bin\plugins\$($dll.Name)"
            }
        }
        
        # Deploy config if it doesn't exist
        $configDest = "C:\Zabbix\conf\zabbix_agent2.d\plugins.d\aggplugin.conf"
        if (-not (Test-Path $configDest)) {
            New-Item -ItemType Directory -Path "C:\Zabbix\conf\zabbix_agent2.d\plugins.d" -Force | Out-Null
            Copy-Item "$buildWinDir\aggplugin.conf" $configDest -Force
            Write-Host "Deployed: $configDest"
        } else {
            Write-Host "Config exists: $configDest (not overwritten)"
        }
    }
}
