# update_and_restart.ps1 - Update plugin and restart service
# Requires Administrator privileges

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "ERROR: This script requires Administrator privileges" -ForegroundColor Red
    Write-Host "Right-click PowerShell and 'Run as Administrator', then run this script again" -ForegroundColor Yellow
    exit 1
}

Write-Host "Stopping Zabbix Agent 2 service..." -ForegroundColor Cyan
Stop-Service "Zabbix Agent 2" -ErrorAction SilentlyContinue

Write-Host "Waiting for service to stop..." -ForegroundColor Cyan
Start-Sleep -Seconds 2

Write-Host "Deploying updated files..." -ForegroundColor Cyan
Copy-Item build\aggplugin-agent2.exe C:\Zabbix\plugins\ -Force
Copy-Item build\libaggcollector.dll C:\Zabbix\plugins\ -Force

Write-Host "Starting Zabbix Agent 2 service..." -ForegroundColor Cyan
Start-Service "Zabbix Agent 2"

Write-Host "Waiting for service to start..." -ForegroundColor Cyan
Start-Sleep -Seconds 2

$status = Get-Service "Zabbix Agent 2" | Select-Object -ExpandProperty Status
Write-Host "Service status: $status" -ForegroundColor $(if ($status -eq "Running") {"Green"} else {"Red"})

if ($status -eq "Running") {
    Write-Host "`nDeployment successful!" -ForegroundColor Green
} else {
    Write-Host "`nWARNING: Service not running. Check C:\Zabbix\log\zabbix_agent2.log for errors" -ForegroundColor Yellow
}
