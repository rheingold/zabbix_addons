@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: Zabbix Agent Auto-Update Script
:: Checks for updates from a remote location and silently updates if newer
:: ============================================================================

:: Configuration
set "UPDATE_URL=%~1"
set "INSTALL_PATH=%~2"
set "VERSION_FILE=.version"
set "TEMP_UPDATE=%TEMP%\zabbix_update_%RANDOM%"

if "%UPDATE_URL%"=="" (
    echo Usage: %~nx0 ^<UPDATE_URL^> [INSTALL_PATH]
    echo.
    echo Example: %~nx0 "https://example.com/zabbix-update.zip" "C:\Program Files\ZabbixAgent"
    exit /b 1
)

if "%INSTALL_PATH%"=="" (
    set "INSTALL_PATH=C:\Program Files\ZabbixAgent"
)

echo ================================================
echo Zabbix Agent Auto-Update
echo ================================================
echo Update URL: %UPDATE_URL%
echo Install Path: %INSTALL_PATH%
echo ================================================

:: Check if installation exists
if not exist "%INSTALL_PATH%" (
    echo ERROR: Installation not found at %INSTALL_PATH%
    exit /b 1
)

:: Read current version
set "CURRENT_VERSION=0"
if exist "%INSTALL_PATH%\%VERSION_FILE%" (
    for /f "usebackq delims=" %%A in ("%INSTALL_PATH%\%VERSION_FILE%") do set "CURRENT_VERSION=%%A"
)
echo Current version: %CURRENT_VERSION%

:: Download version info from update server
echo Checking for updates...
set "REMOTE_VERSION_FILE=%TEMP%\zabbix_remote_version_%RANDOM%.txt"
call :DOWNLOAD_FILE "%UPDATE_URL%/.version" "%REMOTE_VERSION_FILE%"

if not exist "%REMOTE_VERSION_FILE%" (
    echo ERROR: Failed to retrieve remote version information
    exit /b 1
)

:: Read remote version
set "REMOTE_VERSION=0"
for /f "usebackq delims=" %%A in ("%REMOTE_VERSION_FILE%") do set "REMOTE_VERSION=%%A"
echo Remote version: %REMOTE_VERSION%

del "%REMOTE_VERSION_FILE%"

:: Compare versions
if "%CURRENT_VERSION%"=="%REMOTE_VERSION%" (
    echo Already up to date!
    exit /b 0
)

if %REMOTE_VERSION% LEQ %CURRENT_VERSION% (
    echo Current version is newer or equal. No update needed.
    exit /b 0
)

echo New version available: %REMOTE_VERSION%
echo Downloading update...

:: Create temp directory
mkdir "%TEMP_UPDATE%"

:: Download update package
set "UPDATE_PACKAGE=%TEMP_UPDATE%\update.zip"
call :DOWNLOAD_FILE "%UPDATE_URL%/zabbix-update.zip" "%UPDATE_PACKAGE%"

if not exist "%UPDATE_PACKAGE%" (
    echo ERROR: Failed to download update package
    rmdir /s /q "%TEMP_UPDATE%"
    exit /b 1
)

:: Detect agent version
set "agentno=0"
if exist "%INSTALL_PATH%\install\.useagent" (
    for /f "delims=" %%A in (%INSTALL_PATH%\install\.useagent) do set "agentno=%%A"
)

:: Stop service before update
echo Stopping Zabbix Agent service...
if "%agentno%"=="1" (
    net stop "Zabbix Agent" 2>nul
) else if "%agentno%"=="2" (
    net stop "Zabbix Agent 2" 2>nul
) else (
    :: Try both
    net stop "Zabbix Agent" 2>nul
    net stop "Zabbix Agent 2" 2>nul
)

:: Wait a moment for service to stop
timeout /t 2 /nobreak >nul

:: Extract update
echo Extracting update...
powershell -Command "Expand-Archive -Path '%UPDATE_PACKAGE%' -DestinationPath '%TEMP_UPDATE%' -Force" 2>nul

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to extract update package
    rmdir /s /q "%TEMP_UPDATE%"
    exit /b 1
)

:: Backup current installation (excluding logs)
echo Creating backup...
set "BACKUP_DIR=%INSTALL_PATH%_backup_%CURRENT_VERSION%_%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%"
set "BACKUP_DIR=%BACKUP_DIR: =0%"

mkdir "%BACKUP_DIR%" 2>nul
xcopy /E /I /Y /EXCLUDE:%~f0.exclude "%INSTALL_PATH%\*" "%BACKUP_DIR%\" >nul

:: Create exclusion list for backup (don't backup logs)
echo log\ > "%~f0.exclude"
echo *.log >> "%~f0.exclude"
echo *.log.txt >> "%~f0.exclude"

:: Update files (preserve configuration and logs)
echo Updating files...
for /R "%TEMP_UPDATE%" %%F in (*) do (
    set "FILE=%%F"
    set "REL=!FILE:%TEMP_UPDATE%\=!"
    
    :: Skip configuration files and logs
    echo !REL! | findstr /C:"conf\" >nul
    if !ERRORLEVEL! NEQ 0 (
        echo !REL! | findstr /C:"log\" >nul
        if !ERRORLEVEL! NEQ 0 (
            mkdir "%INSTALL_PATH%\!REL!\.." 2>nul
            copy /Y "%%F" "%INSTALL_PATH%\!REL!" >nul
        )
    )
)

:: Update version file
echo %REMOTE_VERSION% > "%INSTALL_PATH%\%VERSION_FILE%"

:: Restart service
echo Starting Zabbix Agent service...
if "%agentno%"=="1" (
    net start "Zabbix Agent" 2>nul
) else if "%agentno%"=="2" (
    net start "Zabbix Agent 2" 2>nul
) else (
    :: Try both
    net start "Zabbix Agent" 2>nul
    net start "Zabbix Agent 2" 2>nul
)

:: Cleanup
echo Cleaning up...
rmdir /s /q "%TEMP_UPDATE%"
del "%~f0.exclude" 2>nul

echo.
echo ================================================
echo Update completed successfully!
echo ================================================
echo Old version: %CURRENT_VERSION%
echo New version: %REMOTE_VERSION%
echo Backup: %BACKUP_DIR%
echo ================================================

:: Write update log
echo [%date% %time%] Updated from %CURRENT_VERSION% to %REMOTE_VERSION% >> "%INSTALL_PATH%\update.log"

exit /b 0

:: ============================================================================
:: Download file using available methods
:: ============================================================================
:DOWNLOAD_FILE
set "URL=%~1"
set "DEST=%~2"

:: Try PowerShell
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; try { Invoke-WebRequest -Uri '%URL%' -OutFile '%DEST%' -UseBasicParsing; exit 0 } catch { exit 1 }}" 2>nul
if %ERRORLEVEL%==0 goto :EOF

:: Try certutil
certutil -urlcache -split -f "%URL%" "%DEST%" >nul 2>&1
if %ERRORLEVEL%==0 (
    certutil -urlcache -split -f "%URL%" delete >nul 2>&1
    goto :EOF
)

:: Try bitsadmin
bitsadmin /transfer "ZabbixUpdate" /priority high "%URL%" "%DEST%" >nul 2>&1
goto :EOF
