@echo off
setlocal

:: ============================================================
::  DualRecordService - Windows Build Script (Qt 5.15.2 MinGW 32-bit)
:: ============================================================

:: --- Environment Setup ---
set PATH=C:\Qt\5.15.2\mingw81_32\bin;C:\Qt\Tools\mingw810_32\bin;%PATH%

:: --- Ensure resources directory ---
if not exist resources mkdir resources
if not exist resources\app.ico copy icons\win\icon.ico resources\app.ico >nul 2>nul

:: --- Run qmake ---
C:\Qt\5.15.2\mingw81_32\bin\qmake.exe DualRecordService.pro
if %errorlevel% neq 0 (
    echo [ERROR] qmake failed!
    exit /b 1
)

:: --- Ensure build output directories exist ---
if not exist build mkdir build
if not exist build\obj mkdir build\obj
if not exist build\moc mkdir build\moc
if not exist build\uic mkdir build\uic

:: --- Build Release ---
mingw32-make release -j4
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo [SUCCESS] Build completed. Output: release\DualRecordService.exe
endlocal
