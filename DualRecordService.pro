QT += core gui widgets network multimedia multimediawidgets

CONFIG += c++17

# 目标名称
TARGET   = DualRecordService
TEMPLATE = app

# 编译产物输出目录：
#   Linux/麒麟 → 项目根目录（make直接产生./DualRecordService）
#   Windows    → release/ 子目录（配合 windeployqt 部署流程）
linux {
    DESTDIR     = $$PWD
}
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR     = $$PWD/build/moc
RCC_DIR     = $$PWD/build/rcc
UI_DIR      = $$PWD/build/ui

# 平台判断
win32 {
    # Windows 32位
    RC_ICONS = resources/app.ico
    LIBS    += -lws2_32
    CONFIG  += win32_use_libssh2
    # 开机自启注册表写入
    DEFINES += Q_OS_WIN_AUTOSTART
}

linux {
    # 信创 麒麟 ARM/x86
    CONFIG  += kylin_compat
    LIBS    += -lssh2 -lssl -lcrypto
    DEFINES += USE_LIBSSH2
    # v2.9-fix9: GStreamer C API（GstRecorder）依赖
    CONFIG  += link_pkgconfig
    PKGCONFIG += gstreamer-1.0 gstreamer-video-1.0
    # X11: 标题栏图标（_NET_WM_ICON），Kylin/UKUI 兼容
    LIBS += -lX11
}

# libssh2 (SFTP支持，可选，若无则降级FTP)
contains(LIBS, -lssh2) {
    DEFINES += USE_LIBSSH2
}

SOURCES += \
    core/main.cpp \
    core/DualRecordWidget.cpp \
    core/GstRecorder.cpp \
    core/PlayerWindow.cpp \
    core/TcpServer.cpp \
    core/HttpServer.cpp \
    core/UploadManager.cpp \
    core/InitConfigDialog.cpp

HEADERS += \
    core/DualRecordWidget.h \
    core/GstRecorder.h \
    core/PlayerWindow.h \
    core/TcpServer.h \
    core/HttpServer.h \
    core/UploadManager.h \
    core/InitConfigDialog.h

# RESOURCES += resources/resources.qrc  # 暂无资源文件，如需图标等资源再启用

# 安装目标
target.path = /opt/dualrecord
INSTALLS   += target
