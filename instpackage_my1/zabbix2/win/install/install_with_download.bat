@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: Zabbix Agent Installer with Remote Binary Download
:: ============================================================================

:: Configuration
set "DEFAULTDSTPATH=C:\Program Files\ZabbixAgent"
set "DEFAULTXDSTPATH=C:\Zabbix"
set "DEFAULTSERVER=zabbix.example.com"
set "CUSTOMERID=Customer-"
set "BINARY_DOWNLOAD_URL=https://example.com/zabbix-binaries.zip"

:: Get script location
for %%I in ("%~dp0.") do set "CURRPATHBASE=%%~dpI"
set "CURRPATHBASE=%CURRPATHBASE:~0,-1%"
set "CURR=%CURRPATHBASE%"

:: State marker files
set "instsrcfilename=\install\.installsrcdir"
set "instmodefilename=\install\.useagent"

call :DETECT_INSTALLATION
call :CHECK_INSTALL_SOURCE
call :CHECK_BINARIES
goto MENU

:: ============================================================================
:: Check if binaries exist, offer to download
:: ============================================================================
:CHECK_BINARIES
if exist "%CURR%\bin\zabbix_agentd.exe" goto :EOF
if exist "%CURR%\bin\zabbix_agent2.exe" goto :EOF

echo.
echo ================================================
echo WARNING: Zabbix binaries not found!
echo ================================================
echo The following files are missing:
echo - bin\zabbix_agentd.exe
echo - bin\zabbix_agent2.exe
echo.
choice /c YN /n /m "Download binaries from remote URL? (Y/N): "
if %ERRORLEVEL%==2 (
    echo Cannot proceed without binaries!
    pause
    exit /b 1
)

echo.
set /p DOWNLOAD_URL="Enter download URL (ENTER=%BINARY_DOWNLOAD_URL%): "
if "%DOWNLOAD_URL%"=="" set "DOWNLOAD_URL=%BINARY_DOWNLOAD_URL%"

call :DOWNLOAD_AND_EXTRACT "%DOWNLOAD_URL%"
if %ERRORLEVEL% NEQ 0 (
    echo Download failed!
    pause
    exit /b 1
)
goto :EOF

:DOWNLOAD_AND_EXTRACT
set "TEMP_ZIP=%TEMP%\zabbix_binaries_%RANDOM%.zip"
echo Downloading from: %~1
echo.

:: Use download.bat if available, otherwise inline PowerShell
if exist "%CURR%\install\download.bat" (
    call "%CURR%\install\download.bat" "%~1" "%TEMP%" "zabbix_binaries.zip"
    set "TEMP_ZIP=%TEMP%\zabbix_binaries.zip"
) else (
    powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%~1' -OutFile '%TEMP_ZIP%' -UseBasicParsing}" 2>nul
)

if not exist "%TEMP_ZIP%" exit /b 1

echo Extracting files...
powershell -Command "Expand-Archive -Path '%TEMP_ZIP%' -DestinationPath '%CURR%' -Force" 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Extraction failed! Trying alternative method...
    :: Fallback: use built-in Windows extraction
    powershell -Command "Add-Type -A System.IO.Compression.FileSystem; [IO.Compression.ZipFile]::ExtractToDirectory('%TEMP_ZIP%', '%CURR%')" 2>nul
)

del "%TEMP_ZIP%" 2>nul
echo Binaries downloaded and extracted!
exit /b 0

:: ============================================================================
:: Main Menu
:: ============================================================================
:MENU
cls
echo ================================================
echo         Zabbix Agent Installer
echo ================================================
if "%ininstallsrc%"=="true" (
    echo 1. INSTALL ^(Copy + Configure + Install Service^)
    echo 0. Exit
    echo.
    echo Current directory: %CURR% ^(SOURCE^)
) else (
    echo 1. REINSTALL ^(Full reinstallation^)
    echo 2. CONFIGURE ^(Update configuration only^)
    echo 3. INSTALL SERVICE ^(Install Windows service^)
    echo 4. UNINSTALL ^(Remove service + files^)
    echo 5. RESTART SERVICE
    echo 6. DOWNLOAD BINARIES ^(Update binaries^)
    echo 8. CLEAN ^(Remove all files^)
    echo 0. Exit
    echo.
    echo Current directory: %CURR% ^(Agent v%agentno%^)
)
echo ================================================
choice /c 123456890 /n /m "Enter choice: "
set choice=%ERRORLEVEL%

if "%choice%"=="9" goto END
if "%choice%"=="1" call :FULL_INSTALL & goto MENU

if "%ininstallsrc%"=="false" (
    if "%choice%"=="2" call :CONFIGURE_AGENT & goto MENU
    if "%choice%"=="3" call :INSTALL_SERVICE & goto MENU
    if "%choice%"=="4" call :UNINSTALL_SERVICE & goto MENU
    if "%choice%"=="5" call :RESTART_SERVICE & goto MENU
    if "%choice%"=="6" call :CHECK_BINARIES & goto MENU
    if "%choice%"=="7" call :CLEAN_INSTALL & goto MENU
)

echo Invalid choice!
pause
goto MENU

:: ============================================================================
:: Installation Functions (same as install_v2.bat)
:: ============================================================================

:DETECT_INSTALLATION
for %%P in ("%DEFAULTDSTPATH%" "%DEFAULTXDSTPATH%") do (
    if exist "%%~P%instsrcfilename%" (
        findstr /C:"0" "%%~P%instsrcfilename%" >nul && (
            echo Found installation at %%~P
            choice /c YN /n /m "Switch to this location? (Y/N): "
            if !ERRORLEVEL!==1 set "CURR=%%~P" & goto :EOF
        )
    )
)
goto :EOF

:CHECK_INSTALL_SOURCE
set "ininstallsrc=false"
set "agentno=0"
if exist "%CURR%%instsrcfilename%" (
    findstr /C:"1" "%CURR%%instsrcfilename%" >nul && set "ininstallsrc=true"
)
if exist "%CURR%%instmodefilename%" (
    findstr /C:"1" "%CURR%%instmodefilename%" >nul && set "agentno=1"
    findstr /C:"2" "%CURR%%instmodefilename%" >nul && set "agentno=2"
)
goto :EOF

:FULL_INSTALL
echo.
echo === INSTALLATION ===
set /p DSTPATH="Target path (ENTER=%DEFAULTDSTPATH%, x=%DEFAULTXDSTPATH%): "
if /I "%DSTPATH%"=="x" set "DSTPATH=%DEFAULTXDSTPATH%"
if "%DSTPATH%"=="" set "DSTPATH=%DEFAULTDSTPATH%"

echo.
echo Source: %CURRPATHBASE%
echo Target: %DSTPATH%
choice /c YN /n /m "Proceed? (Y/N): "
if %ERRORLEVEL%==2 goto :EOF

if not exist "%DSTPATH%" mkdir "%DSTPATH%"

echo Copying files...
for /R "%CURRPATHBASE%" %%F in (*) do (
    set "FNAME=%%~nxF"
    if /I NOT "!FNAME!"==".installsrcdir" (
        set "REL=%%F"
        set "REL=!REL:%CURRPATHBASE%\=!"
        mkdir "%DSTPATH%\!REL!\.." 2>nul
        copy "%%F" "%DSTPATH%\!REL!" /Y >nul
    )
)

<nul set /p=0 >"%DSTPATH%%instsrcfilename%"
set "CURR=%DSTPATH%"

call :CHECK_INSTALL_SOURCE
call :CONFIGURE_AGENT
call :INSTALL_SERVICE
call :RESTART_SERVICE startonly
echo Installation complete!
pause
goto :EOF

:CONFIGURE_AGENT
echo.
echo === AGENT CONFIGURATION ===

if "%agentno%" NEQ "0" (
    echo Current agent: v%agentno%
    choice /c YN /n /m "Change agent version? (Y/N): "
    if !ERRORLEVEL!==2 goto :SKIP_AGENT_SELECT
    call :UNINSTALL_SERVICE
)

:SKIP_AGENT_SELECT
echo Select agent version:
choice /c 12 /n /m "[1] Agent v1  [2] Agent v2: "
set "agentno=%ERRORLEVEL%"
<nul set /p=%agentno% >"%CURR%%instmodefilename%"
echo Using Agent v%agentno%

set /p SERVERSTR="Server address (ENTER=%DEFAULTSERVER%): "
if "%SERVERSTR%"=="" set "SERVERSTR=%DEFAULTSERVER%"

set "SRC=%CURR%\conf\zabbix_agentd.conf"
if "%agentno%"=="2" set "SRC=%CURR%\conf\zabbix_agent2.conf"

move /y "%CURR%\install\.baseval.ini" "%CURR%\conf\.baseval.ini" 2>nul

for /f %%A in ('call inied.bat get "%CURR%\conf\.baseval.ini" "Hostname" "default"') do set "CUSTOMERID=%%A"
set /p HOSTNAMEGIVEN="Hostname (prefix: %CUSTOMERID%): "
set "HOSTNAMESTR=%CUSTOMERID%%HOSTNAMEGIVEN%"

set "DSTPATHLOG=%CURR%\log\agent\"
if "%agentno%"=="2" set "DSTPATHLOG=%CURR%\log\agent2\"
if not exist "%DSTPATHLOG%" mkdir "%DSTPATHLOG%"
set "DSTPATHLOG=%DSTPATHLOG%agent.log.txt"

call inied.bat set "%CURR%\conf\.baseval.ini" "ServerActive" "%SERVERSTR%" "default" "##::"
call inied.bat set "%CURR%\conf\.baseval.ini" "Hostname" "%HOSTNAMESTR%" "default" "##::"
call inied.bat set "%CURR%\conf\.baseval.ini" "LogType" "file" "default" "##::"
call inied.bat set "%CURR%\conf\.baseval.ini" "LogFile" "%DSTPATHLOG%" "default" "##::"
REM Pass INSTALL_PATH macro for expansion (CURR is the install path in this context)
call inied.bat proc "%CURR%\conf\.baseval.ini" "%SRC%" "default" "##::" "" "INSTALL_PATH=%CURR%"

echo Configuration updated!
pause
goto :EOF

:INSTALL_SERVICE
call :SET_AGENT_VARS
echo Installing service: %SCNAME%...
"%EXE%" --config "%CFG%" --install
echo Service installed!
pause
goto :EOF

:UNINSTALL_SERVICE
call :SET_AGENT_VARS
echo Uninstalling service: %SCNAME%...
net stop "%SCNAME%" 2>nul
"%EXE%" --config "%CFG%" --uninstall 2>nul
echo Service uninstalled!
pause
goto :EOF

:RESTART_SERVICE
call :SET_AGENT_VARS
if not "%~1"=="startonly" (
    echo Stopping %SCNAME%...
    net stop "%SCNAME%"
)
echo Starting %SCNAME%...
net start "%SCNAME%"
if not "%~1"=="startonly" pause
goto :EOF

:SET_AGENT_VARS
if "%agentno%"=="1" (
    set "CFG=%CURR%\conf\zabbix_agentd.conf"
    set "EXE=%CURR%\bin\zabbix_agentd.exe"
    set "SCNAME=Zabbix Agent"
) else (
    set "CFG=%CURR%\conf\zabbix_agent2.conf"
    set "EXE=%CURR%\bin\zabbix_agent2.exe"
    set "SCNAME=Zabbix Agent 2"
)
goto :EOF

:CLEAN_INSTALL
if "%ininstallsrc%"=="true" (
    echo Cannot delete source directory!
    pause
    goto :EOF
)
echo WARNING: This will remove all files in %CURR%
choice /c YN /n /m "Continue? (Y/N): "
if %ERRORLEVEL%==2 goto :EOF

call :UNINSTALL_SERVICE
rmdir /s /q "%CURR%"
echo Installation cleaned!
set "CURR=%CURRPATHBASE%"
call :DETECT_INSTALLATION
call :CHECK_INSTALL_SOURCE
pause
goto :EOF

:END
echo Exiting...
exit /b
