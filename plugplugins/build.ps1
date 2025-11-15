Write-Host "Building Zabbix Aggplugin Measurement Plugins..." -ForegroundColor Cyan

try {
    gcc --version | Out-Null
    Write-Host "GCC found" -ForegroundColor Green
} catch {
    Write-Host "ERROR: GCC not found. Install MSYS2/MinGW64." -ForegroundColor Red
    exit 1
}

$outDir = "build"
if (!(Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

Write-Host ""
Write-Host "Building cpu_load_plugin.dll..." -ForegroundColor Yellow
gcc -shared -o "$outDir\cpu_load_plugin.dll" cpu_load_plugin.c -Wall -O2 -static-libgcc
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to build cpu_load_plugin.dll" -ForegroundColor Red
    exit 1
}
Write-Host "Success: cpu_load_plugin.dll" -ForegroundColor Green

Write-Host "Building memory_usage_plugin.dll..." -ForegroundColor Yellow
gcc -shared -o "$outDir\memory_usage_plugin.dll" memory_usage_plugin.c -Wall -O2 -static-libgcc
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to build memory_usage_plugin.dll" -ForegroundColor Red
    exit 1
}
Write-Host "Success: memory_usage_plugin.dll" -ForegroundColor Green

Write-Host "Building disk_stats_plugin.dll..." -ForegroundColor Yellow
gcc -shared -o "$outDir\disk_stats_plugin.dll" disk_stats_plugin.c -lPdh -Wall -O2 -static-libgcc
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to build disk_stats_plugin.dll" -ForegroundColor Red
    exit 1
}
Write-Host "Success: disk_stats_plugin.dll" -ForegroundColor Green

Write-Host ""
Write-Host "Build complete!" -ForegroundColor Cyan
