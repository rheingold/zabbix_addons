$ErrorActionPreference = 'Stop'

# Build script for pingtool Agent2 plugin (Go + CGO + C library)
# Outputs: pingtool.exe (Go plugin executable)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Building Pingtool Agent2 Plugin" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# Source init-env.ps1 to setup environment
$initEnv = Join-Path $PSScriptRoot "init-env.ps1"
if (Test-Path $initEnv) {
    Write-Host "Loading environment from init-env.ps1..." -ForegroundColor Yellow
    . $initEnv
} else {
    Write-Host "Warning: init-env.ps1 not found, using system PATH" -ForegroundColor Yellow
}

# Ensure CGO is enabled
$env:CGO_ENABLED = "1"

# Check Go availability
try {
    $goVersion = go version
    Write-Host "Using: $goVersion" -ForegroundColor Green
} catch {
    Write-Host "ERROR: Go not found in PATH" -ForegroundColor Red
    Write-Host "Please install Go 1.21+ or run init-env.ps1" -ForegroundColor Red
    exit 1
}

# Check GCC availability (for CGO)
try {
    $gccVersion = gcc --version | Select-Object -First 1
    Write-Host "Using: $gccVersion" -ForegroundColor Green
} catch {
    Write-Host "ERROR: GCC not found in PATH" -ForegroundColor Red
    Write-Host "Please install MinGW-w64 or run init-env.ps1" -ForegroundColor Red
    exit 1
}

# Create build directory
$buildDir = Join-Path $PSScriptRoot "build"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Build Go plugin
Write-Host "`nBuilding Go plugin with CGO..." -ForegroundColor Cyan
Write-Host "CGO will compile pingtool_lib.c and link with Go code" -ForegroundColor Yellow

$outExe = Join-Path $buildDir "pingtool.exe"

# Run go build
Push-Location $PSScriptRoot
try {
    go build -v -o "$outExe" .
    
    if ($LASTEXITCODE -ne 0) {
        throw "go build failed with exit code $LASTEXITCODE"
    }
    
    if (!(Test-Path $outExe)) {
        throw "Build succeeded but output file not found: $outExe"
    }
    
    $fileSize = (Get-Item $outExe).Length
    $fileSizeKB = [math]::Round($fileSize / 1KB, 2)
    
    Write-Host "`n==================================================" -ForegroundColor Green
    Write-Host "BUILD SUCCESSFUL" -ForegroundColor Green
    Write-Host "==================================================" -ForegroundColor Green
    Write-Host "Output: $outExe" -ForegroundColor Cyan
    Write-Host "Size: $fileSizeKB KB" -ForegroundColor Cyan
    
} finally {
    Pop-Location
}

Write-Host "`nNext steps:" -ForegroundColor Yellow
Write-Host "1. Copy pingtool.exe to C:\Zabbix\plugins\" -ForegroundColor White
Write-Host "2. Create config: C:\Zabbix\conf\zabbix_agent2.d\plugins.d\pingtool.conf" -ForegroundColor White
Write-Host "3. Add: Plugins.Pingtool.System.Path=C:\Zabbix\plugins\pingtool.exe" -ForegroundColor White
Write-Host "4. Restart Zabbix Agent 2 service" -ForegroundColor White
Write-Host "5. Test with: zabbix_get -s localhost -k 'ping.icmp[127.0.0.1,3,1000]'" -ForegroundColor White
