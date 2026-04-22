@echo off
setlocal

:: ============================================================
::  DualRecordService - Windows Build + Package Script
::  Qt 5.15.2 MinGW 32-bit + NSIS Installer
:: ============================================================

:: --- Environment Setup ---
set PATH=C:\Qt\5.15.2\mingw81_32\bin;C:\Qt\Tools\mingw810_32\bin;%PATH%

set QT_DIR=C:\Qt\5.15.2\mingw81_32
set QT_BIN=%QT_DIR%\bin
set EXE_NAME=DualRecordService.exe
set NSIS_PATH="C:\Program Files (x86)\NSIS\makensis.exe"

:: ============================================================
:: Phase 1: Build
:: ============================================================
echo.
echo ========================================================
echo  Phase 1: Building DualRecordService (Release)
echo ========================================================

:: --- Ensure resources directory ---
if not exist resources mkdir resources
if not exist resources\app.ico copy icons\win\icon.ico resources\app.ico >nul 2>nul

:: --- Run qmake ---
echo [1/4] Running qmake...
%QT_BIN%\qmake.exe DualRecordService.pro
if %errorlevel% neq 0 (
    echo [ERROR] qmake failed!
    exit /b 1
)

:: --- Ensure build output directories exist ---
if not exist build mkdir build
if not exist build\obj mkdir build\obj
if not exist build\moc mkdir build\moc
if not exist build\uic mkdir build\uic
if not exist build\rcc mkdir build\rcc

:: --- Build Release ---
echo [2/4] Compiling...
mingw32-make release -j4
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

if not exist release\%EXE_NAME% (
    echo [ERROR] release\%EXE_NAME% not found!
    exit /b 1
)

echo [SUCCESS] Build completed: release\%EXE_NAME%

:: ============================================================
:: Phase 2: Deploy (windeployqt)
:: ============================================================
echo.
echo ========================================================
echo  Phase 2: Deploying Qt Dependencies
echo ========================================================

:: --- Clean old bin/ directory ---
if exist bin rmdir /s /q bin
mkdir bin

:: --- Copy EXE to bin/ ---
echo [3/4] Copying EXE to bin\...
copy /y release\%EXE_NAME% bin\ >nul

:: --- Run windeployqt to collect all Qt dependencies ---
echo       Running windeployqt...
%QT_BIN%\windeployqt.exe --release --no-translations --no-opengl-sw bin\%EXE_NAME%
if %errorlevel% neq 0 (
    echo [WARNING] windeployqt had warnings, continuing...
)

:: ============================================================
:: Phase 3: Package (NSIS)
:: ============================================================
echo.
echo ========================================================
echo  Phase 3: Creating NSIS Installer
echo ========================================================

:: --- Check NSIS ---
if not exist %NSIS_PATH% (
    echo [WARNING] NSIS not found at %NSIS_PATH%
    echo [INFO]    bin\ directory is ready. You can run NSIS manually.
    echo [INFO]    Command: makensis.exe installer\install_windows.nsi
    goto :done
)

:: --- Run NSIS ---
echo [4/4] Running NSIS...
%NSIS_PATH% installer\install_windows.nsi
if %errorlevel% neq 0 (
    echo [ERROR] NSIS packaging failed!
    exit /b 1
)

echo.
echo ========================================================
echo  ALL DONE!
echo ========================================================
echo  Installer: BankDualRecord_Setup_v1.0.0_x86.exe
echo ========================================================

:done
endlocal
