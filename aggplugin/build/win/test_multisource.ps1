# Test Multi-Source Metric Support
# Tests that disk.io.read returns per-disk statistics

Write-Host "=== Testing Multi-Source Metric Support ===" -ForegroundColor Cyan
Write-Host ""

# Start the plugin
$proc = Start-Process -FilePath ".\aggplugin-agent2.exe" -ArgumentList "-test" -NoNewWindow -PassThru -RedirectStandardOutput "test_output.txt" -RedirectStandardError "test_error.txt"

Write-Host "Waiting 3 seconds for plugin initialization..." -ForegroundColor Yellow
Start-Sleep -Seconds 3

# Test metrics via zabbix_get (if available) or check logs
if (Test-Path "test_output.txt") {
    Write-Host "`nPlugin Output:" -ForegroundColor Green
    Get-Content "test_output.txt" -ErrorAction SilentlyContinue | Select-Object -Last 20
}

if (Test-Path "test_error.txt") {
    Write-Host "`nPlugin Errors:" -ForegroundColor Red
    Get-Content "test_error.txt" -ErrorAction SilentlyContinue
}

# Check debug log
$logPath = "aggplugin_debug.log"
if (Test-Path $logPath) {
    Write-Host "`nDebug Log (last 30 lines):" -ForegroundColor Green
    Get-Content $logPath -Tail 30
}

# Cleanup
Write-Host "`nStopping plugin..." -ForegroundColor Yellow
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

Write-Host "`nTest complete. Check output above for multi-source JSON format." -ForegroundColor Cyan
Write-Host "Expected format: {`"values`":{`"all`":{...},`"sources`":{`"0`":{...},`"1`":{...}}}}" -ForegroundColor Gray
