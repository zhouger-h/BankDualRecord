/**
 * @file main.cpp
 * @brief 银行柜面双录控件 - 主程序入口
 *        单实例保护 + 系统托盘 + TCP服务启动
 */

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QSharedMemory>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>
#include <QTimer>
#include <QImage>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

#ifdef Q_OS_LINUX
#  include <gst/gst.h>
#  include <gst/video/videooverlay.h>
#endif

#include "DualRecordWidget.h"
#include "TcpServer.h"
#include "HttpServer.h"
#include "UploadManager.h"
#include "InitConfigDialog.h"
#include "AppLogger.h"

// X11 头文件必须放在所有 Qt 头文件之后（X11 定义了 Status 等宏会与 Qt 冲突）
#ifdef Q_OS_LINUX
#  include <X11/Xlib.h>
#  include <X11/Xatom.h>
#endif

static const char *APP_KEY = "BankDualRecord_SingletonKey_v1";

// ─────────────────────────────────────────
// Linux: 通过 _NET_WM_ICON X11 属性设置窗口标题栏图标
// Kylin/UKUI 桌面不认 setWindowIcon，必须直接设置 X11 属性
// ─────────────────────────────────────────
#ifdef Q_OS_LINUX
static void setX11WindowIcon(QWidget *widget, const QString &iconPath)
{
    QImage img(iconPath);
    if (img.isNull()) return;
    img = img.convertToFormat(QImage::Format_ARGB32);

    const int w = img.width(), h = img.height();
    const long nPixels = w * h;
    const long dataLen = 2 + nPixels;
    QVector<unsigned long> data(dataLen);
    data[0] = static_cast<unsigned long>(w);
    data[1] = static_cast<unsigned long>(h);

    for (long i = 0; i < nPixels; ++i) {
        const QRgb c = reinterpret_cast<const QRgb *>(img.constBits())[i];
        data[2 + i] = (static_cast<unsigned long>(qAlpha(c)) << 24)
                    | (static_cast<unsigned long>(qRed(c))   << 16)
                    | (static_cast<unsigned long>(qGreen(c)) << 8)
                    |  static_cast<unsigned long>(qBlue(c));
    }

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return;
    Atom netWmIcon = XInternAtom(dpy, "_NET_WM_ICON", False);
    Atom cardinal   = XInternAtom(dpy, "CARDINAL",   False);
    XChangeProperty(dpy, static_cast<Window>(widget->winId()),
                    netWmIcon, cardinal, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char *>(data.constData()),
                    dataLen);
    XFlush(dpy);
    XCloseDisplay(dpy);
}
#endif

// ─────────────────────────────────────────
// 单实例保护
// ─────────────────────────────────────────
class SingletonGuard
{
public:
    SingletonGuard() : m_mem(APP_KEY) {}

    bool isAlreadyRunning()
    {
        if (m_mem.attach()) {
            m_mem.detach();
            return true;
        }
        return !m_mem.create(1);
    }

private:
    QSharedMemory m_mem;
};

// ─────────────────────────────────────────
// 主程序
// ─────────────────────────────────────────
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // ── Linux GStreamer 初始化（v2.9-fix9） ──
    // 必须在使用任何 GStreamer API 之前调用 gst_init()
    // 使用 GstRecorder（C API 封装）需要 GStreamer 库已初始化
#ifdef Q_OS_LINUX
    gst_init(&argc, &argv);
#endif

    // ── Linux GStreamer 诊断 ──
    // v2.9-fix7: GST_DEBUG 输出到 /tmp/gst_debug.log（保留，方便排查问题）
    // 级别说明: 2=ERROR/WARN, 3=INFO, 4=DEBUG(非常详细), 5=LOG
#ifdef Q_OS_LINUX
    qputenv("GST_DEBUG", "2");
    qputenv("GST_DEBUG_FILE", "/tmp/gst_debug.log");
#endif
    app.setApplicationName("BankDualRecord");
    app.setApplicationDisplayName("银行柜面双录系统");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("BankSystem");
    app.setQuitOnLastWindowClosed(false); // 关闭窗口不退出（托盘继续运行）
    // ── Linux: 让窗口管理器通过 .desktop 文件匹配图标（标题栏图标） ──
    // Kylin/UKUI 桌面不认 setWindowIcon，必须通过 DesktopFileName 关联
    app.setDesktopFileName("dualrecord.desktop");

    // ── 单实例检测 ──
    // 已有实例在运行时静默退出（不弹窗）
    // 窗口显示/隐藏由外部通过 TCP SHOW_WINDOW/HIDE_WINDOW 命令控制，
    // 不应在用户隐藏窗口后再次启动时弹出任何提示
    SingletonGuard guard;
    if (guard.isAlreadyRunning()) {
        return 0;
    }

    // ── 首次运行 - 检查配置文件，未配置则弹出初始化向导 ──
    QSettings appSettings("BankSystem", "DualRecord");
    bool configured = appSettings.value("app/configured", false).toBool();
    QString configFilePath = QDir::homePath() + "/BankDualRecord/config.ini";

    if (!configured || !QFile::exists(configFilePath)) {
        InitConfigDialog dlg;
        if (dlg.exec() != QDialog::Accepted) {
            return 0; // 用户取消
        }
        appSettings.setValue("app/configured", true);
        configFilePath = dlg.configFilePath();
    }

    // ── 初始化日志系统 ──
    AppLogger::init(configFilePath);

    // ── 核心对象创建 ──
    DualRecordWidget *mainWidget   = new DualRecordWidget;
    TcpCommandServer *tcpServer    = new TcpCommandServer(mainWidget);
    HttpCommandServer *httpServer  = new HttpCommandServer(tcpServer);
    UploadManager    *uploadMgr    = new UploadManager;

    // 初始化配置
    mainWidget->initConfig(configFilePath);
    uploadMgr->loadConfig(configFilePath);
    uploadMgr->resumePendingUploads(); // 恢复未完成上传

    // 连接上传信号（uploadQueued 只有 localPath，enqueue 需要额外的 taskId，用 lambda 适配）
    QObject::connect(mainWidget, &DualRecordWidget::uploadQueued,
                     [uploadMgr](const QString &localPath) {
                         uploadMgr->enqueue(localPath);
                     });

    // ── 启动TCP服务 ──
    QSettings cfg(configFilePath, QSettings::IniFormat);
    quint16 tcpPort  = static_cast<quint16>(cfg.value("server/tcp_port",  9527).toInt());
    quint16 httpPort = static_cast<quint16>(cfg.value("server/http_port", 9528).toInt());

    if (!tcpServer->start(tcpPort)) {
        QMessageBox::critical(nullptr, "双录服务",
            QString("TCP服务启动失败，端口 %1 可能被占用。").arg(tcpPort));
        return 1;
    }

    // ── 启动HTTP服务 ──
    if (!httpServer->start(httpPort)) {
        QMessageBox::critical(nullptr, "双录服务",
            QString("HTTP服务启动失败，端口 %1 可能被占用。").arg(httpPort));
        return 1;
    }

    // ── 将录制事件广播到 HTTP SSE 客户端 ──
    QObject::connect(mainWidget, &DualRecordWidget::recordStarted,
                     [httpServer](const QString &taskId, const QString &filePath) {
                         httpServer->broadcastSseEvent("RECORD_STARTED",
                             {{"task_id", taskId}, {"file_path", filePath}});
                     });
    QObject::connect(mainWidget, &DualRecordWidget::recordStopped,
                     [httpServer](const QString &taskId, const QString &filePath) {
                         httpServer->broadcastSseEvent("RECORD_STOPPED",
                             {{"task_id", taskId}, {"file_path", filePath}});
                     });
    QObject::connect(mainWidget, &DualRecordWidget::recordError,
                     [httpServer](const QString &taskId, const QString &errorMsg) {
                         httpServer->broadcastSseEvent("RECORD_ERROR",
                             {{"task_id", taskId}, {"error", errorMsg}});
                     });
    QObject::connect(mainWidget, &DualRecordWidget::uploadQueued,
                     [httpServer](const QString &filePath) {
                         httpServer->broadcastSseEvent("UPLOAD_QUEUED",
                             {{"file_path", filePath}});
                     });

    // 上传完成事件
    QObject::connect(uploadMgr, &UploadManager::uploadSuccess,
                     [httpServer](const QString &localPath) {
                         httpServer->broadcastSseEvent("UPLOAD_SUCCESS",
                             {{"file_path", localPath}});
                     });
    QObject::connect(uploadMgr, &UploadManager::uploadFailed,
                     [httpServer](const QString &localPath, const QString &error) {
                         httpServer->broadcastSseEvent("UPLOAD_FAILED",
                             {{"file_path", localPath}, {"error", error}});
                     });
    QObject::connect(mainWidget, &DualRecordWidget::windowShown,
                     [httpServer]() {
                         httpServer->broadcastSseEvent("WINDOW_SHOWN", {});
                     });
    QObject::connect(mainWidget, &DualRecordWidget::windowHidden,
                     [httpServer]() {
                         httpServer->broadcastSseEvent("WINDOW_HIDDEN", {});
                     });

    // ── 全局窗口图标 + 系统托盘 ──
    // 使用 icons/png/ 下的自定义图标
    QString appDirPath = QCoreApplication::applicationDirPath();
    QIcon appIcon;
    QString foundIconPath;  // 记录找到的图标路径，用于 X11 _NET_WM_ICON
    // 搜索路径：deb 安装后的系统路径 / opt/dualrecord / 开发目录
    QStringList iconSearchPaths = {
        "/usr/share/icons/hicolor/256x256/apps/dualrecord.png",   // deb 安装后的 freedesktop 路径
        "/opt/dualrecord/share/icons/256x256.png",                  // deb 安装后的应用专属路径
        appDirPath + "/../share/icons/hicolor/256x256/apps/dualrecord.png",
        appDirPath + "/../icons/png/256x256.png",
        appDirPath + "/icons/png/256x256.png",
    };
    for (const QString &iconPath : iconSearchPaths) {
        if (QFile::exists(iconPath)) {
            appIcon = QIcon(iconPath);
            foundIconPath = iconPath;
            break;
        }
    }
    if (appIcon.isNull()) {
        // 回退到 Qt 内置图标
        appIcon = QIcon::fromTheme("camera-video",
            app.style()->standardIcon(QStyle::SP_MediaPlay));
    }
    app.setWindowIcon(appIcon);

    QSystemTrayIcon *tray = new QSystemTrayIcon(appIcon, &app);

    QMenu *trayMenu = new QMenu;
    QAction *actShow   = trayMenu->addAction("显示录制界面");
    QAction *actStatus = trayMenu->addAction(
        QString("TCP:%1  |  HTTP:%2").arg(tcpPort).arg(httpPort));
    actStatus->setEnabled(false);
    trayMenu->addSeparator();
    QAction *actQuit   = trayMenu->addAction("退出双录服务");

    tray->setContextMenu(trayMenu);
    tray->setToolTip(QString("银行双录服务 | TCP:%1 | HTTP:%2").arg(tcpPort).arg(httpPort));
    tray->show();

    QObject::connect(actShow, &QAction::triggered, [&](){
        mainWidget->show();
        mainWidget->raise();
        mainWidget->activateWindow();
    });
    QObject::connect(actQuit, &QAction::triggered, [&](){
        tray->hide();
        AppLogger::shutdown();
        delete httpServer;
        delete tcpServer;
        delete uploadMgr;
        delete mainWidget;
        app.quit();
    });
    QObject::connect(tray, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason r){
        if (r == QSystemTrayIcon::DoubleClick) {
            mainWidget->show();
            mainWidget->raise();
        }
    });

    // 显示主界面
    mainWidget->show();

#ifdef Q_OS_LINUX
    // X11: 直接通过 _NET_WM_ICON 设置标题栏图标（Kylin/UKUI 桌面兼容）
    if (!foundIconPath.isEmpty()) {
        QTimer::singleShot(100, mainWidget, [mainWidget, foundIconPath]() {
            setX11WindowIcon(mainWidget, foundIconPath);
        });
    }
#endif

    return app.exec();
}
