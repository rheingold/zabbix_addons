# ==========================================================
# Aggplugin Development Environment Initializer
# Zabbix Aggplugin v0.1 (tmp0.1) | November 3, 2025
# Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
# ==========================================================
# This script sets up the development environment for building the plugin.
# Customize the paths below to match your system configuration.

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

# -----------------------------
# Go setup
# -----------------------------
# Go is typically installed in one of these locations:
# - C:\Go (default installer location)
# - C:\Program Files\Go
# - Custom location if you use a portable installation

# Try to find Go automatically
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
# MSYS2/MinGW setup
# -----------------------------
# MSYS2 is typically installed at C:\msys64
# If installed elsewhere, modify this path

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
# Feedback
# -----------------------------
Write-Host "Aggplugin development environment initialized." -ForegroundColor Green
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
# VS Code Shell Integration (Portable)
# -----------------------------
if ($env:TERM_PROGRAM -eq "vscode") {
    # Try to find VS Code in portable location relative to project
    $portableVscodeDir = Join-Path $devBaseDir "_ToolsVSC"
    if (Test-Path $portableVscodeDir) {
        $shellIntegrationPath = Join-Path $portableVscodeDir "resources\app\out\vs\workbench\contrib\terminal\common\scripts\shellIntegration.ps1"
        if (Test-Path $shellIntegrationPath) {
            try {
                Unblock-File $shellIntegrationPath -ErrorAction SilentlyContinue
                . $shellIntegrationPath
                Write-Host "VS Code shell integration enabled" -ForegroundColor Green
            } catch {
                Write-Host "VS Code shell integration failed to load" -ForegroundColor Yellow
            }
        }
    }
}

# -----------------------------
# Git Integration (posh-git)
# -----------------------------
try {
    Import-Module posh-git -ErrorAction SilentlyContinue
    Write-Host "Git integration enabled (posh-git)" -ForegroundColor Green
} catch {
    Write-Host "posh-git module not available" -ForegroundColor Yellow
}

Write-Host "Ready for aggplugin development!" -ForegroundColor Green