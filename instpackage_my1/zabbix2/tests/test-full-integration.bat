@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Test 4: Full Integration Flow
echo ========================================
echo.

set "TEST_COUNT=0"
set "PASS_COUNT=0"

REM Create mock installation structure
mkdir test-install 2>nul
cd test-install
mkdir install 2>nul
mkdir conf 2>nul

REM Create baseval template (simulating bootstrap)
echo [default]> install\.baseval.ini
echo LogFile=%%INSTALL_PATH%%\log\agent2\agent.log.txt>> install\.baseval.ini
echo Hostname=%%CUSTOMER_ID%%-%%COMPUTERNAME%%>> install\.baseval.ini
echo ServerActive=%%SERVER_ADDRESS%%>> install\.baseval.ini
echo.>> install\.baseval.ini

REM Step 1: Bootstrap - Apply initial values via SET (simulates index.html)
call ..\install\inied.bat set "install\.baseval.ini" "ServerActive" "zabbix.example.com" "##::" "default"
call ..\install\inied.bat set "install\.baseval.ini" "Hostname" "PDS-Zakazka1-TESTPC" "##::" "default"

REM Step 2: Installation - PROC with macro expansion (simulates install.bat SILENT_INSTALL)
call ..\install\inied.bat proc "install\.baseval.ini" "conf\zabbix_agent2.conf" "default" "##::" "" "INSTALL_PATH=C:\TestZabbix|CUSTOMER_ID=TestCustomer"

REM Test 1: LogFile macro expanded
set /a TEST_COUNT+=1
findstr /C:"C:\TestZabbix\log\agent2\agent.log.txt" conf\zabbix_agent2.conf >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 1: LogFile path expanded with INSTALL_PATH
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 1: LogFile path not expanded correctly
)

REM Test 2: ServerActive set via bootstrap
set /a TEST_COUNT+=1
findstr /C:"zabbix.example.com" conf\zabbix_agent2.conf >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 2: ServerActive from bootstrap applied
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 2: ServerActive not applied
)

REM Test 3: Hostname set via bootstrap
set /a TEST_COUNT+=1
findstr /C:"PDS-Zakazka1-TESTPC" conf\zabbix_agent2.conf >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 3: Hostname from bootstrap applied
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 3: Hostname not applied
)

REM Test 4: No unexpanded macros in config
set /a TEST_COUNT+=1
findstr /C:"%%" conf\zabbix_agent2.conf >nul
if %ERRORLEVEL%==1 (
    echo [PASS] Test 4: No unexpanded macros in final config
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 4: Unexpanded macros found in config
)

REM Test 5: Source baseval still has macros
set /a TEST_COUNT+=1
findstr /C:"%%INSTALL_PATH%%" install\.baseval.ini >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 5: Source baseval.ini preserved macros
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 5: Source baseval.ini macros were expanded
)

REM Test 6: Update tracking present
set /a TEST_COUNT+=1
findstr /C:"##::" conf\zabbix_agent2.conf >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 6: Update tracking markers present
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 6: Update tracking markers not found
)

cd ..
rmdir /s /q test-install 2>nul

echo.
echo ========================================
echo Test Results: %PASS_COUNT%/%TEST_COUNT% passed
echo ========================================
