@echo off
setlocal

::: ============================================================
:::  DualRecordService - Windows Build + Package Script
:::  Qt 5.15.2 MinGW 32-bit + NSIS Installer
:::  注意：本脚本在 builder/ 目录下，需先切回项目根目录
:::
:::  产物布局：
:::    out/windows/  — 编译产物（exe + DLL + 插件）
:::    out/build/    — 编译中间文件（obj/moc/rcc/uic）
:::    release/      — 最终安装包（.exe）
::: ============================================================

::: --- Environment Setup ---
set PATH=C:\Qt\5.15.2\mingw81_32\bin;C:\Qt\Tools\mingw810_32\bin;%PATH%

set QT_DIR=C:\Qt\5.15.2\mingw81_32
set QT_BIN=%QT_DIR%\bin
set EXE_NAME=DualRecordService.exe
set NSIS_PATH="C:\Program Files (x86)\NSIS\makensis.exe"
set OUT_DIR=out\windows
set BUILD_DIR=out\build
set RELEASE_DIR=release

::: --- Switch to project root directory ---
cd /d "%~dp0\.."
echo [INFO] Project root: %cd%

::: ============================================================
::: Phase 1: Build
::: ============================================================
echo.
echo ========================================================
echo  Phase 1: Building DualRecordService (Release)
echo ========================================================

::: --- Clean old build artifacts (avoid format mismatch) ---
if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
if exist Makefile del Makefile
if exist Makefile.Debug del Makefile.Debug
if exist Makefile.Release del Makefile.Release

::: --- Ensure build output directories exist ---
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %BUILD_DIR%\obj mkdir %BUILD_DIR%\obj
if not exist %BUILD_DIR%\moc mkdir %BUILD_DIR%\moc
if not exist %BUILD_DIR%\uic mkdir %BUILD_DIR%\uic
if not exist %BUILD_DIR%\rcc mkdir %BUILD_DIR%\rcc

::: --- Run qmake ---
echo [1/4] Running qmake...
%QT_BIN%\qmake.exe DualRecordService.pro
if %errorlevel% neq 0 (
    echo [ERROR] qmake failed!
    exit /b 1
)

::: --- Build Release ---
echo [2/4] Compiling...
mingw32-make release -j4
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

if not exist %OUT_DIR%\%EXE_NAME% (
    echo [ERROR] %OUT_DIR%\%EXE_NAME% not found!
    exit /b 1
)

echo [SUCCESS] Build completed: %OUT_DIR%\%EXE_NAME%

::: ============================================================
::: Phase 2: Deploy (windeployqt into out/windows/)
::: ============================================================
echo.
echo ========================================================
echo  Phase 2: Deploying Qt Dependencies into %OUT_DIR%\
echo ========================================================

echo [3/4] Running windeployqt...
%QT_BIN%\windeployqt.exe --release --no-translations --no-opengl-sw %OUT_DIR%\%EXE_NAME%
if %errorlevel% neq 0 (
    echo [WARNING] windeployqt had warnings, continuing...
)

::: Verify Qt5Core.dll exists
if not exist %OUT_DIR%\Qt5Core.dll (
    echo [ERROR] windeployqt failed - Qt5Core.dll not found in %OUT_DIR%\
    echo [INFO]    Try running manually: %QT_BIN%\windeployqt.exe --release --dir %OUT_DIR% %OUT_DIR%\%EXE_NAME%
    exit /b 1
)

echo [SUCCESS] Dependencies deployed to %OUT_DIR%\

::: ============================================================
::: Phase 3: Package (NSIS)
::: ============================================================
echo.
echo ========================================================
echo  Phase 3: Creating NSIS Installer
echo ========================================================

::: --- Ensure release directory exists ---
if not exist %RELEASE_DIR% mkdir %RELEASE_DIR%

::: --- Prepare NSIS input: copy out/windows/ contents to installer/bin/ ---
echo [4/4] Preparing NSIS input...
if exist installer\bin rmdir /s /q installer\bin
mkdir installer\bin
xcopy /E /I /Y %OUT_DIR%\* installer\bin\ >nul
if not exist installer\docs mkdir installer\docs
if not exist installer\docs\LICENSE.txt copy docs\LICENSE.txt installer\docs\ >nul 2>nul

::: --- Check NSIS ---
if not exist %NSIS_PATH% (
    echo [WARNING] NSIS not found at %NSIS_PATH%
    echo [INFO]    installer\bin\ directory is ready with all dependencies.
    echo [INFO]    Command: cd installer ^&^& makensis.exe install_windows.nsi
    goto :done
)

::: --- Run NSIS (from installer/ directory) ---
cd installer
%NSIS_PATH% install_windows.nsi
if %errorlevel% neq 0 (
    echo [ERROR] NSIS packaging failed!
    exit /b 1
)

cd ..

echo.
echo ========================================================
echo  ALL DONE!
echo ========================================================
echo  Build:   %OUT_DIR%\%EXE_NAME%
echo  Release: %RELEASE_DIR%\BankDualRecord_Setup_v1.0.0_x86.exe
echo ========================================================

:done
endlocal
