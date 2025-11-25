@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: Zabbix Agent Auto-Update Task Scheduler
:: Creates a Windows scheduled task to check for updates periodically
:: ============================================================================

set "UPDATE_URL=%~1"
set "INSTALL_PATH=%~2"
set "CHECK_INTERVAL=%~3"

if "%UPDATE_URL%"=="" (
    echo Usage: %~nx0 ^<UPDATE_URL^> [INSTALL_PATH] [CHECK_INTERVAL]
    echo.
    echo UPDATE_URL: URL where updates are hosted
    echo INSTALL_PATH: Installation directory ^(default: C:\Program Files\ZabbixAgent^)
    echo CHECK_INTERVAL: How often to check ^(DAILY, WEEKLY, MONTHLY^)
    echo.
    echo Example: %~nx0 "https://example.com/updates" "C:\Program Files\ZabbixAgent" DAILY
    exit /b 1
)

if "%INSTALL_PATH%"=="" set "INSTALL_PATH=C:\Program Files\ZabbixAgent"
if "%CHECK_INTERVAL%"=="" set "CHECK_INTERVAL=DAILY"

echo ================================================
echo Zabbix Auto-Update Task Configuration
echo ================================================
echo Update URL: %UPDATE_URL%
echo Install Path: %INSTALL_PATH%
echo Check Interval: %CHECK_INTERVAL%
echo ================================================

:: Check for admin rights
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: This script requires administrator privileges!
    echo Please run as administrator.
    pause
    exit /b 1
)

:: Create autoupdate script wrapper
set "WRAPPER_SCRIPT=%INSTALL_PATH%\install\autoupdate_silent.bat"
echo @echo off > "%WRAPPER_SCRIPT%"
echo :: Silent auto-update wrapper >> "%WRAPPER_SCRIPT%"
echo call "%INSTALL_PATH%\install\autoupdate.bat" "%UPDATE_URL%" "%INSTALL_PATH%" ^>^> "%INSTALL_PATH%\log\autoupdate.log" 2^>^&1 >> "%WRAPPER_SCRIPT%"

:: Set schedule time based on interval
set "SCHEDULE_TIME=03:00"
set "SCHEDULE_FREQ=/SC %CHECK_INTERVAL%"

if /I "%CHECK_INTERVAL%"=="WEEKLY" (
    set "SCHEDULE_FREQ=/SC WEEKLY /D SUN"
)

if /I "%CHECK_INTERVAL%"=="MONTHLY" (
    set "SCHEDULE_FREQ=/SC MONTHLY /D 1"
)

:: Create scheduled task
echo Creating scheduled task...
schtasks /Create /TN "Zabbix Agent Auto-Update" /TR "\"%WRAPPER_SCRIPT%\"" %SCHEDULE_FREQ% /ST %SCHEDULE_TIME% /F /RU "SYSTEM" /RL HIGHEST

if %ERRORLEVEL%==0 (
    echo.
    echo ================================================
    echo Scheduled task created successfully!
    echo ================================================
    echo Task Name: Zabbix Agent Auto-Update
    echo Schedule: %CHECK_INTERVAL% at %SCHEDULE_TIME%
    echo Script: %WRAPPER_SCRIPT%
    echo Log: %INSTALL_PATH%\log\autoupdate.log
    echo ================================================
    echo.
    echo To view the task:
    echo   schtasks /Query /TN "Zabbix Agent Auto-Update" /V /FO LIST
    echo.
    echo To run the task manually:
    echo   schtasks /Run /TN "Zabbix Agent Auto-Update"
    echo.
    echo To delete the task:
    echo   schtasks /Delete /TN "Zabbix Agent Auto-Update" /F
    echo ================================================
) else (
    echo ERROR: Failed to create scheduled task!
    exit /b 1
)

pause
exit /b 0
