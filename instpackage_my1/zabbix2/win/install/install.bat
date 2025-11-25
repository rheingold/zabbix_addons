@echo off
setlocal enabledelayedexpansion

:: Check for silent mode
set "SILENT_MODE=0"
if /I "%~1"=="/SILENT" (
    set "SILENT_MODE=1"
)

set DSTPATH=
set SRCPATH=.
:: Get parent folder
for %%I in ("%SRCPATH%") do set "SRCPATH=%%~dpI"
:: Remove trailing backslash
set "CURRPATHBASE=%SRCPATH:~0,-1%"
set "DEFAULTDSTPATH=C:\Program Files\ZabbixAgent"
set "DEFAULTXDSTPATH=C:\Zabbix"
set "DEFAULTSERVER=zabbix.example.com"
set "CUSTOMERID_DEFAULT=Customer-"

:: Current folder
set "CURR=%CURRPATHBASE%"
set "instsrcfilename=\install\.installsrcdir"
set "instmodefilename=\install\.useagent"

:: If silent mode, skip installation detection and go directly to installation
if "%SILENT_MODE%"=="1" (
    echo Running in SILENT mode...
    call :SILENT_INSTALL
    exit /b
)

:FINDINSTALLATION
setlocal enabledelayedexpansion
set "exists=false"
set "choice=0"
SET "INFILE=%DEFAULTDSTPATH%%instsrcfilename%"
if exist "%INFILE%" (
 findstr /C:"0" "%INFILE%" >nul && set "exists=true"
)
if "!exists!"=="true" (
	setlocal enabledelayedexpansion
	echo The installation has been found at path %DEFAULTDSTPATH%
	choice /c yn /n /m "Shall I switch the context there, (Y)es? (N)o=keep current folder %CURR%"
	set "choice=!ERRORLEVEL!"
)

if "!choice!"=="1" (
	set "CURR=%DEFAULTDSTPATH%"
) else (
		setlocal enabledelayedexpansion
		set "exists=false"
		SET "INFILE=%DEFAULTXDSTPATH%%instsrcfilename%"
		if exist "!INFILE!" (
			findstr /C:"0" "!INFILE!" >nul && set "exists=true"
		) 
		if "!exists!"=="true" (
			setlocal enabledelayedexpansion
			echo The installation has been found at path %DEFAULTXDSTPATH%
			choice /c yn /n /m "Shall I switch the context there, (Y)es? (N)o=keep current folder %CURR%"
			set "choice=!ERRORLEVEL!"
			if "!choice!"=="1" set "CURR=%DEFAULTXDSTPATH%"
		)
)

:SRCDIRCHECK
setlocal enabledelayedexpansion
SET "INFILE=%CURR%%instsrcfilename%"
set "ininstallsrc=false"
::echo infile %infile%
::echo ininstallsrc %ininstallsrc%
findstr /C:"1" "%INFILE%" >nul && set "ininstallsrc=true"
SET "INFILE=%CURR%%instmodefilename%"
set "agentno=0"
if exist "%INFILE%" (
 findstr /C:"1" "%INFILE%" >nul && set "agentno=1"
 findstr /C:"2" "%INFILE%" >nul && set "agentno=2"
)
set repetitions=1

goto MENU

:MENU
echo ================================================
echo Choose the option:
if "%ininstallsrc%"=="true" (
	echo 1. INSTALL ^(Copy+configure+install SVC^)
	echo 2. -
	echo 3. -
	echo 4. -
	echo 5. -
	echo 7. -
	echo 8. -
	echo 0. Exit
	echo ================================================
	<nul set /p=9. Change current working directory --- 
	echo %CURR% --- ^(inst.src.^) ---

) else (
	echo 1. Force-REINSTALL ^(Copy+configure+install SVC^)
	echo 2. CONFIGURE
	echo 3. install ^(install SVC^)
	echo 4. uninstal ^(uninstall SVC+[delete files]^)
	echo 5. ^(re^)start service
	echo 7. UPDATE source ^(download/copy new files^)
	echo 8. CLEAN ^(delete files and dir^)
	echo 0. Exit
	echo ================================================
	<nul set /p=9. Change current working directory --- 
	echo %CURR% --- ^(AgentVer:%agentno%^) ---
)
echo ================================================
rem set /p choice=Enter choice [1-5,7-9,0]:
choice /c 123457890 /n /m "Enter choice [1-5,7-9,0]: "
set choice=%ERRORLEVEL%
if "%choice%"=="1" (
	set "nocopydone=0"
	call :COPY_FILES nocopydone
	if not "!nocopydone!"=="1" (
		call :CFG_INSTALL
		call :SVC_INST
		call :SVC_RESTART "startonly"
	)
	goto SRCDIRCHECK

)
if "%ininstallsrc%"=="false" (
	if "%choice%"=="2" (
		call :CFG_INSTALL
		goto SRCDIRCHECK
	)

	if "%choice%"=="3" (
		call :SVC_INST
		goto SRCDIRCHECK
	)
	if "%choice%"=="4" (
		call :SVC_UNINST
		call :CLEARCURR
		goto SRCDIRCHECK
	)
	if "%choice%"=="5" (
		call :SVC_RESTART
		goto SRCDIRCHECK
	)

	if "%choice%"=="6" (
		call :CLEARCURR
		goto SRCDIRCHECK
	)
)
if "%choice%"=="7" (
    call :CHANGE_SOURCE
    goto SRCDIRCHECK
)
if "%choice%"=="8" (
	call :CLEARCURR
	goto END
)
if "%choice%"=="9" (
	goto END
)
rem Already obsolete but lets leave it here
echo Inactive choice! %choice%
pause
set /a repetitions+=1
if "%repetitions%"=="3" goto END
goto MENU


:COPY_FILES
set "INSTALLWIZARDRUN=1"
set /p DSTPATH=Enter target path for installation (default(ENTER)=%DEFAULTDSTPATH%, x=%DEFAULTXDSTPATH%)
if /I "%DSTPATH%"=="x" set "DSTPATH=%DEFAULTXDSTPATH%"
if "%DSTPATH%"=="" set "DSTPATH=%DEFAULTDSTPATH%"
:: Current folder !!!!!!!!!!!!!! POZOR TOHLE SE MUSI DORESIT ABY SE TO PREPNULO SPRAVNE KDYZ REINSTALUJU !!!!!!!!!!!!!!
set "oldCURR=%CURR%"
set "CURR=%CD%"

:: Get parent folder
for %%I in ("%CURR%") do set "SRCPATH=%%~dpI"
:: Remove trailing backslash
set "SRCPATH=%SRCPATH:~0,-1%"
rem set "SRCPATH=%CD%\.."
echo Copy from %SRCPATH%
echo to %DSTPATH%
choice /c YN /n /m "(Y)es or (N)o?"
set choice=%ERRORLEVEL%
if "%choice%"=="2" goto COPY_FILES_NOCOPY
if not exist "%DSTPATH%" (
    mkdir "%DSTPATH%"
    echo Created path: %DSTPATH%
)

:: Loop over all files recursively
for /R "%SRCPATH%" %%F in (*) do (
    set "FNAME=%%~nxF"
	:: Check for exclusion
    if /I NOT "!FNAME!"==".installsrcdir" if /I NOT "!FNAME!"==".installsrcdir" (
		set "REL=%%F"
		setlocal enabledelayedexpansion
		set "REL=!REL:%SRCPATH%\=!"
		mkdir "%DSTPATH%\!REL!\.." 2>nul
		copy "%%F" "%DSTPATH%\!REL!" /Y
		endlocal
	)
)
:: Write that this is NOT the installsrcdir
SET "OUTFILE=%DSTPATH%%instsrcfilename%
<nul set /p=0 >"%OUTFILE%"

::switch the install there
::cls
set "CURR=%DSTPATH%"
exit/b

:COPY_FILES_NOCOPY
cls
echo Nothing done...
echo.
set "CURR=%OLDCURR%"
endlocal & set "%~1=1"
exit /b


:CFG_INSTALL
::For the signature
for /f "tokens=1-3 delims=/: " %%a in ("%date%") do (
    set "today=%%c-%%a-%%b"
)
set "now=%time:~0,8%"

set "askfirst=no"
if "%agentno%"=="1" set "askfirst=yes"
if "%agentno%"=="2" set "askfirst=yes"
if "%askfirst%"=="yes" (
	echo You seem to have currently configured service of agent ver.!agentno! Do you want to change that ^(automatic uninstall will follow^)?
	choice /c YNC /n /m "(Y)es, (N)o, (C)ancel^?"
	set "choice=!ERRORLEVEL!"
	if "!choice!"=="2" goto CFG_INSTALL_skipagent
	if "!choice!"=="3" goto MENU
)

echo Use Agent no. 1 or 2.^?
rem set /p choice=Enter choice [1-5]:
choice /c 12 /n /m "Enter choice [1 or 2]: "
set "cho=%ERRORLEVEL%"
if "%askfirst%"=="yes" (
	if not "!cho!"=="!agentno!" (
		set "CALLBACK=CFG_INSTALL_aftersvcstop"
		set "INSTALLWIZARDRUN=1"
		call :SVC_UNINST
	) else (
		echo You have selected same number - we are just continuing.
	)
)
SET "OUTFILE=%CURR%%instmodefilename%
<nul set /p=%cho% >"%OUTFILE%"
set "agentno=%cho%"
:CFG_INSTALL_skipagent
echo Using agent no. %agentno%..

:: Source file
set "SRC=%CURR%\conf\zabbix_agentd.conf"
if "%agentno%"=="2" set "SRC=%CURR%\conf\zabbix_agent2.conf"

set "DSTPATHLOG=%DSTPATH%\log\agent\"
if "%agentno%"=="2" set "DSTPATHLOG=%DSTPATH%\log\agent2"
if not exist "%DSTPATHLOG%" mkdir "%DSTPATHLOG%"
set "DSTPATHLOG=%DSTPATHLOG%\agent.log.txt"

:: Use inied.bat input mode to read values based on .baseval.ini metadata comments
echo.
echo Configuration prompts based on .baseval.ini metadata:
echo.

:: ServerActive - uses metadata ;;INST|d|Zabbix Server Address
set "SERVERSTR="
for /f "delims=" %%A in ('call inied.bat input "%CURR%\install\.baseval.ini" "ServerActive" "default"') do set "SERVERSTR=%%A"
if not "%SERVERSTR%"=="" (
    call inied.bat set "%CURR%\install\.baseval.ini" "ServerActive" "%SERVERSTR%" "default" "##::"
)

:: Hostname - uses metadata ;;INST|am|Hostname Prefix (computer name will be appended)
set "HOSTNAMESTR="
for /f "delims=" %%A in ('call inied.bat input "%CURR%\install\.baseval.ini" "Hostname" "default"') do set "HOSTNAMESTR=%%A"
if not "%HOSTNAMESTR%"=="" (
    call inied.bat set "%CURR%\install\.baseval.ini" "Hostname" "%HOSTNAMESTR%" "default" "##::"
)

:: LogType - uses metadata ;;INST|m|Log Type (auto-configured)
:: This is mandatory (m) mode, so no user prompt - installer fills it
call inied.bat set "%CURR%\install\.baseval.ini" "LogType" "file" "default" "##::"

:: LogFile - uses metadata ;;INST|m|Log File Path (auto-configured)
:: This is mandatory (m) mode, so no user prompt - installer fills it
call inied.bat set "%CURR%\install\.baseval.ini" "LogFile" "%DSTPATHLOG%" "default" "##::"

:: Note: Aggplugin paths use %{INSTALL_PATH}% macro in .baseval.ini
:: They will be expanded during 'proc' operation below
:: Set INSTALL_PATH for macro expansion (same as DSTPATH in interactive mode)
set "INSTALL_PATH=%DSTPATH%"

rem Pass macro map to inied.bat so macro expansion does not rely on environment variables
call inied.bat proc "%CURR%\install\.baseval.ini" %SRC% "default" "##::" "INSTALL_PATH=%DSTPATH%"


endlocal & set "%~1=0"

exit /b

:SVC_INST
if "%agentno%"=="1" (
	set "CFG=%CURR%\conf\zabbix_agentd.conf"
	set "EXE=%CURR%\bin\zabbix_agentd.exe"
	set "SCNAME=Zabbix Agent"
)
if "%agentno%"=="2" (
	set "CFG=%CURR%\conf\zabbix_agent2.conf"
	set "EXE=%CURR%\bin\zabbix_agent2.exe"
	set "SCNAME=Zabbix Agent 2"
)
%EXE% --config !CFG! --install
exit /b

:SVC_UNINST
if "%agentno%"=="1" (
	set "CFG=%CURR%\conf\zabbix_agentd.conf"
	set "EXE=%CURR%\bin\zabbix_agentd.exe"
	set "SCNAME=Zabbix Agent"
)
if "%agentno%"=="2" (
	set "CFG=%CURR%\conf\zabbix_agent2.conf"
	set "EXE=%CURR%\bin\zabbix_agent2.exe"
	set "SCNAME=Zabbix Agent 2"
)
net stop "!SCNAME!" 2>&1
%EXE% --config !CFG! --uninstall 2>&1
exit /b

:SVC_RESTART
if "%agentno%"=="1" (
	set "SCNAME=Zabbix Agent"
)
if "%agentno%"=="2" (
	set "SCNAME=Zabbix Agent 2"
)
if not "%~1"=="startonly" (
	net stop "%SCNAME%"
)
net start "%SCNAME%"
exit /b

:CLEARCURR
call :SVC_UNINST
set "CALLBACK=0"
if "%ininstallsrc%"=="false" (
	if exist "%CURR%" (
		rmdir /s /q "%CURR%"
		echo Deleted folder: %CURR%
		if "%CURR%"=="%CURRPATHBASE%" (
			echo Deleted installsource.
		) else (
			set "CURR=%CURRPATHBASE%"
			goto FINDINSTALLATION
		)
	)
) else (
	echo Cannot delete source installation diretory.
)
exit /b

:CHANGE_SOURCE
:: Update source files from download or local source
echo ================================================
echo   UPDATE SOURCE
echo ================================================
echo Current directory: %CURR%
echo.
echo Options:
echo 1. Download from URL
echo 2. Copy from local path
echo 0. Cancel
echo.
choice /c 120 /n /m "Enter choice [1,2,0]: "
set "update_choice=%ERRORLEVEL%"

if "%update_choice%"=="3" exit /b

if "%update_choice%"=="1" (
    set /p "DOWNLOAD_URL=Enter download URL (default=https://updates.zabbix.example.com/zabbix-win.zip): "
    if "!DOWNLOAD_URL!"=="" set "DOWNLOAD_URL=https://updates.zabbix.example.com/zabbix-win.zip"
    
    echo.
    echo Downloading and extracting from: !DOWNLOAD_URL!
    echo Target: %CURR%
    echo.
    choice /c YN /n /m "Continue (Y)es or (N)o? "
    if !ERRORLEVEL! NEQ 1 exit /b
    
    call "%~dp0bootstrap.bat" download "!DOWNLOAD_URL!" "%CURR%"
    if !ERRORLEVEL! EQU 0 (
        echo.
        echo Source updated successfully!
    ) else (
        echo.
        echo Source update failed!
    )
)

if "%update_choice%"=="2" (
    set /p "SOURCE_PATH=Enter source path (ZIP file or directory): "
    if "!SOURCE_PATH!"=="" (
        echo ERROR: Source path cannot be empty!
        pause
        exit /b
    )
    
    echo.
    echo Copying/Extracting from: !SOURCE_PATH!
    echo Target: %CURR%
    echo.
    choice /c YN /n /m "Continue (Y)es or (N)o? "
    if !ERRORLEVEL! NEQ 1 exit /b
    
    call "%~dp0bootstrap.bat" local "!SOURCE_PATH!" "%CURR%"
    if !ERRORLEVEL! EQU 0 (
        echo.
        echo Source updated successfully!
    ) else (
        echo.
        echo Source update failed!
    )
)

pause
exit /b

:END
echo Exiting...
exit /b

:SILENT_INSTALL
:: Silent installation subroutine
echo ================================================
echo   Zabbix Agent Silent Install
echo ================================================
echo.

:: Read configuration from .baseval.ini
echo Reading configuration from .baseval.ini...
set "BASEVAL_FILE=%CURR%\install\.baseval.ini"

:: Read values from [default] section using inied.bat
for /f "delims=" %%V in ('call "%CURR%\install\inied.bat" get "%BASEVAL_FILE%" "ServerActive" "default"') do set "SERVER_ADDRESS=%%V"
for /f "delims=" %%V in ('call "%CURR%\install\inied.bat" get "%BASEVAL_FILE%" "Hostname" "default"') do set "CUSTOMER_PREFIX=%%V"

:: Read values from [bootstrap-silent-install-only] section using inied.bat
for /f "delims=" %%V in ('call "%CURR%\install\inied.bat" get "%BASEVAL_FILE%" "InstallPath" "bootstrap-silent-install-only"') do set "INSTALL_PATH=%%V"
for /f "delims=" %%V in ('call "%CURR%\install\inied.bat" get "%BASEVAL_FILE%" "AutoAppendHostname" "bootstrap-silent-install-only"') do set "AUTO_APPEND_HOSTNAME=%%V"
for /f "delims=" %%V in ('call "%CURR%\install\inied.bat" get "%BASEVAL_FILE%" "AutoStart" "bootstrap-silent-install-only"') do set "AUTO_START=%%V"
for /f "delims=" %%V in ('call "%CURR%\install\inied.bat" get "%BASEVAL_FILE%" "CreateLogs" "bootstrap-silent-install-only"') do set "CREATE_LOGS=%%V"

:: Read agent version from .useagent file
for /f "usebackq" %%V in ("%CURR%\install\.useagent") do set "AGENT_VERSION=%%V"

echo Agent Version: !AGENT_VERSION!
echo Installation path: !INSTALL_PATH!
echo Server: !SERVER_ADDRESS!
echo Customer Prefix: !CUSTOMER_PREFIX!
echo Auto Append: !AUTO_APPEND_HOSTNAME!
echo Auto Start: !AUTO_START!
echo Create Logs: !CREATE_LOGS!
echo.

:: Set up environment
set "ininstallsrc=true"
set "agentno=!AGENT_VERSION!"

:: Create destination directory
if not exist "!INSTALL_PATH!" (
    mkdir "!INSTALL_PATH!"
    echo Created directory: !INSTALL_PATH!
)

:: Copy files from current directory to installation path
echo Copying files...
for /R "%CURR%" %%F in (*) do (
    set "FNAME=%%~nxF"
    if /I NOT "!FNAME!"==".installsrcdir" (
        set "REL=%%F"
        setlocal enabledelayedexpansion
        set "REL=!REL:%CURR%\=!"
        mkdir "!INSTALL_PATH!\!REL!\.." 2>nul
        copy "%%F" "!INSTALL_PATH!\!REL!" /Y >nul
        endlocal
    )
)

:: Mark as installed directory
SET "OUTFILE=!INSTALL_PATH!!instsrcfilename!"
<nul set /p=0 >"%OUTFILE%"

:: Mark agent version
SET "OUTFILE=!INSTALL_PATH!!instmodefilename!"
<nul set /p=!AGENT_VERSION! >"%OUTFILE%"

:: Create log directories if requested
if "!CREATE_LOGS!"=="1" (
    echo Creating log directories...
    mkdir "!INSTALL_PATH!\log\agent" 2>nul
    mkdir "!INSTALL_PATH!\log\agent2" 2>nul
)

:: Configure agent (silent - use .baseval.ini template)
echo Configuring agent...

:: Read configuration from .baseval.ini (already updated by bootstrap)
for /f "usebackq tokens=1,* delims==" %%A in ("!INSTALL_PATH!\install\.baseval.ini") do (
    if /I "%%A"=="ServerActive" set "SERVER_ADDRESS=%%B"
    if /I "%%A"=="Hostname" set "CUSTOMER_PREFIX=%%B"
)

:: Set log file path
set "LOGFILEPATH=!INSTALL_PATH!\log\agent\agent.log.txt"
if "!AGENT_VERSION!"=="2" set "LOGFILEPATH=!INSTALL_PATH!\log\agent2\agent.log.txt"

:: Build hostname - append computer name if AUTO_APPEND_HOSTNAME is enabled
set "FULL_HOSTNAME=!CUSTOMER_PREFIX!"
if "!AUTO_APPEND_HOSTNAME!"=="1" (
    if not "!CUSTOMER_PREFIX:~-1!"=="-" (
        set "FULL_HOSTNAME=!CUSTOMER_PREFIX!-%COMPUTERNAME%"
    ) else (
        set "FULL_HOSTNAME=!CUSTOMER_PREFIX!%COMPUTERNAME%"
    )
)
echo Using hostname: !FULL_HOSTNAME!

:: Update hostname in .baseval.ini if it was modified
if not "!FULL_HOSTNAME!"=="!CUSTOMER_PREFIX!" (
    call "!INSTALL_PATH!\install\inied.bat" set "!INSTALL_PATH!\install\.baseval.ini" "Hostname" "!FULL_HOSTNAME!" "" "default"
)

:: Update log settings in .baseval.ini
call "!INSTALL_PATH!\install\inied.bat" set "!INSTALL_PATH!\install\.baseval.ini" "LogType" "file" "" "default"
call "!INSTALL_PATH!\install\inied.bat" set "!INSTALL_PATH!\install\.baseval.ini" "LogFile" "!LOGFILEPATH!" "" "default"

:: Note: Aggplugin paths use %%INSTALL_PATH%% macro in .baseval.ini
:: They will be expanded during 'proc' operation below

:: Determine which config file to process
set "CONF_FILE=!INSTALL_PATH!\conf\zabbix_agentd.conf"
if "!AGENT_VERSION!"=="2" set "CONF_FILE=!INSTALL_PATH!\conf\zabbix_agent2.conf"

:: Process .baseval.ini and apply to agent config (use install/ folder location)
:: Pass INSTALL_PATH macro so %%INSTALL_PATH%% gets expanded correctly
call "!INSTALL_PATH!\install\inied.bat" proc "!INSTALL_PATH!\install\.baseval.ini" "!CONF_FILE!" "default" "##::" "" "INSTALL_PATH=!INSTALL_PATH!"
popd

:: Install and start service
echo Installing Windows service...
set "EXE=!INSTALL_PATH!\bin\zabbix_agentd.exe"
set "SERVICE_NAME=Zabbix Agent"
if "!AGENT_VERSION!"=="2" (
    set "EXE=!INSTALL_PATH!\bin\zabbix_agent2.exe"
    set "SERVICE_NAME=Zabbix Agent 2"
)

"!EXE!" --config "!CONF_FILE!" --install

if "!AUTO_START!"=="1" (
    echo Starting service...
    net start "!SERVICE_NAME!"
)

echo.
echo ================================================
echo Installation completed successfully!
echo ================================================
echo Service: !SERVICE_NAME!
echo Configuration: !CONF_FILE!
echo Logs: !LOGFILEPATH!
echo ================================================
exit /b 0


::color 07
::Value	Color
::0	Black
::1	Blue
::2	Green
::3	Aqua (Cyan)
::4	Red
::5	Purple
::6	Yellow
::7	White (Gray)
::8	Gray (Dark)
::9	Light Blue
::A	Light Green
::B	Light Aqua
::C	Light Red
::D	Light Purple
::E	Light Yellow
::F	Bright White
