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

# Ensure build directory exists
$buildDir = Join-Path $PSScriptRoot "build"
if (-not (Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

Push-Location -Path (Join-Path $PSScriptRoot "cpp_agent_wrapper")
try {
    $commonPath = Join-Path $PSScriptRoot "cpp_common"
    
    # Detect zabbixlib location (portable: ../../../zabbixlib or ../../zabbixlib)
    $zabbixInclude = ""
    if (Test-Path "$PWD/../../../zabbixlib/include") {
        $zabbixInclude = "-I`"$PWD/../../../zabbixlib/include`""
        Write-Host "Using zabbixlib from: $PWD/../../../zabbixlib/include"
    } elseif (Test-Path "$PWD/../../../../zabbixlib/include") {
        $zabbixInclude = "-I`"$PWD/../../../../zabbixlib/include`""
        Write-Host "Using zabbixlib from: $PWD/../../../../zabbixlib/include"
    } else {
        Write-Warning "zabbixlib headers not found (optional for Go external plugin)"
    }
    
    if ($Variant -eq 'classic' -or $Variant -eq 'both') {
        Write-Host "Building classic agent DLL..."
        g++ -shared -o "../build/aggplugin_classic.dll" unified_wrapper.cpp "$commonPath/plugin_common.cpp" "$commonPath/plugin_loader.cpp" "$commonPath/collector.cpp" $zabbixInclude -I"$commonPath" -DZABBIX_CLASSIC_AGENT -static-libgcc -static-libstdc++ -lpdh
        if ($LASTEXITCODE -ne 0) { throw "classic build failed" }
        Write-Host "Created build/aggplugin_classic.dll"
    }

    if ($Variant -eq 'agent2' -or $Variant -eq 'both') {
        Write-Host "Building agent2 plugin DLL (requires agent2 headers/libs)..."
        g++ -shared -o "../build/aggplugin_agent2.dll" unified_wrapper.cpp "$commonPath/plugin_common.cpp" "$commonPath/collector.cpp" "$commonPath/plugin_loader.cpp" $zabbixInclude -I"$commonPath" -DZABBIX_AGENT2 -static-libgcc -static-libstdc++ -lpdh
        if ($LASTEXITCODE -ne 0) { Write-Warning "agent2 build failed (likely missing SDK headers/libs)." }
        else { Write-Host "Created build/aggplugin_agent2.dll" }
    }

    # Build collector shared library for CGO
    Write-Host "Building collector shared library for Go CGO..."
    g++ -shared -o "../build/libaggcollector.dll" "$commonPath/collector_shared.cpp" "$commonPath/collector.cpp" "$commonPath/plugin_common.cpp" "$commonPath/plugin_loader.cpp" -I"$commonPath" -static-libgcc -static-libstdc++ -lpdh
    if ($LASTEXITCODE -ne 0) { Write-Warning "collector shared library build failed." }
    else { Write-Host "Created build/libaggcollector.dll" }
}
finally { Pop-Location }

# Build Go Agent2 wrapper
if ($Variant -eq 'agent2' -or $Variant -eq 'both') {
    Write-Host "`nBuilding Go Agent2 wrapper executable..."
    Push-Location -Path (Join-Path $PSScriptRoot "go_agent2_wrapper")
    try {
        go build -o "..\build\aggplugin-agent2.exe"
        if ($LASTEXITCODE -ne 0) { Write-Warning "Go Agent2 wrapper build failed." }
        else { Write-Host "Created build/aggplugin-agent2.exe" }
    }
    finally { Pop-Location }
}

# Deployment section

if ($Deploy -and (Test-Path "C:\Zabbix")) {
    Write-Host "`nDeploying to Zabbix standard directories..."
    
    # Classic Agent DLL deployment
    if (Test-Path "$buildDir\aggplugin_classic.dll") {
        Copy-Item "$buildDir\aggplugin_classic.dll" "C:\Zabbix\modules\" -Force
        Write-Host "Deployed: C:\Zabbix\modules\aggplugin_classic.dll"
    }
    
    # Agent2 Plugin deployment  
    if (Test-Path "$buildDir\aggplugin-agent2.exe") {
        Copy-Item "$buildDir\aggplugin-agent2.exe" "C:\Zabbix\plugins\" -Force
        Copy-Item "$buildDir\libaggcollector.dll" "C:\Zabbix\plugins\" -Force
        Write-Host "Deployed: C:\Zabbix\plugins\aggplugin-agent2.exe"
        Write-Host "Deployed: C:\Zabbix\plugins\libaggcollector.dll"
    }
}
