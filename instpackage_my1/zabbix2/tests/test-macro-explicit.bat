@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Test 1: Explicit Macro Map Passing
echo ========================================
echo.

set "TEST_COUNT=0"
set "PASS_COUNT=0"

REM Create test files
mkdir test-work 2>nul
cd test-work

echo [default]> baseval-test.ini
echo LogFile=%%INSTALL_PATH%%\log\agent2\agent.log.txt>> baseval-test.ini
echo Hostname=Customer-%%HOSTNAME%%>> baseval-test.ini
echo.>> baseval-test.ini

REM Test 1: Explicit macro map with INSTALL_PATH
set /a TEST_COUNT+=1
call ..\install\inied.bat proc "baseval-test.ini" "config-test.conf" "default" "" "" "INSTALL_PATH=C:\Zabbix|HOSTNAME=TestHost"

findstr /C:"C:\Zabbix\log\agent2\agent.log.txt" config-test.conf >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 1: LogFile macro expanded correctly
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 1: LogFile macro not expanded
)

REM Test 2: Hostname expansion
set /a TEST_COUNT+=1
findstr /C:"Customer-TestHost" config-test.conf >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 2: Hostname macro expanded correctly
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 2: Hostname macro not expanded
)

REM Test 3: Source file should still have unexpanded macros
set /a TEST_COUNT+=1
findstr /C:"%%INSTALL_PATH%%" baseval-test.ini >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 3: Source file preserved with unexpanded macros
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 3: Source file macros were modified
)

REM Test 4: No unexpanded macros in output
set /a TEST_COUNT+=1
findstr /C:"%%" config-test.conf >nul
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
