# Test script for internal metrics architecture
Write-Host "Testing Internal Metrics Architecture" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Test 1: Check if plugin executable exists
Write-Host "[1] Checking plugin executable..." -ForegroundColor Yellow
if (Test-Path "build\aggplugin-agent2.exe") {
    Write-Host "    ✓ Plugin executable found" -ForegroundColor Green
} else {
    Write-Host "    ✗ Plugin executable NOT found" -ForegroundColor Red
    exit 1
}

# Test 2: Check debug log for internal metric registration
Write-Host "[2] Checking for internal metric registration in log..." -ForegroundColor Yellow
if (Test-Path "C:\Zabbix\log\aggplugin_debug.log") {
    $logLines = Get-Content "C:\Zabbix\log\aggplugin_debug.log" | Select-String "internal" | Select-Object -Last 10
    if ($logLines) {
        Write-Host "    ✓ Found internal metric references:" -ForegroundColor Green
        $logLines | ForEach-Object { Write-Host "      $_" -ForegroundColor Gray }
    } else {
        Write-Host "    ⚠ No internal metric references found (plugin may not have started yet)" -ForegroundColor Yellow
    }
} else {
    Write-Host "    ⚠ Log file not found (plugin not started yet)" -ForegroundColor Yellow
}

# Test 3: Show registered metrics from latest log
Write-Host "[3] Checking registered metrics..." -ForegroundColor Yellow
if (Test-Path "C:\Zabbix\log\aggplugin_debug.log") {
    $registered = Get-Content "C:\Zabbix\log\aggplugin_debug.log" | Select-String "Registered" | Select-Object -Last 10
    if ($registered) {
        Write-Host "    ✓ Found registration entries:" -ForegroundColor Green
        $registered | ForEach-Object { Write-Host "      $_" -ForegroundColor Gray }
    }
}

Write-Host ""
Write-Host "Test completed. To fully test, start Zabbix Agent2 service and query metrics:" -ForegroundColor Cyan
Write-Host "  zabbix_get -s 127.0.0.1 -p 10050 -k aggplugin._internal.test" -ForegroundColor White
Write-Host "  zabbix_get -s 127.0.0.1 -p 10050 -k aggplugin._internal.cpu_load" -ForegroundColor White
Write-Host "  zabbix_get -s 127.0.0.1 -p 10050 -k aggplugin.cpu_load" -ForegroundColor White
