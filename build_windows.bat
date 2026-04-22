@echo off
chcp 65001 >nul
echo ==========================================
echo  银行柜面双录控件 - Windows 32位编译脚本
echo ==========================================
echo.

REM 设置 Qt 5.15.2 MinGW 32-bit 环境变量
set PATH=C:\Qt\5.15.2\mingw81_32\bin;C:\Qt\Tools\mingw810_32\bin;%PATH%

REM 检查 Qt 环境
qmake --version >nul 2>nul
if %errorlevel% neq 0 (
    echo [错误] 未找到 qmake，请确保已安装 Qt 5.15.2 (MinGW 32-bit)
    echo.
    echo 请从以下地址下载 Qt:
    echo https://download.qt.io/archive/qt/5.15/5.15.2/
    echo 安装时选择: MinGW 8.1.0 32-bit
    echo.
    pause
    exit /b 1
)

echo [1/5] 清理旧文件...
if exist release rmdir /s /q release
if exist deploy rmdir /s /q deploy
if exist Makefile del /f Makefile
if exist Makefile.Debug del /f Makefile.Debug
if exist Makefile.Release del /f Makefile.Release

echo [2/5] 生成 Makefile...
qmake DualRecordService.pro
if %errorlevel% neq 0 (
    echo [错误] qmake 执行失败
    pause
    exit /b 1
)

echo [3/5] 编译程序...
mingw32-make release -j4
if %errorlevel% neq 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

echo [4/5] 部署 Qt 依赖...
mkdir deploy 2>nul
copy release\DualRecordService.exe deploy\
windeployqt --release --force --no-translations deploy\DualRecordService.exe

echo [5/5] 打包安装程序...
cd installer
if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
    "C:\Program Files (x86)\NSIS\makensis.exe" install_windows.nsi
    echo.
    echo ==========================================
    echo  编译完成!
    echo ==========================================
    echo.
    echo 输出文件:
    echo   - 主程序: release\DualRecordService.exe
    echo   - 便携版: deploy\ (目录)
    echo   - 安装包: installer\BankDualRecord_Setup_v1.0.0_x86.exe
    echo.
) else (
    echo [警告] 未找到 NSIS，跳过安装包生成
    echo 如需生成安装包，请安装 NSIS: https://nsis.sourceforge.io/
    echo.
    echo ==========================================
    echo  编译完成 (无安装包)
    echo ==========================================
    echo.
    echo 输出文件:
    echo   - 主程序: release\DualRecordService.exe
    echo   - 便携版: deploy\ (目录)
    echo.
)

cd ..
pause
