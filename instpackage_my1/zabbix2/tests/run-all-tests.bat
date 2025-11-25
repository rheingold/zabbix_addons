@echo off
setlocal enabledelayedexpansion

echo ========================================
echo   Zabbix Installer - Full Test Suite
echo ========================================
echo.

set "TOTAL_TESTS=0"
set "PASSED_TESTS=0"
set "FAILED_TESTS=0"

REM Test 1: Explicit Macro Map
echo.
echo [TEST 1/4] Explicit Macro Map Expansion
echo ----------------------------------------
call test-macro-explicit.bat > test-results-1.txt 2>&1
findstr /C:"[PASS]" test-results-1.txt | find /C "[PASS]" > temp-count.txt
set /p PASS_COUNT=<temp-count.txt
findstr /C:"[FAIL]" test-results-1.txt | find /C "[FAIL]" > temp-count.txt
set /p FAIL_COUNT=<temp-count.txt
set /a TOTAL_TESTS+=!PASS_COUNT!+!FAIL_COUNT!
set /a PASSED_TESTS+=!PASS_COUNT!
set /a FAILED_TESTS+=!FAIL_COUNT!
if !FAIL_COUNT! EQU 0 (
    echo Result: PASS ^(!PASS_COUNT!/!PASS_COUNT! tests^)
) else (
    echo Result: FAIL ^(!PASS_COUNT! passed, !FAIL_COUNT! failed^)
)

REM Test 2: Auto Mode
echo.
echo [TEST 2/4] Auto Mode ^(read from [inied.bat-macros]^)
echo ----------------------------------------
call test-macro-auto.bat > test-results-2.txt 2>&1
findstr /C:"[PASS]" test-results-2.txt | find /C "[PASS]" > temp-count.txt
set /p PASS_COUNT=<temp-count.txt
findstr /C:"[FAIL]" test-results-2.txt | find /C "[FAIL]" > temp-count.txt
set /p FAIL_COUNT=<temp-count.txt
set /a TOTAL_TESTS+=!PASS_COUNT!+!FAIL_COUNT!
set /a PASSED_TESTS+=!PASS_COUNT!
set /a FAILED_TESTS+=!FAIL_COUNT!
if !FAIL_COUNT! EQU 0 (
    echo Result: PASS ^(!PASS_COUNT!/!PASS_COUNT! tests^)
) else (
    echo Result: FAIL ^(!PASS_COUNT! passed, !FAIL_COUNT! failed^)
)

REM Test 3: Bootstrap Cycles
echo.
echo [TEST 3/4] Bootstrap SET Cycles ^(no %% explosion^)
echo ----------------------------------------
call test-bootstrap-cycles.bat > test-results-3.txt 2>&1
findstr /C:"[PASS]" test-results-3.txt | find /C "[PASS]" > temp-count.txt
set /p PASS_COUNT=<temp-count.txt
findstr /C:"[FAIL]" test-results-3.txt | find /C "[FAIL]" > temp-count.txt
set /p FAIL_COUNT=<temp-count.txt
set /a TOTAL_TESTS+=!PASS_COUNT!+!FAIL_COUNT!
set /a PASSED_TESTS+=!PASS_COUNT!
set /a FAILED_TESTS+=!FAIL_COUNT!
if !FAIL_COUNT! EQU 0 (
    echo Result: PASS ^(!PASS_COUNT!/!PASS_COUNT! tests^)
) else (
    echo Result: FAIL ^(!PASS_COUNT! passed, !FAIL_COUNT! failed^)
)

REM Test 4: Full Integration
echo.
echo [TEST 4/4] Full Integration ^(bootstrap to install^)
echo ----------------------------------------
call test-full-integration.bat > test-results-4.txt 2>&1
findstr /C:"[PASS]" test-results-4.txt | find /C "[PASS]" > temp-count.txt
set /p PASS_COUNT=<temp-count.txt
findstr /C:"[FAIL]" test-results-4.txt | find /C "[FAIL]" > temp-count.txt
set /p FAIL_COUNT=<temp-count.txt
set /a TOTAL_TESTS+=!PASS_COUNT!+!FAIL_COUNT!
set /a PASSED_TESTS+=!PASS_COUNT!
set /a FAILED_TESTS+=!FAIL_COUNT!
if !FAIL_COUNT! EQU 0 (
    echo Result: PASS ^(!PASS_COUNT!/!PASS_COUNT! tests^)
) else (
    echo Result: FAIL ^(!PASS_COUNT! passed, !FAIL_COUNT! failed^)
)

REM Clean up temp files
del temp-count.txt 2>nul

echo.
echo ========================================
echo   Test Suite Summary
echo ========================================
echo Total Tests: !TOTAL_TESTS!
echo Passed: !PASSED_TESTS!
echo Failed: !FAILED_TESTS!
echo.
if !FAILED_TESTS! EQU 0 (
    echo Result: ALL TESTS PASSED ✓
    echo.
    echo The macro expansion architecture is ready for deployment!
) else (
    echo Result: SOME TESTS FAILED ✗
    echo.
    echo Check test-results-*.txt files for details.
)
echo ========================================
echo.

REM Return appropriate exit code
if !FAILED_TESTS! EQU 0 (
    exit /b 0
) else (
    exit /b 1
)
