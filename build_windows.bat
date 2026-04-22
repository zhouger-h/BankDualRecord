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

:: --- Clean old build artifacts (avoid format mismatch) ---
if exist build rmdir /s /q build
if exist Makefile del Makefile
if exist Makefile.Debug del Makefile.Debug
if exist Makefile.Release del Makefile.Release

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
:: Phase 2: Deploy (windeployqt into release/)
:: ============================================================
echo.
echo ========================================================
echo  Phase 2: Deploying Qt Dependencies into release\
echo ========================================================

echo [3/4] Running windeployqt...
%QT_BIN%\windeployqt.exe --release --no-translations --no-opengl-sw release\%EXE_NAME%
if %errorlevel% neq 0 (
    echo [WARNING] windeployqt had warnings, continuing...
)

:: Verify Qt5Core.dll exists
if not exist release\Qt5Core.dll (
    echo [ERROR] windeployqt failed - Qt5Core.dll not found in release\
    echo [INFO]    Try running manually: %QT_BIN%\windeployqt.exe --release --dir release release\%EXE_NAME%
    exit /b 1
)

echo [SUCCESS] Dependencies deployed to release\

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
    echo [INFO]    release\ directory is ready with all dependencies.
    echo [INFO]    Command: makensis.exe install_windows.nsi
    goto :done
)

:: --- Run NSIS ---
echo [4/4] Running NSIS...
%NSIS_PATH% /DDEPLOY_DIR=release install_windows.nsi
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
