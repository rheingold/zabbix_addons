@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Test 3: Bootstrap Cycle Prevention
echo ========================================
echo.

set "TEST_COUNT=0"
set "PASS_COUNT=0"

REM Create test files
mkdir test-work 2>nul
cd test-work

echo [default]> baseval-bootstrap.ini
echo LogFile=%%INSTALL_PATH%%\log\agent.log.txt>> baseval-bootstrap.ini
echo.>> baseval-bootstrap.ini

REM Test 1: Initial state has 2 percent signs
set /a TEST_COUNT+=1
findstr /C:"%%%%INSTALL_PATH%%%%" baseval-bootstrap.ini >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 1: Initial state has %%%%INSTALL_PATH%%%%
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 1: Initial state does not have correct pattern
)

REM Simulate 6 consecutive SET operations
for /L %%i in (1,1,6) do (
    call ..\install\inied.bat set "baseval-bootstrap.ini" "TestKey%%i" "TestValue%%i" "##::" "default"
)

REM Test 2: After 6 SET calls, should still be 2 percent signs
set /a TEST_COUNT+=1
findstr /C:"%%%%INSTALL_PATH%%%%" baseval-bootstrap.ini >nul
if %ERRORLEVEL%==0 (
    echo [PASS] Test 2: Still %%%%INSTALL_PATH%%%% after 6 SET calls
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 2: Percent signs changed after SET calls
)

REM Test 3: Should NOT have 4 or 8 percent signs (explosion check)
set /a TEST_COUNT+=1
findstr /C:"%%%%%%%%" baseval-bootstrap.ini >nul
if %ERRORLEVEL%==1 (
    echo [PASS] Test 3: No percent sign explosion detected
    set /a PASS_COUNT+=1
) else (
    echo [FAIL] Test 3: Percent sign explosion occurred
)

cd ..
rmdir /s /q test-work 2>nul

echo.
echo ========================================
echo Test Results: %PASS_COUNT%/%TEST_COUNT% passed
echo ========================================
