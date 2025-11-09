# Zabbix Module Helper Functions
# Add this to your PowerShell profile or source it when needed

function Rescan-ZabbixModules {
    <#
    .SYNOPSIS
        Trigger Zabbix module/widget manifest refresh
    .DESCRIPTION
        Touches module directories and lists installed modules/widgets.
        Still requires manual "Scan directory" click in Administration → Modules
    #>
    
    [System.Net.ServicePointManager]::ServerCertificateValidationCallback = {$true}
    
    $result = Invoke-RestMethod -Uri 'http://192.168.254.16:8080/modules_rescan.php?token=d52aecac75ee7af2dd2a31b1725b423ce68027db0567303f859a78b750cf0d83'
    
    Write-Host "✓ Directories touched" -ForegroundColor Green
    Write-Host "Modules found: $($result.modules_found)" -ForegroundColor Cyan
    Write-Host "Widgets found: $($result.widgets_found)" -ForegroundColor Cyan
    
    if ($result.modules.Count -gt 0) {
        Write-Host "`nModules:" -ForegroundColor Yellow
        $result.modules | ForEach-Object {
            Write-Host "  • $($_.name) ($($_.id}) v$($_.version)"
        }
    }
    
    if ($result.widgets.Count -gt 0 -and $result.widgets.Count -le 10) {
        Write-Host "`nWidgets:" -ForegroundColor Yellow
        $result.widgets | ForEach-Object {
            $size = if ($_.size) { " [$($_.size.width)x$($_.size.height)]" } else { "" }
            Write-Host "  • $($_.name) ($($_.id})$size"
        }
    }
    
    Write-Host "`n⚠ Please click 'Scan directory' in Administration → Modules to complete registration" -ForegroundColor Yellow
    
    return $result
}

# Create alias for convenience
Set-Alias -Name zabbix-rescan -Value Rescan-ZabbixModules

Write-Host "Zabbix helper functions loaded. Use: Rescan-ZabbixModules or zabbix-rescan" -ForegroundColor Green
