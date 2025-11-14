# restart_zabbix_service.ps1 - Restart Zabbix Agent2 Service
# Requires Administrator privileges
#
# Usage: Right-click → "Run with PowerShell as Administrator"
#        OR: From admin PowerShell: .\restart_zabbix_service.ps1

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script requires Administrator privileges!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please run PowerShell as Administrator and try again." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Or right-click this script and select 'Run with PowerShell as Administrator'" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Restarting Zabbix Agent 2" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Test configuration first
Write-Host "Testing configuration..." -ForegroundColor Yellow
$testResult = & C:\Zabbix\bin\zabbix_agent2.exe -c C:\Zabbix\conf\zabbix_agent2.conf -T 2>&1
Write-Host $testResult

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Configuration validation failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "Configuration valid. Restarting service..." -ForegroundColor Yellow

# Restart the service
try {
    Restart-Service "Zabbix Agent 2" -Force -ErrorAction Stop
    Write-Host ""
    Write-Host "SUCCESS: Zabbix Agent 2 restarted successfully!" -ForegroundColor Green
    
    # Wait a moment for service to start
    Start-Sleep -Seconds 2
    
    # Check status
    $service = Get-Service "Zabbix Agent 2"
    Write-Host ""
    Write-Host "Service Status:" -ForegroundColor Cyan
    Write-Host "  Name:       $($service.Name)" -ForegroundColor Gray
    Write-Host "  Status:     $($service.Status)" -ForegroundColor $(if ($service.Status -eq 'Running') { 'Green' } else { 'Red' })
    Write-Host "  Start Type: $($service.StartType)" -ForegroundColor Gray
    
} catch {
    Write-Host ""
    Write-Host "ERROR: Failed to restart service!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "Plugin DLLs deployed:" -ForegroundColor Cyan
Get-ChildItem C:\Zabbix\plugins\measurements\*.dll | ForEach-Object {
    Write-Host "  - $($_.Name) ($([math]::Round($_.Length/1KB, 0)) KB)" -ForegroundColor Gray
}

Write-Host ""
Write-Host "You can now test the metrics with zabbix_get:" -ForegroundColor Cyan
Write-Host "  zabbix_get -s 127.0.0.1 -k `"aggplugin.cpu_load[avg,60]`"" -ForegroundColor White
Write-Host "  zabbix_get -s 127.0.0.1 -k `"aggplugin.mem_free[avg,60]`"" -ForegroundColor White
Write-Host ""

Read-Host "Press Enter to exit"
