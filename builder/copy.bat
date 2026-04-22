@echo off
::: 从 Qt 目录手动复制 DLL 到 out\windows\
::: 注意：本脚本在 builder/ 目录下

::: 切换到项目根目录
cd /d "%~dp0\.."

set OUT_DIR=out\windows

::: 确保 out\windows 目录存在
if not exist %OUT_DIR% mkdir %OUT_DIR%

::: 复制核心 DLL
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Core.dll %OUT_DIR%\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Gui.dll %OUT_DIR%\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Widgets.dll %OUT_DIR%\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Network.dll %OUT_DIR%\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Multimedia.dll %OUT_DIR%\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5MultimediaWidgets.dll %OUT_DIR%\

::: 复制平台插件
mkdir %OUT_DIR%\platforms 2>nul
copy C:\Qt\5.15.2\mingw81_32\plugins\platforms\qwindows.dll %OUT_DIR%\platforms\

::: 复制样式插件
mkdir %OUT_DIR%\styles 2>nul
copy C:\Qt\5.15.2\mingw81_32\plugins\styles\qwindowsvistastyle.dll %OUT_DIR%\styles\

::: 复制图片格式插件
mkdir %OUT_DIR%\imageformats 2>nul
copy C:\Qt\5.15.2\mingw81_32\plugins\imageformats\*.dll %OUT_DIR%\imageformats\

::: 复制音频插件
mkdir %OUT_DIR%\audio 2>nul
copy C:\Qt\5.15.2\mingw81_32\plugins\audio\*.dll %OUT_DIR%\audio\

::: 复制媒体服务插件
mkdir %OUT_DIR%\mediaservice 2>nul
copy C:\Qt\5.15.2\mingw81_32\plugins\mediaservice\*.dll %OUT_DIR%\mediaservice\

::: 复制播放列表格式插件
mkdir %OUT_DIR%\playlistformats 2>nul
copy C:\Qt\5.15.2\mingw81_32\plugins\playlistformats\*.dll %OUT_DIR%\playlistformats\

::: 复制 MinGW 运行时
copy C:\Qt\Tools\mingw810_32\bin\libgcc_s_dw2-1.dll %OUT_DIR%\
copy C:\Qt\Tools\mingw810_32\bin\libstdc++-6.dll %OUT_DIR%\
copy C:\Qt\Tools\mingw810_32\bin\libwinpthread-1.dll %OUT_DIR%\

echo.
echo [DONE] All DLLs copied to %OUT_DIR%\
