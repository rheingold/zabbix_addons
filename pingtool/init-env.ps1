# ==========================================================
# Pingtool Development Environment Initializer
# Zabbix Pingtool Plugin v2.0 (Agent2 Go Plugin) | December 2, 2025
# Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
# ==========================================================
# This script sets up the development environment for building the plugin.
# Adapted from aggplugin init-env.ps1

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

# -----------------------------
# Go setup
# -----------------------------
$goDir = $null

# Detect portable Go installation relative to project
$devBaseDir = Split-Path -Parent (Split-Path -Parent $projectDir)
$portableGoPath = Join-Path $devBaseDir "_ToolsGo\bin"

$goPossiblePaths = @(
    "C:\Go\bin",
    "C:\Program Files\Go\bin",
    "$env:LOCALAPPDATA\Programs\Go\bin",
    $portableGoPath
)

foreach ($path in $goPossiblePaths) {
    if (Test-Path "$path\go.exe") {
        $goDir = $path
        break
    }
}

if ($goDir) {
    $env:PATH = "$goDir;$env:PATH"

    # Set GOPATH - try portable workspace first, fall back to user profile
    $portableGoWorkspace = Join-Path $devBaseDir "_GoWorkspace"
    if (Test-Path $portableGoWorkspace) {
        $env:GOPATH = $portableGoWorkspace
    } elseif (-not $env:GOPATH) {
        $env:GOPATH = Join-Path $env:USERPROFILE "go"
    }
}

# -----------------------------
# MSYS2/MinGW setup (for CGO)
# -----------------------------
$msysPossiblePaths = @(
    "C:\msys64\mingw64\bin",
    "C:\msys32\mingw64\bin"
)

foreach ($path in $msysPossiblePaths) {
    if (Test-Path $path) {
        $env:PATH = "$path;$env:PATH"
        break
    }
}

# -----------------------------
# Zabbix SDK (for includes)
# -----------------------------
$zabbixLibPath = "C:\Users\plachy\Documents\Dev\Cpp\zabbixlib"
if (Test-Path $zabbixLibPath) {
    Write-Host "Zabbix SDK: $zabbixLibPath" -ForegroundColor Cyan
    $env:ZABBIX_SDK_PATH = $zabbixLibPath
} else {
    Write-Host "Zabbix SDK not found at $zabbixLibPath" -ForegroundColor Yellow
}

# -----------------------------
# Feedback
# -----------------------------
Write-Host "Pingtool development environment initialized." -ForegroundColor Green
Write-Host "Project directory: $projectDir" -ForegroundColor Cyan

try {
    $goVersion = go version 2>&1
    Write-Host "Go: $goVersion" -ForegroundColor Green
    if ($env:GOPATH) {
        Write-Host "GOPATH: $env:GOPATH" -ForegroundColor Cyan
    }
} catch {
    Write-Host "Go not available - please install from https://go.dev/dl/" -ForegroundColor Yellow
    Write-Host "Or manually add Go bin directory to PATH" -ForegroundColor Yellow
}

try {
    $gccVersion = gcc --version 2>&1 | Select-Object -First 1
    Write-Host "GCC: $gccVersion" -ForegroundColor Green
} catch {
    Write-Host "GCC not available - please install MSYS2 from https://www.msys2.org/" -ForegroundColor Yellow
    Write-Host "Or manually add MinGW bin directory to PATH" -ForegroundColor Yellow
}

# -----------------------------
# CGO Check
# -----------------------------
$env:CGO_ENABLED = "1"
Write-Host "CGO_ENABLED=1" -ForegroundColor Green

Write-Host "Ready for pingtool plugin development!" -ForegroundColor Green
