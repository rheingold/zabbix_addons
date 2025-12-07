$ErrorActionPreference = 'Stop'

# Build script for pingtool (MinGW-w64 gcc required in PATH)
# Outputs: ..\build\pingtool.exe

$root = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $root 'build'
if (!(Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

$src = Join-Path $PSScriptRoot 'pingtool.c'
$out = Join-Path $outDir 'pingtool.exe'

Write-Host "Compiling pingtool..." -ForegroundColor Cyan

$gcc = 'gcc'
$args = @('-O2','-s','-Wall',"`"$src`"",'-o',"`"$out`"",'-lIphlpapi','-lws2_32')

& $gcc @args

if ($LASTEXITCODE -ne 0) { throw "gcc failed with exit code $LASTEXITCODE" }

Write-Host "Built $out" -ForegroundColor Green
