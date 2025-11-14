# build_example_plugin.ps1 - Build Example Measurement Plugins
# Zabbix Aggplugin v0.1 (tmp0.1) | November 14, 2025
# Author: Claude Sonnet 4.5 (AI Assistant) | Lead & Architecture: lukas@plachy.eu
#
# PURPOSE:
#   Compiles example measurement plugins from example_plugins/ directory
#   Creates DLL files ready for deployment to Zabbix plugin directory
#
# USAGE:
#   .\build_example_plugin.ps1 [-PluginName disk_stats_plugin] [-OutputDir build\plugins]
#
# REQUIREMENTS:
#   - MinGW-w64 GCC in PATH (or sourced via init-env.ps1)
#   - Windows PDH library (included in Windows SDK)
#
# OUTPUT:
#   - build\plugins\<plugin_name>.dll

param(
    [string]$PluginName = "all",  # Changed default to "all"
    [string]$OutputDir = "build\plugins"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Aggplugin Example Plugin Builder" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if GCC is available
$gccPath = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gccPath) {
    Write-Host "ERROR: gcc not found in PATH" -ForegroundColor Red
    Write-Host "       Run init-env.ps1 first to set up MinGW environment" -ForegroundColor Yellow
    exit 1
}

Write-Host "GCC Compiler:  $($gccPath.Source)" -ForegroundColor Green
$gccVersion = & gcc --version | Select-Object -First 1
Write-Host "GCC Version:   $gccVersion" -ForegroundColor Green
Write-Host ""

# Ensure output directory exists
if (-not (Test-Path $OutputDir)) {
    Write-Host "Creating output directory: $OutputDir" -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

# Get list of plugins to build
$pluginsToBuild = @()
if ($PluginName -eq "all") {
    Write-Host "Building all example plugins..." -ForegroundColor Cyan
    Get-ChildItem example_plugins\*.c | ForEach-Object {
        $pluginsToBuild += $_.BaseName
    }
} else {
    $pluginsToBuild = @($PluginName)
}

Write-Host "Plugins to build: $($pluginsToBuild -join ', ')" -ForegroundColor Cyan
Write-Host ""

# Build each plugin
$successCount = 0
$failCount = 0

foreach ($plugin in $pluginsToBuild) {
    $sourceFile = "example_plugins\${plugin}.c"
    $outputFile = "${OutputDir}\${plugin}.dll"

    if (-not (Test-Path $sourceFile)) {
        Write-Host "ERROR: Source file not found: $sourceFile" -ForegroundColor Red
        $failCount++
        continue
    }

    Write-Host "Building Plugin: $plugin" -ForegroundColor Cyan
    Write-Host "  Source:  $sourceFile" -ForegroundColor Gray
    Write-Host "  Output:  $outputFile" -ForegroundColor Gray

    # Compiler flags
    $compilerFlags = @(
        "-shared",              # Build DLL
        "-o", $outputFile,      # Output file
        $sourceFile,            # Source file
        "-lPdh",                # Link PDH library (Performance Data Helper)
        "-Wall",                # Enable all warnings
        "-O2",                  # Optimize for speed
        "-std=c11"              # C11 standard
    )

    Write-Host "  Compiling..." -ForegroundColor Yellow

    try {
        & gcc @compilerFlags 2>&1 | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkGray }
        
        if ($LASTEXITCODE -ne 0) {
            throw "Compilation failed with exit code $LASTEXITCODE"
        }
        
        Write-Host "  SUCCESS!" -ForegroundColor Green
        
        # Show output file info
        $outputInfo = Get-Item $outputFile
        Write-Host "    Size: $([math]::Round($outputInfo.Length / 1KB, 2)) KB" -ForegroundColor Gray
        Write-Host ""
        
        $successCount++
        
    } catch {
        Write-Host "  ERROR: Build failed!" -ForegroundColor Red
        Write-Host "  $($_.Exception.Message)" -ForegroundColor Red
        Write-Host ""
        $failCount++
    }
}

# Summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Build Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Total:    $($pluginsToBuild.Count) plugin(s)" -ForegroundColor Gray
Write-Host "  Success:  $successCount" -ForegroundColor Green
Write-Host "  Failed:   $failCount" -ForegroundColor $(if ($failCount -gt 0) { "Red" } else { "Gray" })
Write-Host ""

if ($successCount -eq 0) {
    Write-Host "No plugins were built successfully." -ForegroundColor Red
    exit 1
}
if ($successCount -eq 0) {
    Write-Host "No plugins were built successfully." -ForegroundColor Red
    exit 1
}

# Deployment instructions
Write-Host "Deployment Instructions:" -ForegroundColor Cyan
Write-Host "  1. Copy plugins to Zabbix directory:" -ForegroundColor Gray
Write-Host "     Copy-Item '$OutputDir\*.dll' C:\Zabbix\plugins\measurements\" -ForegroundColor White
Write-Host ""
Write-Host "  2. Add to aggplugin.conf:" -ForegroundColor Gray
Write-Host "     [Plugins]" -ForegroundColor White
Write-Host "     PluginPath=C:\Zabbix\plugins\measurements\*.dll" -ForegroundColor White
Write-Host "" 
Write-Host "     # Built-in metric plugins" -ForegroundColor White
Write-Host "     [plugin.cpu_load_plugin]" -ForegroundColor White
Write-Host "     enabled=1" -ForegroundColor White
Write-Host ""
Write-Host "     [plugin.memory_usage_plugin]" -ForegroundColor White
Write-Host "     enabled=1" -ForegroundColor White
Write-Host ""
Write-Host "     # Example disk plugin" -ForegroundColor White
Write-Host "     [plugin.disk_stats_plugin]" -ForegroundColor White
Write-Host "     enabled=1" -ForegroundColor White
Write-Host ""
Write-Host "  3. Restart Zabbix Agent2" -ForegroundColor Gray
Write-Host ""
Write-Host "  4. Test with zabbix_get:" -ForegroundColor Gray
Write-Host "     zabbix_get -s 127.0.0.1 -k `"aggplugin.cpu_load[avg,60]`"" -ForegroundColor White
Write-Host "     zabbix_get -s 127.0.0.1 -k `"aggplugin.mem_free[avg,60]`"" -ForegroundColor White
Write-Host "     zabbix_get -s 127.0.0.1 -k `"aggplugin.disk.io.read[0,avg,60]`"" -ForegroundColor White
Write-Host ""

Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host ""
