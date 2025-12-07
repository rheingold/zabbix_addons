# Macro List Editor - Deployment Script
# Purpose: Deploy module to Zabbix Docker container on TrueNAS

param(
    [switch]$DryRun = $false
)

$ErrorActionPreference = "Stop"

# Configuration
$SSH_KEY = "ai_priv\id_aibot"
$SSH_USER = "aibot"
$SSH_HOST = "192.168.254.16"
$CONTAINER = "ix-zabbix-web-zabbix-web-1"
$TEMP_DIR = "/tmp/listedit_deploy"
$TARGET_DIR = "/usr/share/zabbix/modules/listedit"

Write-Host "=== Macro List Editor Deployment ===" -ForegroundColor Green
Write-Host ""

# Check if source files exist
if (-not (Test-Path "src")) {
    Write-Host "ERROR: src directory not found!" -ForegroundColor Red
    exit 1
}

Write-Host "Step 1: Preparing source files..." -ForegroundColor Cyan
$fileCount = (Get-ChildItem -Path "src" -Recurse -File).Count
Write-Host "  Found $fileCount files to deploy" -ForegroundColor Gray

if ($DryRun) {
    Write-Host "  [DRY RUN] Would copy files to TrueNAS" -ForegroundColor Yellow
} else {
    Write-Host "Step 2: Copying files to TrueNAS host..." -ForegroundColor Cyan
    
    # Create temp directory on host
    ssh -i $SSH_KEY "$SSH_USER@$SSH_HOST" "mkdir -p $TEMP_DIR; rm -rf $TEMP_DIR/*"
    
    # Copy files
    scp -i $SSH_KEY -r src/* "$SSH_USER@${SSH_HOST}:$TEMP_DIR/"
    
    Write-Host "  Files copied to ${SSH_HOST}:${TEMP_DIR}" -ForegroundColor Gray
}

if ($DryRun) {
    Write-Host "  [DRY RUN] Would copy files into Docker container" -ForegroundColor Yellow
    Write-Host "  [DRY RUN] Would set permissions" -ForegroundColor Yellow
} else {
    Write-Host "Step 3: Deploying to Docker container..." -ForegroundColor Cyan
    
    # Create directory in container
    ssh -i $SSH_KEY "$SSH_USER@$SSH_HOST" "sudo docker exec $CONTAINER mkdir -p $TARGET_DIR"
    
    # Copy files into container
    ssh -i $SSH_KEY "$SSH_USER@$SSH_HOST" "sudo docker cp $TEMP_DIR/. ${CONTAINER}:$TARGET_DIR/"
    
    Write-Host "  Files deployed to container:$TARGET_DIR" -ForegroundColor Gray
    
    Write-Host "Step 4: Setting permissions..." -ForegroundColor Cyan
    
    # Set ownership and permissions (using root user in container)
    ssh -i $SSH_KEY "$SSH_USER@$SSH_HOST" "sudo docker exec -u root $CONTAINER chown -R zabbix:zabbix $TARGET_DIR"
    ssh -i $SSH_KEY "$SSH_USER@$SSH_HOST" "sudo docker exec -u root $CONTAINER chmod -R 755 $TARGET_DIR"
    
    Write-Host "  Permissions set (755, zabbix:zabbix)" -ForegroundColor Gray
    
    Write-Host "Step 5: Cleaning up..." -ForegroundColor Cyan
    ssh -i $SSH_KEY "$SSH_USER@$SSH_HOST" "rm -rf $TEMP_DIR"
    Write-Host "  Temp files removed" -ForegroundColor Gray
}

Write-Host ""
Write-Host "=== Deployment Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Navigate to: Administration → Modules" -ForegroundColor White
Write-Host "  2. Click: 'Scan directory' button" -ForegroundColor White
Write-Host "  3. Find 'Macro List Editor' and set Status: Enabled" -ForegroundColor White
Write-Host "  4. Access via: Configuration → Macro List Editor" -ForegroundColor White
Write-Host ""

if ($DryRun) {
    Write-Host "[DRY RUN MODE] No actual deployment was performed" -ForegroundColor Yellow
}
