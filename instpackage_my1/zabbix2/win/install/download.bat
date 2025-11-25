@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: Zabbix Agent Binary Downloader
:: Downloads Zabbix agent binaries from a specified URL
:: Uses native Windows tools (PowerShell preferred, fallback to certutil/bitsadmin)
:: ============================================================================

set "DOWNLOAD_URL=%~1"
set "DESTINATION=%~2"
set "FILENAME=%~3"

if "%DOWNLOAD_URL%"=="" (
    echo Usage: %~nx0 ^<URL^> ^<Destination^> ^<Filename^>
    echo.
    echo Example: %~nx0 "https://example.com/zabbix.zip" "C:\Temp" "zabbix.zip"
    exit /b 1
)

if "%DESTINATION%"=="" set "DESTINATION=%TEMP%"
if "%FILENAME%"=="" (
    for %%F in ("%DOWNLOAD_URL%") do set "FILENAME=%%~nxF"
)

set "FULLPATH=%DESTINATION%\%FILENAME%"

echo ================================================
echo Zabbix Binary Downloader
echo ================================================
echo URL: %DOWNLOAD_URL%
echo Destination: %FULLPATH%
echo ================================================

:: Create destination directory if needed
if not exist "%DESTINATION%" mkdir "%DESTINATION%"

:: Try PowerShell first (most reliable, available on Windows 7+)
call :TRY_POWERSHELL && goto :SUCCESS

:: Fallback to certutil (available on Windows Vista+)
call :TRY_CERTUTIL && goto :SUCCESS

:: Last resort: bitsadmin (deprecated but widely available)
call :TRY_BITSADMIN && goto :SUCCESS

echo ERROR: All download methods failed!
exit /b 1

:SUCCESS
echo.
echo Download completed successfully!
echo File: %FULLPATH%

:: Verify file exists and has size
if not exist "%FULLPATH%" (
    echo ERROR: Downloaded file not found!
    exit /b 1
)

for %%A in ("%FULLPATH%") do set "FILESIZE=%%~zA"
if "%FILESIZE%"=="0" (
    echo ERROR: Downloaded file is empty!
    del "%FULLPATH%"
    exit /b 1
)

echo File size: %FILESIZE% bytes
exit /b 0

:: ============================================================================
:: Download Methods
:: ============================================================================

:TRY_POWERSHELL
echo Attempting download with PowerShell...
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; try { $ProgressPreference = 'SilentlyContinue'; Invoke-WebRequest -Uri '%DOWNLOAD_URL%' -OutFile '%FULLPATH%' -UseBasicParsing; exit 0 } catch { exit 1 }}" 2>nul
if %ERRORLEVEL%==0 goto :EOF
echo PowerShell download failed.
exit /b 1

:TRY_CERTUTIL
echo Attempting download with certutil...
certutil -urlcache -split -f "%DOWNLOAD_URL%" "%FULLPATH%" >nul 2>&1
if %ERRORLEVEL%==0 (
    certutil -urlcache -split -f "%DOWNLOAD_URL%" delete >nul 2>&1
    goto :EOF
)
echo Certutil download failed.
exit /b 1

:TRY_BITSADMIN
echo Attempting download with bitsadmin...
bitsadmin /transfer "ZabbixDownload" /priority high "%DOWNLOAD_URL%" "%FULLPATH%" >nul 2>&1
if %ERRORLEVEL%==0 goto :EOF
echo Bitsadmin download failed.
exit /b 1
