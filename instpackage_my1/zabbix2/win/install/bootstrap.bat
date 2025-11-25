@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: Zabbix Agent Bootstrap/Update Helper
:: This script downloads, unpacks, and copies Zabbix files
:: Can be called standalone or from main install script
:: ============================================================================

set "DEFAULT_URL=https://updates.zabbix.example.com/zabbix-win.zip"
set "DOWNLOAD_URL=%DEFAULT_URL%"
set "TARGET_PATH="
set "MODE=%~1"
set "TEMP_DIR=%TEMP%\zabbix_bootstrap_%RANDOM%"

:: Parse arguments
if /I "%MODE%"=="download" (
    set "DOWNLOAD_URL=%~2"
    set "TARGET_PATH=%~3"
) else if /I "%MODE%"=="local" (
    set "SOURCE_PATH=%~2"
    set "TARGET_PATH=%~3"
) else (
    goto :USAGE
)

echo ================================================
echo   Zabbix Agent Bootstrap/Update Helper
echo ================================================
echo.

:: Check for admin rights
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: This script requires administrator privileges!
    echo Please run as administrator.
    pause
    exit /b 1
)

if /I "%MODE%"=="download" (
    call :DOWNLOAD_AND_EXTRACT
) else if /I "%MODE%"=="local" (
    call :EXTRACT_LOCAL
)

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ================================================
    echo Bootstrap/Update failed!
    echo ================================================
    pause
    exit /b 1
)

echo.
echo ================================================
echo Bootstrap/Update completed successfully!
echo ================================================
echo Files are ready at: %TARGET_PATH%
echo.
pause
exit /b 0

:: ============================================================================
:DOWNLOAD_AND_EXTRACT
echo Mode: Download and extract
echo URL: %DOWNLOAD_URL%
echo Target: %TARGET_PATH%
echo.

mkdir "%TEMP_DIR%" 2>nul
set "DOWNLOAD_FILE=%TEMP_DIR%\zabbix.zip"

echo Downloading files...
call "%~dp0download.bat" "%DOWNLOAD_URL%" "%TEMP_DIR%" "zabbix.zip"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Download failed!
    rmdir /s /q "%TEMP_DIR%" 2>nul
    exit /b 1
)

echo Extracting files...
mkdir "%TARGET_PATH%" 2>nul
powershell -Command "Expand-Archive -Path '%DOWNLOAD_FILE%' -DestinationPath '%TARGET_PATH%' -Force" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Extraction failed!
    rmdir /s /q "%TEMP_DIR%" 2>nul
    exit /b 1
)

:: Cleanup
rmdir /s /q "%TEMP_DIR%" 2>nul
exit /b 0

:: ============================================================================
:EXTRACT_LOCAL
echo Mode: Extract from local source
echo Source: %SOURCE_PATH%
echo Target: %TARGET_PATH%
echo.

if not exist "%SOURCE_PATH%" (
    echo ERROR: Source path not found: %SOURCE_PATH%
    exit /b 1
)

echo Extracting files...
mkdir "%TARGET_PATH%" 2>nul

:: Check if source is a ZIP file or directory
if /I "%SOURCE_PATH:~-4%"==".zip" (
    powershell -Command "Expand-Archive -Path '%SOURCE_PATH%' -DestinationPath '%TARGET_PATH%' -Force" 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Extraction failed!
        exit /b 1
    )
) else (
    :: Copy from directory
    xcopy /E /I /Y "%SOURCE_PATH%\*" "%TARGET_PATH%\" >nul
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Copy failed!
        exit /b 1
    )
)

exit /b 0

:: ============================================================================
:USAGE
echo Usage:
echo   %~nx0 download ^<URL^> ^<target_path^>
echo   %~nx0 local ^<source_path^> ^<target_path^>
echo.
echo Examples:
echo   %~nx0 download https://example.com/zabbix.zip C:\zabbix
echo   %~nx0 local C:\Temp\zabbix.zip C:\zabbix
echo   %~nx0 local C:\Temp\zabbix_files C:\zabbix
echo.
pause
exit /b 1
