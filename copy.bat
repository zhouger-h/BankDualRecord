cd C:\Users\ark\Documents\mycode\BankDualRecord

:: 复制核心 DLL
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Core.dll release\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Gui.dll release\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Widgets.dll release\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Network.dll release\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5Multimedia.dll release\
copy C:\Qt\5.15.2\mingw81_32\bin\Qt5MultimediaWidgets.dll release\

:: 复制平台插件
mkdir release\platforms
copy C:\Qt\5.15.2\mingw81_32\plugins\platforms\qwindows.dll release\platforms\

:: 复制样式插件
mkdir release\styles
copy C:\Qt\5.15.2\mingw81_32\plugins\styles\qwindowsvistastyle.dll release\styles\

:: 复制图片格式插件
mkdir release\imageformats
copy C:\Qt\5.15.2\mingw81_32\plugins\imageformats\*.dll release\imageformats\

:: 复制音频插件
mkdir release\audio
copy C:\Qt\5.15.2\mingw81_32\plugins\audio\*.dll release\audio\

:: 复制媒体服务插件
mkdir release\mediaservice
copy C:\Qt\5.15.2\mingw81_32\plugins\mediaservice\*.dll release\mediaservice\

:: 复制播放列表格式插件
mkdir release\playlistformats
copy C:\Qt\5.15.2\mingw81_32\plugins\playlistformats\*.dll release\playlistformats\

:: 复制 MinGW 运行时
copy C:\Qt\Tools\mingw810_32\bin\libgcc_s_dw2-1.dll release\
copy C:\Qt\Tools\mingw810_32\bin\libstdc++-6.dll release\
copy C:\Qt\Tools\mingw810_32\bin\libwinpthread-1.dll release\
