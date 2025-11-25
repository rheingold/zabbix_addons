@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Test 2: Auto Mode Macro Expansion
echo ========================================
echo.

set "TEST_COUNT=0"
set "PASS_COUNT=0"

REM Create test files
mkdir test-work 2>nul
cd test-work

REM Create baseval with macro section
echo [default]> baseval-auto.ini
echo LogFile=%%INSTALL_PATH%%\log\agent2\agent.log.txt>> baseval-auto.ini
echo ServerActive=%%SERVER_ADDRESS%%>> baseval-auto.ini
echo.>> baseval-auto.ini
echo [inied.bat-macros]>> baseval-auto.ini
echo INSTALL_PATH=C:\Zabbix>> baseval-auto.ini
echo SERVER_ADDRESS=zabbix.example.com>> baseval-auto.ini

REM Test 1: Auto mode reads from section
set /a TEST_COUNT+=1
call ..\install\inied.bat proc "baseval-auto.ini" "config-auto.conf" "default" "" "" "auto"

findstr /C:"C:\Zabbix\log\agent2\agent.log.txt" config-auto.conf >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 1: Auto mode expanded INSTALL_PATH
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 1: Auto mode did not expand INSTALL_PATH
)

REM Test 2: Multiple macros expanded
set /a TEST_COUNT+=1
findstr /C:"zabbix.example.com" config-auto.conf >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 2: Auto mode expanded SERVER_ADDRESS
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 2: Auto mode did not expand SERVER_ADDRESS
)

REM Test 3: Macro section preserved in source
set /a TEST_COUNT+=1
findstr /C:"[inied.bat-macros]" baseval-auto.ini >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 3: Macro section preserved in source
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 3: Macro section removed from source
)

REM Test 4: No unexpanded macros in output
set /a TEST_COUNT+=1
findstr /C:"%%" config-auto.conf >nul
if %ERRORLEVEL%==1 (
    echo [PASS] Test 4: No unexpanded macros in output
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 4: Unexpanded macros found in output
)

cd ..
rmdir /s /q test-work 2>nul

echo.
echo ========================================
echo Test Results: %PASS_COUNT%/%TEST_COUNT% passed
echo ========================================
