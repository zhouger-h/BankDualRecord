/**
 * @file DualRecordWidget.cpp
 * @brief 银行柜面双录控件 - 主录制界面实现
 *
 * 行为说明：
 *   - 打开界面时：枚举设备，显示占位图，摄像头不自动开启
 *   - 点「开始录制」：打开摄像头 → 等就绪 → 创建 recorder + audioInput → record()
 *                    → 等 RecordingState 确认（超时5秒告警）→ UI 显示"录制中"
 *   - 点「停止录制」：recorder->stop() → 等 StoppedState 落盘 → closeCamera()
 *                    → 文件入上传队列
 *   - 录制中禁止切换摄像头（避免多文件问题）
 *   - 「配置」按钮随时可打开设置界面
 *
 * 修复要点（v2.9）：
 *   1. Linux 下 recorder 闪录闪停（record() 后 94ms 自动 Stopped+Unloaded）
 *      原因：video/ogg 容器必须搭配 video/x-theora+audio/x-vorbis，
 *      但未显式指定 codec，GStreamer 自动选择了不兼容编码器导致管线静默失败
 *   2. 新增 selectMatchingCodecs()：根据容器格式从 supportedVideoCodecs/supportedAudioCodecs
 *      中查找兼容的编码器并显式设置
 *   3. Linux 优先使用 video/webm（VP8+Vorbis），更稳定；回退 video/ogg（Theora+Vorbis）
 *   4. Windows 继续让系统自动选择编码器（DirectShow/WMF 对 mp4 支持良好）
 */

#include "DualRecordWidget.h"
#include "InitConfigDialog.h"
#include "PlayerWindow.h"

#ifdef Q_OS_LINUX
#include "GstRecorder.h"
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#endif

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QCameraInfo>
#include <QAudioDeviceInfo>
#include <QVideoEncoderSettings>
#include <QAudioEncoderSettings>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QDateTime>
#include <QMessageBox>
#include <QStyle>
#include <QSizePolicy>
#include <QFrame>
#include <QTextStream>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QThread>

// ─────────────────────────────────────────
// 日志：统一使用 AppLogger（AppLogger.h）
// ─────────────────────────────────────────
#include "AppLogger.h"
#define LOG(msg) APP_LOG(msg)

// ─────────────────────────────────────────
// 构造 / 析构
// ─────────────────────────────────────────
DualRecordWidget::DualRecordWidget(QWidget *parent)
    : QWidget(parent)
    , m_camera(nullptr)
    , m_recorder(nullptr)
    , m_durationTimer(nullptr)
    , m_recordStartTimer(nullptr)
    , m_settings(nullptr)
    , m_state(RecordState::Idle)
    , m_recordingStartMs(0)
    , m_pendingCloseCamera(false)
    , m_videoContainer(nullptr)
#ifdef Q_OS_LINUX
    , m_gstRecorder(nullptr)
    , m_gstPreviewWidget(nullptr)
#endif
{
    LOG("========== DualRecordWidget 新 session ==========");

    m_settings = new QSettings("BankSystem", "DualRecord", this);
    loadConfig();
    setupUI();
    loadDevices();       // 枚举设备，但不开摄像头
    updateButtonStates();

    // 监听视频容器 resize 事件，同步更新 GStreamer 预览区域大小
    m_videoContainer->installEventFilter(this);

    // 持续计时器（每秒更新录制时长）
    m_durationTimer = new QTimer(this);
    m_durationTimer->setInterval(1000);
    connect(m_durationTimer, &QTimer::timeout, this, &DualRecordWidget::updateDuration);

    // 录制启动超时计时器（防止摄像头就绪后 recorder 始终无法进入 RecordingState）
    m_recordStartTimer = new QTimer(this);
    m_recordStartTimer->setSingleShot(true);
    m_recordStartTimer->setInterval(5000);  // 5 秒超时
    connect(m_recordStartTimer, &QTimer::timeout, this, &DualRecordWidget::onRecordStartTimeout);
}

DualRecordWidget::~DualRecordWidget()
{
#ifdef Q_OS_LINUX
    // 清理 GStreamer 录制器
    if (m_gstRecorder) {
        if (m_gstRecorder->isRecording())
            m_gstRecorder->stop(3000);
        delete m_gstRecorder;
        m_gstRecorder = nullptr;
    }
#endif

    // 安全清理：停止录制器，关闭摄像头
    if (m_recorder && m_recorder->state() != QMediaRecorder::StoppedState)
        m_recorder->stop();

    if (m_camera) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }
}

// ─────────────────────────────────────────
// UI 搭建
// ─────────────────────────────────────────
void DualRecordWidget::setupUI()
{
    setWindowTitle("银行柜面双录系统 v1.0");
    setWindowIcon(QApplication::windowIcon());  // 继承全局应用图标
    setMinimumSize(560, 500);
    resize(620, 560);
    setStyleSheet(R"(
        QWidget {
            background: #f3f4f8;
            color: #1f2937;
            font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #e2e5ec;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 16px;
            background: #ffffff;
            font-weight: bold;
            color: #374151;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            color: #1a56db;
        }
        QPushButton {
            background: #1a56db;
            color: white;
            border: none;
            border-radius: 7px;
            padding: 9px 20px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton:hover  { background: #1340a8; }
        QPushButton:disabled {
            background: #e5e7eb;
            color: #9ca3af;
            border: 1px solid #d1d5db;
        }
        /* ── 开始录制按钮：绿色（默认/空闲）/ 红色（录制中，属性 state=stop） ── */
        QPushButton#btnRecord                    { background: #16a34a; color: white; border: none; font-size: 14px; }
        QPushButton#btnRecord:enabled            { background: #16a34a; color: white; border: none; }
        QPushButton#btnRecord:hover              { background: #15803d; color: white; }
        QPushButton#btnRecord[state="stop"]      { background: #dc2626; color: white; border: none; }
        QPushButton#btnRecord[state="stop"]:enabled { background: #dc2626; color: white; border: none; }
        QPushButton#btnRecord[state="stop"]:hover { background: #b91c1c; color: white; }
        QPushButton#btnRecord:disabled           { background: #9ca3af; color: #e5e7eb; border: none; }
        QPushButton#btnConfig {
            background: #6b7280;
            color: white;
            border: none;
            border-radius: 7px;
            padding: 9px 18px;
        }
        QPushButton#btnConfig:hover { background: #4b5563; color: white; }
        QPushButton#btnExit {
            background: #b91c1c;
            color: white;
            border: none;
            border-radius: 7px;
            padding: 9px 18px;
        }
        QPushButton#btnExit:hover { background: #991b1b; color: white; }
        QComboBox {
            background: #fff;
            border: 1px solid #d1d5db;
            border-radius: 6px;
            padding: 5px 10px;
            color: #1f2937;
        }
        QComboBox::drop-down { border: none; }
        QLabel#statusLabel { font-size: 14px; font-weight: bold; }
        QLabel#durationLabel {
            font-size: 34px;
            font-weight: bold;
            color: #d97706;
            font-family: 'Consolas', monospace;
            letter-spacing: 3px;
        }
    )");

    // ═══ 整体垂直主布局 ═══
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 14, 18, 14);
    mainLayout->setSpacing(12);

    // ── 顶部标题行（标题 + 配置按钮） ──
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *titleLabel = new QLabel("🎬 银行柜面双录系统");
    titleLabel->setStyleSheet(
        "font-size:17px;font-weight:bold;color:#1a56db;"
        "background:transparent;");
    titleRow->addWidget(titleLabel);
    titleRow->addStretch();

    m_btnConfig = new QPushButton("⚙  配置");
    m_btnConfig->setObjectName("btnConfig");
    m_btnConfig->setFixedHeight(36);
    m_btnConfig->setFixedWidth(100);
    connect(m_btnConfig, &QPushButton::clicked, this, &DualRecordWidget::onConfigBtnClicked);
    titleRow->addWidget(m_btnConfig);

    m_btnExit = new QPushButton("✕  关闭");
    m_btnExit->setObjectName("btnExit");
    m_btnExit->setFixedHeight(36);
    m_btnExit->setFixedWidth(90);
    connect(m_btnExit, &QPushButton::clicked, this, &DualRecordWidget::onExitBtnClicked);
    titleRow->addWidget(m_btnExit);

    mainLayout->addLayout(titleRow);

    // ── 摄像头预览区域 ──
    m_videoContainer = new QWidget;
    m_videoContainer->setMinimumHeight(300);
    m_videoContainer->setStyleSheet(
        "background:#0a0e1a;border-radius:10px;");
    m_videoContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *vcLayout = new QVBoxLayout(m_videoContainer);
    vcLayout->setContentsMargins(0, 0, 0, 0);
    vcLayout->setSpacing(0);

    m_viewfinder = new QCameraViewfinder;
    m_viewfinder->setStyleSheet("background:transparent;");
    vcLayout->addWidget(m_viewfinder);

    m_camPlaceholder = new QLabel("📷\n点击「开始录制」开启摄像头");
    m_camPlaceholder->setAlignment(Qt::AlignCenter);
    m_camPlaceholder->setStyleSheet(
        "color:#6b7280;font-size:15px;background:transparent;padding:20px;");
    m_camPlaceholder->setVisible(true);
    vcLayout->addWidget(m_camPlaceholder);

    mainLayout->addWidget(m_videoContainer, 1);

    // ── 设备选择行（摄像头下方） ──
    QWidget *devCard = new QWidget;
    devCard->setObjectName("devCard");
    devCard->setStyleSheet(
        "QWidget#devCard{background:#ffffff;border:1px solid #e2e5ec;"
        "border-radius:8px;padding:4px;}");
    QGridLayout *devGrid = new QGridLayout(devCard);
    devGrid->setContentsMargins(12, 8, 12, 8);
    devGrid->setHorizontalSpacing(16);
    devGrid->setVerticalSpacing(4);

    auto mkLabel = [](const QString &t) {
        QLabel *l = new QLabel(t);
        l->setStyleSheet("color:#6b7280;font-size:12px;background:transparent;");
        return l;
    };

    devGrid->addWidget(mkLabel("📷 摄像头"), 0, 0);
    m_cameraSelector = new QComboBox;
    devGrid->addWidget(m_cameraSelector, 1, 0);

    devGrid->addWidget(mkLabel("🎤 麦克风"), 0, 1);
    m_audioSelector = new QComboBox;
    devGrid->addWidget(m_audioSelector, 1, 1);

    devGrid->addWidget(mkLabel("📐 分辨率"), 0, 2);
    m_resolutionSelector = new QComboBox;
    m_resolutionSelector->addItem("1280×720 (HD)",   QSize(1280, 720));
    m_resolutionSelector->addItem("1920×1080 (FHD)", QSize(1920, 1080));
    m_resolutionSelector->addItem("640×480 (SD)",    QSize(640, 480));
    devGrid->addWidget(m_resolutionSelector, 1, 2);

    devGrid->setColumnStretch(0, 1);
    devGrid->setColumnStretch(1, 1);
    devGrid->setColumnStretch(2, 1);

    mainLayout->addWidget(devCard);

    // ── 录制控制区域（摄像头下方） ──
    QWidget *ctrlCard = new QWidget;
    ctrlCard->setObjectName("ctrlCard");
    ctrlCard->setStyleSheet(
        "QWidget#ctrlCard{background:#ffffff;border:1px solid #e2e5ec;"
        "border-radius:8px;}");
    QVBoxLayout *ctrlLayout = new QVBoxLayout(ctrlCard);
    ctrlLayout->setContentsMargins(16, 12, 16, 12);
    ctrlLayout->setSpacing(10);

    // 计时 + 状态横排
    QHBoxLayout *infoRow = new QHBoxLayout;
    m_durationLabel = new QLabel("00:00:00");
    m_durationLabel->setObjectName("durationLabel");
    m_durationLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    infoRow->addWidget(m_durationLabel);

    infoRow->addStretch();

    m_statusLabel = new QLabel("● 就绪");
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_statusLabel->setStyleSheet("color:#6b7280;font-weight:bold;font-size:14px;"
                                 "background:transparent;");
    infoRow->addWidget(m_statusLabel);
    ctrlLayout->addLayout(infoRow);

    // 按钮行
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(12);

    m_btnRecord = new QPushButton("▶  开始录制");
    m_btnRecord->setObjectName("btnRecord");
    m_btnRecord->setFixedHeight(46);
    m_btnRecord->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    btnRow->addWidget(m_btnRecord, 1);
    ctrlLayout->addLayout(btnRow);

    mainLayout->addWidget(ctrlCard);

    setLayout(mainLayout);

    // ── 信号连接 ──
    connect(m_btnRecord, &QPushButton::clicked, this, &DualRecordWidget::onRecordBtnClicked);
    connect(m_cameraSelector,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DualRecordWidget::onCameraChanged);
}

// ─────────────────────────────────────────
// 设备枚举（只枚举，不打开摄像头）
// ─────────────────────────────────────────
void DualRecordWidget::loadDevices()
{
    m_cameraSelector->blockSignals(true);
    m_cameraSelector->clear();

    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cam : cameras)
        m_cameraSelector->addItem(cam.description(), cam.deviceName());

    if (m_cameraSelector->count() == 0)
        m_cameraSelector->addItem("未检测到摄像头", "");

    m_cameraSelector->blockSignals(false);

    m_audioSelector->clear();
    m_audioSelector->addItem("默认麦克风", "");
    for (const QAudioDeviceInfo &dev :
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        m_audioSelector->addItem(dev.deviceName(), dev.deviceName());
    }

    m_statusLabel->setText("● 就绪");
    m_statusLabel->setStyleSheet("color:#6b7280;font-weight:bold;font-size:14px;"
                                 "background:transparent;");
}

// ─────────────────────────────────────────
// 根据容器格式选择匹配的视频和音频编码器
//
// v2.9 关键修复：
//   video/ogg 只支持 video/x-theora + audio/x-vorbis
//   video/webm 只支持 video/x-vp8/vp9 + audio/x-vorbis/opus
//   不显式指定 codec 时，GStreamer 可能自动选择不兼容编码器，
//   导致管线构建失败后 recorder 静默停止（无 error 信号）
// ─────────────────────────────────────────
void DualRecordWidget::selectMatchingCodecs(const QString &container,
                                            QVideoEncoderSettings &vSettings,
                                            QAudioEncoderSettings &aSettings)
{
    QStringList videoCodecs = m_recorder->supportedVideoCodecs();
    QStringList audioCodecs = m_recorder->supportedAudioCodecs();

    LOG("[Recorder] 可用视频编码: " + videoCodecs.join(", "));
    LOG("[Recorder] 可用音频编码: " + audioCodecs.join(", "));

    QString containerBase = container;
    // 去掉 MIME 前缀用于匹配（注意："video/x-matroska" 去掉 "video/" 后是 "x-matroska"，
    // 所以用 contains() 而不是直接比较）
    if (containerBase.startsWith("video/"))
        containerBase = containerBase.mid(6);
    else if (containerBase.startsWith("application/"))
        containerBase = containerBase.mid(12);
    // 处理带 "x-" 前缀的格式名（如 "x-matroska" → "matroska"）
    if (containerBase.startsWith("x-"))
        containerBase = containerBase.mid(2);

    // ── 根据容器格式确定首选/备选编码器 ──
    QString preferredVideo, fallbackVideo;
    QString preferredAudio, fallbackAudio;

    if (containerBase == "webm" || containerBase.contains("webm")) {
        // webm 容器：只支持 VP8/V9 + Vorbis/Opus，不支持 alaw
        preferredVideo = "video/x-vp8";
        fallbackVideo  = "video/x-vp9";
        preferredAudio = "audio/x-vorbis";
        fallbackAudio  = "audio/x-opus";
    } else if (containerBase == "matroska" || containerBase.contains("matroska")) {
        // MKV: VP8 + PCM alaw（不用编码器插件，raw PCM 直通，GStreamer 最稳定）
        // 所有编码音频（aac/vorbis/h264）在 Linux 上均闪停，音频采集端有根本性问题
        // 改用 raw PCM 音频（alaw 固定格式，不需要编码器插件），绕过音频编码失败问题
        preferredVideo = "video/x-vp8";
        fallbackVideo  = "video/x-vp9";
        preferredAudio = "audio/x-alaw";   // G.711 alaw — raw PCM，不需要编码器
        fallbackAudio  = "audio/x-mulaw";  // G.711 ulaw 备选
    } else if (containerBase == "ogg" || containerBase.contains("ogg")) {
        // OGG: Theora; Vorbis(首选) > Opus
        preferredVideo = "video/x-theora";
        preferredAudio = "audio/x-vorbis";
        fallbackAudio  = "audio/x-opus";
    } else if (containerBase.contains("mp4") || containerBase.contains("mpeg4")
               || containerBase.contains("quicktime") || containerBase.contains("msvideo")) {
        // MP4/MOV/AVI: H264+AAC（Linux Qt GStreamer 最标准组合，与原始版本一致）
        preferredVideo = "video/x-h264";
        fallbackVideo  = "video/x-h265";
        preferredAudio = "audio/aac";
        fallbackAudio  = "audio/mpeg";
    }

    // ── 从 supportedVideoCodecs 中匹配视频编码器 ──
    if (!preferredVideo.isEmpty()) {
        for (const QString &codec : videoCodecs) {
            if (codec.compare(preferredVideo, Qt::CaseInsensitive) == 0) {
                vSettings.setCodec(codec);
                LOG("[Recorder] 视频编码匹配: " + codec + " (首选)");
                goto videoDone;
            }
        }
        if (!fallbackVideo.isEmpty()) {
            for (const QString &codec : videoCodecs) {
                if (codec.compare(fallbackVideo, Qt::CaseInsensitive) == 0) {
                    vSettings.setCodec(codec);
                    LOG("[Recorder] 视频编码匹配: " + codec + " (备选)");
                    goto videoDone;
                }
            }
        }
    }
    // 未匹配到，让系统自动选择（仅 Windows 可以这样）
    LOG("[Recorder] 视频编码: 未显式指定，由系统自动选择");
videoDone:

    // ── 从 supportedAudioCodecs 中匹配音频编码器 ──
    if (!preferredAudio.isEmpty()) {
        for (const QString &codec : audioCodecs) {
            if (codec.compare(preferredAudio, Qt::CaseInsensitive) == 0) {
                aSettings.setCodec(codec);
                LOG("[Recorder] 音频编码匹配: " + codec + " (首选)");
                goto audioDone;
            }
        }
        if (!fallbackAudio.isEmpty()) {
            for (const QString &codec : audioCodecs) {
                if (codec.compare(fallbackAudio, Qt::CaseInsensitive) == 0) {
                    aSettings.setCodec(codec);
                    LOG("[Recorder] 音频编码匹配: " + codec + " (备选)");
                    goto audioDone;
                }
            }
        }
    }
    LOG("[Recorder] 音频编码: 未显式指定，由系统自动选择");
audioDone:
    ;
}

// ─────────────────────────────────────────
// 运行时检测实际可用的容器格式
// ─────────────────────────────────────────
QString DualRecordWidget::detectContainerFormat()
{
    if (!m_recorder) {
        LOG("[Recorder] detectContainerFormat: m_recorder 为空");
#ifdef Q_OS_WIN
        return "video/mp4";
#else
        return "video/webm";
#endif
    }

    QStringList containers = m_recorder->supportedContainers();
    LOG("[Recorder] 支持的容器格式: " + containers.join(", "));

#ifdef Q_OS_WIN
    // Windows 优先 mp4 / mpeg4 / matroska / avi，最后 mov
    for (const QString &c : {"mp4", "video/mp4", "mpeg4", "video/mpeg4",
                              "matroska", "video/x-matroska", "avi", "video/x-msvideo",
                              "mov", "video/quicktime"}) {
        for (const QString &sc : containers) {
            if (sc.compare(c, Qt::CaseInsensitive) == 0)
                return c;
        }
    }
#else
    // Linux/麒麟：webm 原生 > matroska > ogg > quicktime
    // v2.9-fix6: webm 是 GStreamer 原生格式（VP8/Vorbis），管线最简单
    // 不用 mp4（需要 libav 插件，可能导致管线不稳定）
    for (const QString &c : {"video/webm", "webm",
                            "video/x-matroska", "matroska",
                            "application/ogg", "ogg",
                            "video/quicktime", "mov"}) {
        for (const QString &sc : containers) {
            if (sc.compare(c, Qt::CaseInsensitive) == 0)
                return c;
        }
    }
#endif

    // 兜底：使用第一个可用容器，加上 video/ 前缀
    if (!containers.isEmpty()) {
        QString first = containers.first();
        if (!first.contains('/'))
            return "video/" + first;
        return first;
    }
#ifdef Q_OS_WIN
    return "video/mp4";
#else
    return "video/webm";
#endif
}

/**
 * @brief 容器格式对应的文件扩展名
 */
static QString containerToExtension(const QString &container)
{
    QString c = container.toLower();
    // 支持 MIME 格式（"video/mp4"）和短名格式（"mp4"）
    if (c == "mp4" || c == "mpeg4" || c == "video/mp4" || c == "video/mpeg4")  return ".mp4";
    if (c == "mov" || c == "video/quicktime")                                       return ".mov";
    if (c == "matroska" || c == "mkv" || c == "video/x-matroska")                  return ".mkv";
    if (c == "avi" || c == "video/x-msvideo")                                      return ".avi";
    if (c == "ogg" || c == "video/ogg")                                            return ".ogv";
    if (c == "webm" || c == "video/webm")                                          return ".webm";
    return ".mp4";  // 兜底
}

// ─────────────────────────────────────────
// 打开摄像头（录制时调用）
// ─────────────────────────────────────────
void DualRecordWidget::openCamera()
{
    int index = m_cameraSelector->currentIndex();
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty() || index < 0 || index >= cameras.size()) {
        QMessageBox::warning(this, "提示", "未检测到可用摄像头，请确认设备已连接。");
        return;
    }

    if (m_camera) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }

    m_camera = new QCamera(cameras[index], this);

    // v2.9-fix6: setCaptureMode(CaptureVideo) 在麒麟 Qt5 GStreamer 后端会直接 segfault
    // 所以不调用此方法。默认 CaptureViewfinder 模式下 recorder 也能工作，
    // 问题出在其他地方（codec 配置/pipeline 协商），需要 GST_DEBUG 日志确认。
    // m_camera->setCaptureMode(QCamera::CaptureVideo);  // ← 麒麟上崩溃，禁用

    m_camera->setViewfinder(m_viewfinder);

    // 监听摄像头错误
    connect(m_camera, QOverload<QCamera::Error>::of(&QCamera::error),
            this, [this](QCamera::Error err) {
        Q_UNUSED(err)
        QString msg = m_camera ? m_camera->errorString() : "未知错误";
        qWarning() << "[Camera] 错误:" << msg;
        m_statusLabel->setText("⚠ 摄像头错误: " + msg);
        m_statusLabel->setStyleSheet("color:#dc2626;font-weight:bold;"
                                     "background:transparent;");
        m_recordStartTimer->stop();
        m_durationTimer->stop();
        m_durationLabel->setText("00:00:00");
        m_state = RecordState::Idle;
        m_btnRecord->setText("▶  开始录制");
        m_btnRecord->setProperty("state", "");
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);
        m_btnRecord->setEnabled(true);
        updateButtonStates();
        closeCamera();
    });

    // 摄像头状态变化：ActiveStatus 时触发 onRecorderReady
    // 必须在 start() 之前连接，防止错过事件
    connect(m_camera, &QCamera::statusChanged, this, [this](QCamera::Status s) {
        bool active = (s == QCamera::ActiveStatus);
        m_camPlaceholder->setVisible(!active);
        m_viewfinder->setVisible(active);
    });

    connect(m_camera, &QCamera::statusChanged, this, &DualRecordWidget::onRecorderReady);

    m_viewfinder->setVisible(false);
    m_camera->start();
}

// ─────────────────────────────────────────
// 摄像头就绪 → 创建 recorder + audioInput → 开始录制
// ─────────────────────────────────────────
void DualRecordWidget::onRecorderReady()
{
    // 只在录制启动阶段处理一次，其他状态忽略
    if (!m_camera || m_camera->status() != QCamera::ActiveStatus)
        return;

    // 断开一次性连接（避免重复触发）
    disconnect(m_camera, &QCamera::statusChanged, this, &DualRecordWidget::onRecorderReady);

    // ── v2.9-fix9: Linux 录制改用 GstRecorder（GStreamer C API） ──
    // 原因 v2.9-fix7: Qt5 QMediaRecorder 在麒麟 GStreamer 1.16.3 上不创建编码管线
    // 原因 v2.9-fix8: gst-launch-1.0 的 ximagesink embed/window-handle 需要 GStreamer 1.18+
    // 方案：GstRecorder 用 C API 直接管理 pipeline，GstVideoOverlay（1.0+可用）嵌入预览
#ifdef Q_OS_LINUX
    LOG("[Linux] 使用 GstRecorder（GStreamer C API）录制模式");
    LOG(QString("[Linux] 录制文件: %1").arg(m_currentFilePath));

    // 确定视频设备节点（/dev/video0）
    QString videoDevice = "/dev/video0";
    if (m_camera) {
        QCameraInfo ci(*m_camera);
        if (!ci.deviceName().isEmpty()) {
            videoDevice = ci.deviceName();
        }
    }

    // ★ 关键：先停止 QCamera 释放设备节点，否则 v4l2src 报"设备正忙"
    LOG(QString("[Linux] 释放 QCamera 预览，让 GStreamer 独占设备 %1").arg(videoDevice));
    if (m_camera) {
        disconnect(m_camera, &QCamera::statusChanged, this, &DualRecordWidget::onRecorderReady);
        disconnect(m_camera, nullptr, m_viewfinder, nullptr);
        m_camera->stop();
        m_viewfinder->setVisible(false);
    }

    // 等待设备节点释放（QCamera stop 是异步的）
    QElapsedTimer releaseTimer;
    releaseTimer.start();
    QThread::msleep(300);
    LOG(QString("[Linux] QCamera 已停止，等待 %1ms").arg(releaseTimer.elapsed()));

    // ★ 创建 GStreamer 预览容器 widget，用于 VideoOverlay 嵌入
    // 直接作为 m_videoContainer 的子控件，覆盖整个视频区域
    if (!m_gstPreviewWidget) {
        m_gstPreviewWidget = new QWidget(m_videoContainer);
        m_gstPreviewWidget->setStyleSheet("background:#000;");
        m_gstPreviewWidget->setAttribute(Qt::WA_NativeWindow);  // 确保有原生窗口句柄
        m_gstPreviewWidget->setAttribute(Qt::WA_DontCreateNativeAncestors);
    }
    // 让预览容器完全覆盖视频区域
    m_gstPreviewWidget->setGeometry(m_videoContainer->rect());
    m_gstPreviewWidget->show();
    m_gstPreviewWidget->raise();
    m_viewfinder->hide();
    m_camPlaceholder->hide();

    // 获取预览容器的窗口 ID（GstVideoOverlay 嵌入用）
    WId previewWId = m_gstPreviewWidget->winId();
    LOG(QString("[Linux] GStreamer 预览窗口 WId: %1").arg(previewWId));

    // ★ 创建 GstRecorder 并启动录制 pipeline ★
    m_gstRecorder = new GstRecorder(this);

    // 监听 bus 日志消息（用于诊断）
    connect(m_gstRecorder, &GstRecorder::busMessage, this, [](const QString &msg) {
        LOG("[Linux][gst-bus] " + msg);
    });

    // 监听录制完成/异常信号
    connect(m_gstRecorder, &GstRecorder::finished, this,
            [this](bool success, const QString &errorMsg) {
        LOG(QString("[Linux] GstRecorder finished, success=%1, error='%2'")
            .arg(success ? "true" : "false").arg(errorMsg));

        // 通知 pipeline 可清理
        m_gstRecorder->deleteLater();
        m_gstRecorder = nullptr;

        if (m_state == RecordState::Recording) {
            // 非预期退出（不是用户手动停止的）
            m_state = RecordState::Stopped;
            m_durationTimer->stop();
            m_durationLabel->setText("00:00:00");

            if (success) {
                m_statusLabel->setText("✓ 录制完成");
                m_statusLabel->setStyleSheet("color:#059669;font-weight:bold;font-size:14px;"
                                             "background:transparent;");
            } else {
                m_statusLabel->setText("⚠ 录制异常: " + errorMsg);
                m_statusLabel->setStyleSheet("color:#dc2626;font-weight:bold;font-size:14px;"
                                             "background:transparent;");
            }

            m_btnRecord->setText("▶  开始录制");
            m_btnRecord->setProperty("state", "");
            m_btnRecord->style()->unpolish(m_btnRecord);
            m_btnRecord->style()->polish(m_btnRecord);
            m_btnRecord->setEnabled(true);
            updateButtonStates();

            // 清理预览窗口
            if (m_gstPreviewWidget) m_gstPreviewWidget->hide();
            m_camPlaceholder->show();
            closeCamera();
        }
    });

    // 获取分辨率
    QSize res = m_resolutionSelector->currentData().toSize();
    if (!res.isValid()) res = QSize(1280, 720);

    bool started = m_gstRecorder->start(videoDevice, m_currentFilePath, previewWId, res);

    if (!started) {
        QString gstError = m_gstRecorder->lastError();
        LOG("[Linux] ✗ GstRecorder 启动失败: " + gstError);
        m_gstRecorder->deleteLater();
        m_gstRecorder = nullptr;

        m_state = RecordState::Idle;
        m_btnRecord->setText("▶  开始录制");
        m_btnRecord->setProperty("state", "");
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);
        m_btnRecord->setEnabled(true);
        m_statusLabel->setText("⚠ 录制启动失败: " + gstError);
        m_statusLabel->setStyleSheet("color:#dc2626;font-weight:bold;font-size:14px;"
                                     "background:transparent;");
        updateButtonStates();

        // 清理预览窗口
        if (m_gstPreviewWidget) m_gstPreviewWidget->hide();
        m_camPlaceholder->show();
        closeCamera();
        return;
    }

    LOG("[Linux] ✓ GstRecorder 录制已启动");

    // 录制成功启动
    m_state = RecordState::Recording;
    m_recordStartTimer->stop();
    m_recordingStartMs = QDateTime::currentMSecsSinceEpoch();
    m_durationTimer->start();

    m_btnRecord->setText("⏹  停止录制");
    m_btnRecord->setProperty("state", "stop");
    m_btnRecord->style()->unpolish(m_btnRecord);
    m_btnRecord->style()->polish(m_btnRecord);
    m_btnRecord->setEnabled(true);
    m_statusLabel->setText("🔴 录制中...");
    m_statusLabel->setStyleSheet("color:#dc2626;font-weight:bold;font-size:14px;"
                                 "background:transparent;");
    updateButtonStates();

    emit recordStarted(m_currentTaskId, m_currentFilePath);

#else
    // ── Windows: 保持原有 QMediaRecorder 逻辑 ──

    // ── 创建 MediaRecorder ──
    setupRecorder(m_currentFilePath);

    if (!m_recorder) {
        // setupRecorder 内部失败（不应发生，但防万一）
        qWarning() << "[Recorder] 创建失败，中止录制";
        m_state = RecordState::Idle;
        m_btnRecord->setText("▶  开始录制");
        m_btnRecord->setProperty("state", "");
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);
        m_btnRecord->setEnabled(true);
        m_statusLabel->setText("⚠ 录制器创建失败");
        m_statusLabel->setStyleSheet("color:#dc2626;font-weight:bold;font-size:14px;"
                                     "background:transparent;");
        updateButtonStates();
        closeCamera();
        return;
    }

    // ── Qt5 音频说明 ──
    // Qt5 的 QMediaRecorder 通过 setEncodingSettings 三参数方法统一配置音视频+容器。
    // 音频由底层 media service 自动采集（Windows: DirectShow/WMF 会使用默认麦克风）。
    // setAudioSettings 已在 setupRecorder 中通过 setEncodingSettings 一起配置。
    LOG("[Audio] 音频由底层 media service 自动采集（默认麦克风）");

    // ── 诊断：record() 之前的 recorder 状态 ──
    LOG(QString("[Recorder] record() 前 — state=%1 status=%2 error=%3 errorString='%4' container=%5 output=%6")
        .arg(m_recorder->state()).arg(m_recorder->status()).arg(m_recorder->error())
        .arg(m_recorder->errorString()).arg(m_actualContainer).arg(m_currentFilePath));

    // ── 开始录制 ──
    LOG("[Recorder] >>> 调用 record()，输出文件: " + m_currentFilePath);
    m_recorder->record();

    // ── 启动超时检测：5 秒内必须进入 RecordingState ──
    m_recordStartTimer->start();

    // 更新 UI 状态
    m_recordingStartMs = QDateTime::currentMSecsSinceEpoch();
    m_durationTimer->start();
    m_state = RecordState::Recording;
    m_statusLabel->setText("⏳ 录制启动中...");
    m_statusLabel->setStyleSheet("color:#d97706;font-weight:bold;font-size:14px;"
                                 "background:transparent;");

    m_btnRecord->setText("■  停止录制");
    m_btnRecord->setProperty("state", "stop");
    m_btnRecord->style()->unpolish(m_btnRecord);
    m_btnRecord->style()->polish(m_btnRecord);
    m_btnRecord->setEnabled(true);

    updateButtonStates();
#endif // Q_OS_LINUX
}

// ─────────────────────────────────────────
// 录制启动超时处理
// ─────────────────────────────────────────
void DualRecordWidget::onRecordStartTimeout()
{
    LOG("[Recorder] !!! 录制启动超时（5秒）!!!");
    if (m_recorder) {
        LOG(QString("[Recorder]   state=%1 status=%2 error=%3 errorString='%4' container=%5 output=%6 available=%7")
            .arg(m_recorder->state()).arg(m_recorder->status()).arg(m_recorder->error())
            .arg(m_recorder->errorString()).arg(m_actualContainer).arg(m_currentFilePath)
            .arg(m_recorder->isAvailable() ? "YES" : "NO"));
    } else {
        LOG("[Recorder]   m_recorder 为 null");
    }

    if (m_state == RecordState::Recording && m_recorder
        && m_recorder->state() != QMediaRecorder::RecordingState) {
        // 超时但确实没进入录制状态 → 视为失败
        m_statusLabel->setText("⚠ 录制启动失败（超时）");
        m_statusLabel->setStyleSheet("color:#dc2626;font-weight:bold;font-size:14px;"
                                     "background:transparent;");
        m_durationTimer->stop();
        m_durationLabel->setText("00:00:00");

        // 清理并回到空闲
        m_recorder->stop();
        closeCamera();

        m_state = RecordState::Idle;
        m_btnRecord->setText("▶  开始录制");
        m_btnRecord->setProperty("state", "");
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);
        m_btnRecord->setEnabled(true);
        updateButtonStates();

        emit recordError(m_currentTaskId, "录制启动超时，recorder 未进入 RecordingState");
    }
    // 如果已经 RecordingState 了，忽略超时
}

// ─────────────────────────────────────────
// 关闭摄像头（停止录制后调用）
// 核心原则：先清理 recorder 和 audioInput，再删除 camera
// ─────────────────────────────────────────
void DualRecordWidget::closeCamera()
{
    m_recordStartTimer->stop();

#ifdef Q_OS_LINUX
    // 1. 停止 GstRecorder 录制器（如果还在运行）
    if (m_gstRecorder) {
        if (m_gstRecorder->isRecording()) {
            LOG("[Linux] closeCamera: 停止 GstRecorder");
            m_gstRecorder->stop(5000);
        }
        m_gstRecorder->deleteLater();
        m_gstRecorder = nullptr;
    }
#endif

    // 2. 停止并清理 recorder（Windows 下使用；Linux 下为 null）
    if (m_recorder) {
        if (m_recorder->state() != QMediaRecorder::StoppedState)
            m_recorder->stop();
        disconnect(m_recorder, nullptr, this, nullptr);
        m_recorder->deleteLater();
        m_recorder = nullptr;
    }
    m_pendingCloseCamera = false;

    // 3. 最后停止并删除 camera
    if (m_camera) {
        disconnect(m_camera, nullptr, this, nullptr);
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }

    m_viewfinder->setVisible(false);
    m_camPlaceholder->setVisible(true);
    m_camPlaceholder->setText("📷\n点击「开始录制」开启摄像头");
}

// ─────────────────────────────────────────
// 摄像头切换槽（录制中禁止切换）
// ─────────────────────────────────────────
void DualRecordWidget::onCameraChanged(int index)
{
    Q_UNUSED(index)

    // 空闲/停止态：下次开始录制时使用新选项，无需立即操作
    if (m_state == RecordState::Idle || m_state == RecordState::Stopped)
        return;

    // 录制中：不允许切换摄像头（切换会导致新文件+时间重置）
    // 回滚选择器到之前的选项
    m_cameraSelector->blockSignals(true);
    if (m_camera) {
        QCameraInfo currentInfo(*m_camera);
        m_cameraSelector->setCurrentIndex(
            m_cameraSelector->findData(currentInfo.deviceName()));
    }
    m_cameraSelector->blockSignals(false);
}

// ─────────────────────────────────────────
// 「开始录制」/「停止录制」合一按钮
// ─────────────────────────────────────────
void DualRecordWidget::onRecordBtnClicked()
{
    if (m_state == RecordState::Idle || m_state == RecordState::Stopped) {
        // ── 开始录制 ──
        // 先探测可用容器格式（需要在 camera 创建 recorder 之后才能查询）
        // 这里先创建一个临时 camera 来查询——不，那太浪费了。
        // 实际上 detectContainerFormat 需要 QMediaRecorder 实例，
        // 而 QMediaRecorder 需要 QCamera 作为 parent/mediaObject。
        // 所以我们先打开摄像头，在 onRecorderReady 中做 setupRecorder。
        // 但生成文件名需要知道扩展名... 这是个鸡生蛋的问题。
        //
        // 解决方案：先用默认扩展名生成临时文件名，setupRecorder 中检测到实际容器后，
        // 如果扩展名不匹配则重命名文件。
        //
        // 更简单的方案：先生成文件路径，setupRecorder 中设置容器格式时同时更新扩展名。

        QDir().mkpath(m_videoCachePath);

        // 生成临时文件名（扩展名稍后在 setupRecorder 中修正）
        QString fileName = generateFileName(m_currentTaskId);
        m_currentFilePath = m_videoCachePath + "/" + fileName;

        // 按钮立即切为「停止录制」
        m_btnRecord->setText("⏳ 启动中...");
        m_btnRecord->setProperty("state", "stop");
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);
        m_btnRecord->setEnabled(false);
        m_statusLabel->setText("⏳ 摄像头启动中...");
        m_statusLabel->setStyleSheet("color:#d97706;font-weight:bold;font-size:14px;"
                                     "background:transparent;");

        // 打开摄像头 → 摄像头就绪后触发 onRecorderReady → setupRecorder → record()
        openCamera();
        if (!m_camera) {
            // openCamera 内部检测到无摄像头并弹框提示
            m_btnRecord->setText("▶  开始录制");
            m_btnRecord->setProperty("state", "");
            m_btnRecord->style()->unpolish(m_btnRecord);
            m_btnRecord->style()->polish(m_btnRecord);
            m_btnRecord->setEnabled(true);
            m_statusLabel->setText("● 就绪");
            m_statusLabel->setStyleSheet("color:#6b7280;font-weight:bold;font-size:14px;"
                                         "background:transparent;");
            return;
        }

    } else if (m_state == RecordState::Recording) {
        // ── 停止录制 ──
        QString savedPath = m_currentFilePath;
        QString savedTaskId = m_currentTaskId;
        m_state = RecordState::Stopped;

        // 停止超时计时器
        m_recordStartTimer->stop();

        // 设标记：等 recorder 完全落盘（StoppedState）后再关摄像头
        // 不能同步 closeCamera，否则 m_camera 被删除会导致 recorder 无法正常 flush
        m_pendingCloseCamera = true;

#ifdef Q_OS_LINUX
        // Linux: 停止 GstRecorder（发 EOS → 优雅落盘）
        if (m_gstRecorder && m_gstRecorder->isRecording()) {
            LOG("[Linux] 停止 GstRecorder...");
            bool ok = m_gstRecorder->stop(10000);
            LOG(QString("[Linux] GstRecorder 停止%1").arg(ok ? "成功" : "超时，已强制停止"));
            m_gstRecorder->deleteLater();
            m_gstRecorder = nullptr;
        }

        // 隐藏 GStreamer 预览窗口，恢复占位图
        if (m_gstPreviewWidget) {
            m_gstPreviewWidget->hide();
        }
        m_camPlaceholder->show();

        m_pendingCloseCamera = false;  // GStreamer 已停，直接关摄像头
        closeCamera();
#else
        // Windows: 停止 QMediaRecorder
        if (m_recorder) {
            qDebug() << "[Recorder] 停止录制，等待落盘...";
            m_recorder->stop();
        }
#endif

        m_durationTimer->stop();
        m_durationLabel->setText("00:00:00");

        m_pendingFileName.clear();   // 录制完成，清除预设文件名

        m_statusLabel->setText("✓ 录制完成");
        m_statusLabel->setStyleSheet("color:#059669;font-weight:bold;font-size:14px;"
                                     "background:transparent;");

        m_btnRecord->setText("▶  开始录制");
        m_btnRecord->setProperty("state", "");
        m_btnRecord->style()->unpolish(m_btnRecord);
        m_btnRecord->style()->polish(m_btnRecord);
        updateButtonStates();

        emit recordStopped(savedTaskId, savedPath);
        emit uploadQueued(savedPath);
    }
}

// ─────────────────────────────────────────
// 「配置」按钮：打开设置界面
// ─────────────────────────────────────────
void DualRecordWidget::onConfigBtnClicked()
{
    InitConfigDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // 重新加载配置
        loadConfig();
        m_statusLabel->setText("✓ 配置已保存");
        m_statusLabel->setStyleSheet("color:#059669;font-weight:bold;font-size:14px;"
                                     "background:transparent;");
    }
}

// ─────────────────────────────────────────
// 「退出」按钮：录制中提示确认，否则直接退出
// ─────────────────────────────────────────
void DualRecordWidget::onExitBtnClicked()
{
    if (m_state == RecordState::Recording) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            this, "确认关闭",
            "当前正在录制中，关闭界面将停止录制并保存文件。\n确定要关闭界面吗？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (btn != QMessageBox::Yes)
            return;
        // 先停止录制，确保文件写入完整（stopRecord 内部会清除 pendingFileName）
        stopRecord();
    }
    // 关闭界面时清除预设文件名，避免下次吊起时使用旧文件名
    m_pendingFileName.clear();
    // 仅隐藏界面，不退出后台服务
    hide();
    emit windowHidden();
}

// ─────────────────────────────────────────
// 右上角叉关闭事件：隐藏窗口而非退出，同时清除预设文件名
// ─────────────────────────────────────────
void DualRecordWidget::closeEvent(QCloseEvent *event)
{
    // 录制中：弹确认框
    if (m_state == RecordState::Recording) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            this, "确认关闭",
            "当前正在录制中，关闭界面将停止录制并保存文件。\n确定要关闭界面吗？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (btn != QMessageBox::Yes) {
            event->ignore();   // 取消关闭
            return;
        }
        stopRecord();
    }
    // 清除预设文件名，避免下次吊起时使用旧文件名
    m_pendingFileName.clear();
    // 拦截关闭事件，改为隐藏窗口，保持后台服务运行
    event->ignore();
    hide();
    emit windowHidden();
}

// ─────────────────────────────────────────
// 事件过滤器：监听 videoContainer 的 Resize 事件
// ─────────────────────────────────────────
bool DualRecordWidget::eventFilter(QObject *watched, QEvent *event)
{
#ifdef Q_OS_LINUX
    if (watched == m_videoContainer && event->type() == QEvent::Resize) {
        // 同步 GStreamer 预览窗口尺寸
        if (m_gstPreviewWidget && m_gstPreviewWidget->isVisible()) {
            m_gstPreviewWidget->setGeometry(m_videoContainer->rect());
        }
    }
#else
    Q_UNUSED(watched)
    Q_UNUSED(event)
#endif
    return QWidget::eventFilter(watched, event);
}

// ─────────────────────────────────────────
// 打开/隐藏主界面（TCP 接口调用）
// ─────────────────────────────────────────
void DualRecordWidget::showWindow()
{
    // 恢复窗口（如果最小化）并激活到前台
    if (isMinimized())
        showNormal();
    else
        show();
    raise();
    activateWindow();
    emit windowShown();
}

void DualRecordWidget::hideWindow()
{
    hide();
    emit windowHidden();
}

// ─────────────────────────────────────────
// 播放器
// ─────────────────────────────────────────
void DualRecordWidget::playVideo(const QString &filePath)
{
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        QMessageBox::warning(this, "提示",
            QString("文件不存在:\n%1").arg(filePath));
        return;
    }
    PlayerWindow *player = new PlayerWindow(filePath, this);
    player->setAttribute(Qt::WA_DeleteOnClose);
    player->show();
}

// ─────────────────────────────────────────
// 计时更新
// ─────────────────────────────────────────
void DualRecordWidget::updateDuration()
{
    if (m_state == RecordState::Recording) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_recordingStartMs;
        m_durationLabel->setText(formatDuration(elapsed));
    }
}

QString DualRecordWidget::formatDuration(qint64 ms)
{
    int s = static_cast<int>(ms / 1000);
    return QString("%1:%2:%3")
        .arg(s / 3600,          2, 10, QChar('0'))
        .arg((s % 3600) / 60,   2, 10, QChar('0'))
        .arg(s % 60,            2, 10, QChar('0'));
}

// ─────────────────────────────────────────
// MediaRecorder 状态/错误回调
// ─────────────────────────────────────────
void DualRecordWidget::onMediaRecorderStateChanged(QMediaRecorder::State state)
{
    LOG(QString("[Recorder] stateChanged -> %1").arg(state));
    switch (state) {
    case QMediaRecorder::RecordingState:
        LOG("[Recorder] 状态: RecordingState — 录制已真正开始，文件: " + m_currentFilePath
            + "  容器: " + m_actualContainer);
        // 停止超时计时器（录制已确认开始）
        m_recordStartTimer->stop();

        // 更新 UI 为"录制中"
        if (m_state == RecordState::Recording) {
            m_statusLabel->setText("● 录制中");
            m_statusLabel->setStyleSheet("color:#dc2626;font-weight:bold;font-size:14px;"
                                         "background:transparent;");
        }
        break;

    case QMediaRecorder::PausedState:
        LOG("[Recorder] 状态: PausedState");
        break;

    case QMediaRecorder::StoppedState:
        LOG("[Recorder] 状态: StoppedState — 已停止落盘，文件: " + m_currentFilePath);
        // 录制器已完全落盘，现在可以安全关闭摄像头
        if (m_pendingCloseCamera) {
            m_pendingCloseCamera = false;
            closeCamera();
        }
        break;
    }
}

void DualRecordWidget::onMediaRecorderStatusChanged(QMediaRecorder::Status status)
{
    // status: UnloadedStatus / LoadingStatus / LoadedStatus /
    //         StartingStatus / RecordingStatus / PausedStatus / FinalizingStatus
    LOG(QString("[Recorder] statusChanged -> %1 state=%2").arg(status).arg(m_recorder ? m_recorder->state() : -1));
    if (status == QMediaRecorder::UnloadedStatus) {
        LOG("[Recorder] !!! 录制器进入 UnloadedStatus — 可能表示 media service 初始化失败");
    }
}

void DualRecordWidget::onMediaRecorderError(QMediaRecorder::Error error)
{
    QString msg = m_recorder ? m_recorder->errorString() : "未知错误";
    LOG(QString("[Recorder] !!! ERROR code=%1 msg='%2' state=%3 status=%4 container=%5 output=%6")
        .arg(error).arg(msg)
        .arg(m_recorder ? m_recorder->state() : -1)
        .arg(m_recorder ? m_recorder->status() : -1)
        .arg(m_actualContainer).arg(m_currentFilePath));
    m_recordStartTimer->stop();
    m_statusLabel->setText("⚠ 录制错误: " + msg);
    m_statusLabel->setStyleSheet("color:#dc2626;font-weight:bold;font-size:14px;"
                                 "background:transparent;");
    m_durationTimer->stop();
    m_durationLabel->setText("00:00:00");
    m_state = RecordState::Idle;
    m_btnRecord->setText("▶  开始录制");
    m_btnRecord->setProperty("state", "");
    m_btnRecord->style()->unpolish(m_btnRecord);
    m_btnRecord->style()->polish(m_btnRecord);
    m_btnRecord->setEnabled(true);
    updateButtonStates();
    // closeCamera 会先清理 recorder 再删 camera
    closeCamera();
    emit recordError(m_currentTaskId, msg);
}

// ─────────────────────────────────────────
// 按钮状态管理
// ─────────────────────────────────────────
void DualRecordWidget::updateButtonStates()
{
    bool active   = (m_state == RecordState::Recording);
    bool idle     = (m_state == RecordState::Idle || m_state == RecordState::Stopped);

    // ── btnRecord：始终可点，文字/颜色跟随状态 ──
    m_btnRecord->setEnabled(true);
    if (active) {
        m_btnRecord->setText("■  停止录制");
        m_btnRecord->setProperty("state", "stop");
    } else {
        m_btnRecord->setText("▶  开始录制");
        m_btnRecord->setProperty("state", "");
    }
    m_btnRecord->style()->unpolish(m_btnRecord);
    m_btnRecord->style()->polish(m_btnRecord);

    // 录制中所有设备选择器均禁用（切换摄像头会导致多文件问题）
    m_cameraSelector->setEnabled(idle);
    m_audioSelector->setEnabled(idle);
    m_resolutionSelector->setEnabled(idle);
}

// ─────────────────────────────────────────
// 配置加载/保存
// ─────────────────────────────────────────
void DualRecordWidget::loadConfig()
{
    m_videoCachePath = m_settings->value(
        "cache/video_path",
        QDir::homePath() + "/BankDualRecord/videos").toString();
}

void DualRecordWidget::saveConfig()
{
    m_settings->setValue("cache/video_path", m_videoCachePath);
    m_settings->sync();
}

void DualRecordWidget::initConfig(const QString &configPath)
{
    QSettings cfg(configPath, QSettings::IniFormat);
    m_videoCachePath = cfg.value("cache/video_path", m_videoCachePath).toString();
    QDir().mkpath(m_videoCachePath);
    saveConfig();
}

// ─────────────────────────────────────────
// 预设文件名接口
// ─────────────────────────────────────────
void DualRecordWidget::setPendingFileName(const QString &fileName)
{
    m_pendingFileName = fileName.trimmed();
}

void DualRecordWidget::clearPendingFileName()
{
    m_pendingFileName.clear();
}

QString DualRecordWidget::getPendingFileName() const
{
    return m_pendingFileName;
}

// ─────────────────────────────────────────
// 公共接口（TCP 调用入口）
// ─────────────────────────────────────────
void DualRecordWidget::startRecord(const QString &taskId, const QString &custName, const QString &fileName)
{
    m_currentTaskId = taskId;
    Q_UNUSED(custName)
    // 设置预设文件名（空则使用自动生成规则）
    m_pendingFileName = fileName.trimmed();
    if (m_state == RecordState::Idle || m_state == RecordState::Stopped)
        onRecordBtnClicked();
}

void DualRecordWidget::stopRecord()
{
    if (m_state == RecordState::Recording)
        onRecordBtnClicked();
}

RecordState DualRecordWidget::getState() const        { return m_state; }
QString     DualRecordWidget::getCurrentFilePath() const { return m_currentFilePath; }

// ─────────────────────────────────────────
// 工具方法
// ─────────────────────────────────────────
QString DualRecordWidget::generateFileName(const QString &taskId)
{
    // 如果预设了文件名，优先使用
    if (!m_pendingFileName.isEmpty()) {
        QString name = m_pendingFileName;
        // 若不含扩展名，使用默认扩展名（setupRecorder 中会修正）
        if (!name.contains('.')) {
#ifdef Q_OS_WIN
            name += ".mp4";
#else
            name += ".mp4";  // Linux 使用 GStreamer x264enc+avenc_aac+mp4mux，输出 MP4
#endif
        }
        return name;
    }


    // 默认规则：{taskId}_{yyyyMMdd_HHmmss}.{ext}
    QString ts  = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString tid = taskId.isEmpty() ? "REC" : taskId;
#ifdef Q_OS_WIN
    return QString("%1_%2.mp4").arg(tid, ts);
#else
    return QString("%1_%2.mp4").arg(tid, ts);  // Linux 使用 GStreamer x264enc+avenc_aac+mp4mux
#endif
}

// ─────────────────────────────────────────
// 录制器配置（统一编码器/容器/信号设置）
//
// 关键修复（v2.9）：
//   1. 根据容器格式显式指定匹配的 codec（避免 GStreamer 自动选择不兼容编码器导致管线静默失败）
//   2. Linux 优先 webm（VP8+Vorbis），比 ogg/Theora 更稳定
//   3. 用 setEncodingSettings(audio, video, containerMime) 三参数合并设置（Qt5 推荐方式）
// ─────────────────────────────────────────
void DualRecordWidget::setupRecorder(const QString &outputPath)
{
    if (m_recorder) {
        disconnect(m_recorder, nullptr, this, nullptr);
        delete m_recorder;
        m_recorder = nullptr;
    }

    // QMediaRecorder 需要 QCamera 作为 mediaObject
    m_recorder = new QMediaRecorder(m_camera, this);
    if (!m_recorder) {
        LOG("[Recorder] 创建 QMediaRecorder 失败");
        return;
    }

    LOG(QString("[Recorder] QMediaRecorder 创建成功，available=%1").arg(m_recorder->isAvailable() ? "YES" : "NO"));

    // ── 第一步：探测实际可用的容器格式（MIME 格式） ──
    m_actualContainer = detectContainerFormat();
    LOG("[Recorder] 选中容器格式: " + m_actualContainer);

    // ── 第二步：根据容器修正输出文件扩展名 ──
    QString correctExt = containerToExtension(m_actualContainer);
    QString currentExt = QFileInfo(outputPath).suffix().toLower();
    QString finalPath = outputPath;

    if (currentExt.isEmpty()) {
        finalPath = outputPath + correctExt;
    } else {
        // 比较扩展名是否匹配容器
        QString expectedExtBase = correctExt.mid(1);  // 去掉 "."
        if (currentExt != expectedExtBase
            && !(currentExt == "mp4" && expectedExtBase == "mpeg4")  // mp4/mpeg4 等价
            && !(currentExt == "ogg" && expectedExtBase == "ogv")) {  // ogg/ogv 兼容
            // 扩展名不匹配 → 替换（保留目录路径，使用 QDir 拼接）
            QFileInfo fi(outputPath);
            QDir dir(fi.absolutePath());
            finalPath = dir.filePath(fi.completeBaseName() + correctExt);
            LOG("[Recorder] 扩展名修正: " + outputPath + " -> " + finalPath);
        }
    }
    m_currentFilePath = finalPath;

    // ── 第三步：配置视频编码参数 ──
    QVideoEncoderSettings vSettings;
    // v2.9-fix6: 不指定任何 codec/参数，让 GStreamer 完全自动协商
    // 之前 4 种显式 codec 组合全部闪停，问题不在 codec 选择
    QSize res = m_resolutionSelector->currentData().toSize();
    vSettings.setResolution(res.isValid() ? res : QSize(1280, 720));
    vSettings.setFrameRate(0);       // 0 = 自动
    vSettings.setBitRate(0);         // 0 = 自动
    vSettings.setQuality(QMultimedia::NormalQuality);

    // ── 第四步：配置音频编码参数 ──
    QAudioEncoderSettings aSettings;
    // v2.9-fix6: 不指定任何 codec/参数，完全自动协商
    aSettings.setSampleRate(0);      // 0 = 自动
    aSettings.setBitRate(0);         // 0 = 自动
    aSettings.setChannelCount(0);    // 0 = 自动
    aSettings.setQuality(QMultimedia::NormalQuality);

    // ── 第五步（v2.9 新增）：根据容器格式显式匹配编码器 ──
    // v2.9-fix6: 跳过 codec 匹配！所有显式 codec 组合在 Linux GStreamer 均闪停
    // selectMatchingCodecs(m_actualContainer, vSettings, aSettings);  // ← 暂时禁用

    // ── 第六步：用 setEncodingSettings 三参数方法统一设置（Qt5 推荐方式） ──
    // v2.9-fix6: 不指定 codec，让 GStreamer 全自动协商音视频编码器
    LOG(QString("[Recorder] setEncodingSettings — video codec=auto audio codec=auto container=%1 (全自动模式)")
        .arg(m_actualContainer));
    m_recorder->setEncodingSettings(aSettings, vSettings, m_actualContainer);

    // ── 第七步：设置输出位置 ──
    m_recorder->setOutputLocation(QUrl::fromLocalFile(finalPath));

    // ── 诊断日志 ──
    LOG("[Recorder] 输出文件: " + finalPath);
    LOG(QString("[Recorder] recorder state=%1 status=%2").arg(m_recorder->state()).arg(m_recorder->status()));

    // ── 第八步：连接信号 ──
    connect(m_recorder, &QMediaRecorder::stateChanged,
            this, &DualRecordWidget::onMediaRecorderStateChanged);
    connect(m_recorder, &QMediaRecorder::statusChanged,
            this, &DualRecordWidget::onMediaRecorderStatusChanged);
    connect(m_recorder,
            QOverload<QMediaRecorder::Error>::of(&QMediaRecorder::error),
            this, &DualRecordWidget::onMediaRecorderError);
}
