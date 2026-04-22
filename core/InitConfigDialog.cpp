/**
 * @file InitConfigDialog.cpp
 * @brief 双录控件配置界面实现
 *        - 两个页签：「基础设置」和「FTP/SFTP 上传」
 *        - 基础设置：缓存路径 + TCP端口 + 上传策略
 *        - FTP/SFTP上传：协议 + 服务器信息 + 测试连接
 */

#include "InitConfigDialog.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QSettings>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

// libssh2 (SFTP连接测试)
#ifdef USE_LIBSSH2
#  include <libssh2.h>
#  include <libssh2_sftp.h>
#endif

#ifdef Q_OS_WIN
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#else
#  include <sys/socket.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <arpa/inet.h>
#endif

InitConfigDialog::InitConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    m_configPath = QDir::homePath() + "/BankDualRecord/config.ini";

    // 测试按钮颜色自动复原计时器（单次触发，3秒后恢复绿色）
    m_testBtnResetTimer = new QTimer(this);
    m_testBtnResetTimer->setSingleShot(true);
    connect(m_testBtnResetTimer, &QTimer::timeout, this, [this]() {
        m_testBtn->setText("🔌  测试连接");
        m_testBtn->setStyleSheet(
            "QPushButton#testBtn {"
            "  background-color:#16a34a; color:white; border:none;"
            "  font-size:14px; font-weight:bold; border-radius:6px;"
            "}"
            "QPushButton#testBtn:hover { background-color:#15803d; color:white; }"
        );
    });

    setupUI();
    loadExistingConfig();
    setWindowTitle("双录控件 — 参数配置");
    setWindowIcon(QApplication::windowIcon());  // 继承全局应用图标
    setMinimumWidth(560);
    setMinimumHeight(480);
    setModal(true);
    resize(600, 560);
}

void InitConfigDialog::setupUI()
{
    setStyleSheet(R"(
        QDialog {
            background: #f0f2f5;
            color: #1f2937;
            font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif;
            font-size: 13px;
        }
        QTabWidget::pane {
            background: #f0f2f5;
            border: none;
        }
        QTabBar::tab {
            background: #e5e7eb;
            color: #6b7280;
            border: none;
            padding: 10px 28px;
            font-size: 13px;
            font-weight: bold;
            min-width: 120px;
        }
        QTabBar::tab:selected {
            background: #ffffff;
            color: #1a56db;
            border-bottom: 3px solid #1a56db;
        }
        QTabBar::tab:hover:!selected { background: #d1d5db; }
        QGroupBox {
            border: 1px solid #d1d5db;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 18px;
            background: #ffffff;
            font-weight: bold;
            color: #374151;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            color: #1a56db;
            font-size: 13px;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background: #f9fafb;
            border: 1px solid #d1d5db;
            border-radius: 6px;
            padding: 6px 10px;
            color: #1f2937;
            font-size: 13px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border-color: #1a56db;
            background: #eff6ff;
        }
        QPushButton {
            background: #1a56db;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 7px 16px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton:hover { background: #1340a8; color: white; }
        QPushButton#saveBtn              { background: #059669; color: white; border: none; }
        QPushButton#saveBtn:hover        { background: #047857; color: white; }
        QPushButton#cancelBtn            { background: #6b7280; color: white; border: none; }
        QPushButton#cancelBtn:hover      { background: #4b5563; color: white; }
        QPushButton#testBtn              { background-color: #16a34a !important; color: white; border: none; font-size: 14px; font-weight: bold; }
        QPushButton#testBtn:hover        { background-color: #15803d !important; color: white; }
        QPushButton#browseBtn {
            background: #e5e7eb;
            color: #374151;
            padding: 6px 12px;
            border: 1px solid #d1d5db;
            border-radius: 6px;
            font-weight: normal;
        }
        QPushButton#browseBtn:hover { background: #d1d5db; color: #1f2937; }
    )");

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // ── 顶部标题 ──
    QWidget *header = new QWidget;
    header->setStyleSheet("background:#1a56db;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 13, 20, 13);
    QLabel *titleLabel = new QLabel("⚙  双录控件 — 参数配置");
    titleLabel->setStyleSheet("font-size:15px;font-weight:bold;color:white;"
                              "background:transparent;");
    headerLayout->addWidget(titleLabel);
    outerLayout->addWidget(header);

    // ── 页签区域 ──
    QTabWidget *tabs = new QTabWidget;
    tabs->setDocumentMode(true);
    tabs->setContentsMargins(0, 0, 0, 0);

    // ═══════════════════════════════════════════
    // 页签1：基础设置
    // ═══════════════════════════════════════════
    // 使用 QScrollArea 包裹内容，避免窗口高度不足时文字截断
    QScrollArea *tab1Scroll = new QScrollArea;
    tab1Scroll->setWidgetResizable(true);
    tab1Scroll->setFrameShape(QFrame::NoFrame);
    tab1Scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tab1Scroll->setStyleSheet("QScrollArea { background: #f0f2f5; border: none; }"
                              "QScrollBar:vertical { width: 6px; background: #e5e7eb; border-radius: 3px; }"
                              "QScrollBar::handle:vertical { background: #9ca3af; border-radius: 3px; }"
                              "QScrollBar::handle:vertical:hover { background: #6b7280; }");

    QWidget *tab1 = new QWidget;
    tab1->setStyleSheet("background:#f0f2f5;");
    QVBoxLayout *tab1Layout = new QVBoxLayout(tab1);
    tab1Layout->setContentsMargins(20, 16, 20, 16);
    tab1Layout->setSpacing(14);

    // ─── 影像缓存配置 ───
    QGroupBox *cacheGroup = new QGroupBox("📁  影像缓存配置");
    QFormLayout *cfl = new QFormLayout(cacheGroup);
    cfl->setContentsMargins(16, 8, 16, 14);
    cfl->setSpacing(10);
    cfl->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_videoCachePath = new QLineEdit(QDir::homePath() + "/BankDualRecord/videos");
    m_videoCachePath->setPlaceholderText("视频文件保存路径");
    m_browseCacheBtn = new QPushButton("浏览...");
    m_browseCacheBtn->setObjectName("browseBtn");
    m_browseCacheBtn->setFixedWidth(72);
    connect(m_browseCacheBtn, &QPushButton::clicked,
            this, &InitConfigDialog::onBrowseCachePath);
    QHBoxLayout *cacheRow = new QHBoxLayout;
    cacheRow->setSpacing(8);
    cacheRow->addWidget(m_videoCachePath);
    cacheRow->addWidget(m_browseCacheBtn);
    cfl->addRow("视频缓存路径:", cacheRow);
    tab1Layout->addWidget(cacheGroup);

    // ─── 服务配置 ───
    QGroupBox *svcGroup = new QGroupBox("🔌  服务配置");
    QFormLayout *sfl = new QFormLayout(svcGroup);
    sfl->setContentsMargins(16, 8, 16, 14);
    sfl->setSpacing(10);
    sfl->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_tcpPort = new QSpinBox;
    m_tcpPort->setRange(1024, 65535);
    m_tcpPort->setValue(9527);
    m_tcpPort->setToolTip("双录控件监听的本地TCP端口，默认9527");
    sfl->addRow("TCP 监听端口:", m_tcpPort);

    m_httpPort = new QSpinBox;
    m_httpPort->setRange(1024, 65535);
    m_httpPort->setValue(9528);
    m_httpPort->setToolTip("HTTP/SSE 服务监听端口，默认9528；接口与TCP完全一致，支持 POST /cmd、GET /status 等");
    sfl->addRow("HTTP 监听端口:", m_httpPort);
    tab1Layout->addWidget(svcGroup);

    // ─── 上传策略 ───
    QGroupBox *policyGroup = new QGroupBox("📊  上传策略配置");
    QFormLayout *pfl = new QFormLayout(policyGroup);
    pfl->setContentsMargins(16, 8, 16, 14);
    pfl->setSpacing(10);
    pfl->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_bandwidthKb = new QSpinBox;
    m_bandwidthKb->setRange(10, 102400);
    m_bandwidthKb->setValue(500);
    m_bandwidthKb->setSuffix(" KB/s");
    m_bandwidthKb->setToolTip("限制上传占用带宽（10~102400 KB/s）");
    pfl->addRow("上传带宽限制:", m_bandwidthKb);

    m_maxRetries = new QSpinBox;
    m_maxRetries->setRange(0, 20);
    m_maxRetries->setValue(5);
    m_maxRetries->setSuffix(" 次");
    m_maxRetries->setToolTip("上传失败后的最大自动重试次数，0=不重试");
    pfl->addRow("最大重试次数:", m_maxRetries);

    m_retryInterval = new QSpinBox;
    m_retryInterval->setRange(1, 3600);
    m_retryInterval->setValue(30);
    m_retryInterval->setSuffix(" 秒");
    m_retryInterval->setToolTip("每次重试之间的基础等待时间（秒）");
    pfl->addRow("重试间隔:", m_retryInterval);
    tab1Layout->addWidget(policyGroup);

    // ─── 日志配置 ───
    QGroupBox *logGroup = new QGroupBox("📝  日志配置");
    QFormLayout *lfl = new QFormLayout(logGroup);
    lfl->setContentsMargins(16, 8, 16, 14);
    lfl->setSpacing(10);
    lfl->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_logEnabled = new QCheckBox("启用日志记录");
    m_logEnabled->setChecked(true);
    m_logEnabled->setToolTip("关闭后仅输出到控制台，不写日志文件");
    lfl->addRow("记录日志:", m_logEnabled);

    m_logDir = new QLineEdit;
    m_logDir->setText(QDir::homePath() + "/BankDualRecord/logs");
    m_logDir->setToolTip("日志文件存放目录，按日期自动滚动");
    QPushButton *browseLogBtn = new QPushButton("浏览...");
    browseLogBtn->setFixedWidth(70);
    browseLogBtn->setStyleSheet("QPushButton{background:#4299e1;color:white;border:none;border-radius:4px;padding:6px 10px;}QPushButton:hover{background:#3182ce;}");
    connect(browseLogBtn, &QPushButton::clicked, this, &InitConfigDialog::onBrowseLogDir);
    QHBoxLayout *logDirRow = new QHBoxLayout;
    logDirRow->setSpacing(8);
    logDirRow->addWidget(m_logDir);
    logDirRow->addWidget(browseLogBtn);
    lfl->addRow("日志目录:", logDirRow);
    tab1Layout->addWidget(logGroup);

    tab1Layout->addStretch();
    tab1Scroll->setWidget(tab1);
    tabs->addTab(tab1Scroll, "📋  基础设置");

    // ═══════════════════════════════════════════
    // 页签2：FTP/SFTP 上传
    // ═══════════════════════════════════════════
    QWidget *tab2 = new QWidget;
    tab2->setStyleSheet("background:#f0f2f5;");
    QVBoxLayout *tab2Layout = new QVBoxLayout(tab2);
    tab2Layout->setContentsMargins(20, 16, 20, 16);
    tab2Layout->setSpacing(14);

    QGroupBox *uploadGroup = new QGroupBox("☁  FTP/SFTP 服务器配置");
    QFormLayout *ufl = new QFormLayout(uploadGroup);
    ufl->setContentsMargins(16, 8, 16, 14);
    ufl->setSpacing(10);
    ufl->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_protocol = new QComboBox;
    m_protocol->addItems({"ftp", "sftp"});
    m_protocol->setToolTip("选择上传协议，SFTP 默认端口 22");
    ufl->addRow("上传协议:", m_protocol);

    m_host = new QLineEdit;
    m_host->setPlaceholderText("192.168.1.100");
    ufl->addRow("服务器地址:", m_host);

    m_port = new QSpinBox;
    m_port->setRange(1, 65535);
    m_port->setValue(21);
    ufl->addRow("服务器端口:", m_port);

    m_user = new QLineEdit;
    m_user->setPlaceholderText("ftpuser");
    ufl->addRow("用户名:", m_user);

    m_pass = new QLineEdit;
    m_pass->setEchoMode(QLineEdit::Password);
    m_pass->setPlaceholderText("••••••••");
    ufl->addRow("密码:", m_pass);

    m_remoteDir = new QLineEdit("/dualrecord/");
    m_remoteDir->setPlaceholderText("/dualrecord/");
    ufl->addRow("远端目录:", m_remoteDir);

    // 协议切换自动调整端口
    connect(m_protocol, &QComboBox::currentTextChanged, [this](const QString &p) {
        m_port->setValue(p == "sftp" ? 22 : 21);
    });

    // 测试连接按钮（居中独立一行，醒目绿色）
    m_testBtn = new QPushButton("🔌  测试连接");
    m_testBtn->setObjectName("testBtn");
    m_testBtn->setFixedHeight(42);
    m_testBtn->setFixedWidth(160);
    // 内联样式强制设置默认绿色背景（优先级高于全局 setStyleSheet）
    m_testBtn->setStyleSheet(
        "background-color:#16a34a; color:white; border:none;"
        "font-size:14px; font-weight:bold; border-radius:6px;"
    );
    connect(m_testBtn, &QPushButton::clicked,
            this, &InitConfigDialog::onTestConnection);
    QHBoxLayout *testRow = new QHBoxLayout;
    testRow->addStretch();
    testRow->addWidget(m_testBtn);
    testRow->addStretch();
    ufl->addRow("", testRow);

    tab2Layout->addWidget(uploadGroup);
    tab2Layout->addStretch();
    tabs->addTab(tab2, "☁  FTP/SFTP 上传");

    outerLayout->addWidget(tabs, 1);

    // ── 底部操作按钮栏 ──
    QWidget *footer = new QWidget;
    footer->setObjectName("footerBar");
    footer->setStyleSheet("QWidget#footerBar { background:#ffffff; border-top:1px solid #e5e7eb; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 12, 20, 12);
    footerLayout->setSpacing(10);

    m_cancelBtn = new QPushButton("✘  取消");
    m_cancelBtn->setObjectName("cancelBtn");
    m_cancelBtn->setFixedHeight(38);
    m_cancelBtn->setFixedWidth(100);

    m_saveBtn = new QPushButton("✔  保存配置");
    m_saveBtn->setObjectName("saveBtn");
    m_saveBtn->setFixedHeight(38);
    m_saveBtn->setFixedWidth(120);

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_saveBtn,   &QPushButton::clicked, this, &InitConfigDialog::onSaveConfig);

    footerLayout->addStretch();
    footerLayout->addWidget(m_cancelBtn);
    footerLayout->addWidget(m_saveBtn);
    outerLayout->addWidget(footer);
}

void InitConfigDialog::loadExistingConfig()
{
    if (!QFile::exists(m_configPath)) return;
    QSettings cfg(m_configPath, QSettings::IniFormat);

    m_videoCachePath->setText(cfg.value("cache/video_path",
        m_videoCachePath->text()).toString());
    m_tcpPort->setValue(cfg.value("server/tcp_port", 9527).toInt());
    m_httpPort->setValue(cfg.value("server/http_port", 9528).toInt());

    m_protocol->setCurrentText(cfg.value("upload/protocol", "ftp").toString());
    m_host->setText(cfg.value("upload/ftp_host").toString());
    m_port->setValue(cfg.value("upload/ftp_port", 21).toInt());
    m_user->setText(cfg.value("upload/ftp_user").toString());
    m_pass->setText(cfg.value("upload/ftp_pass").toString());
    m_remoteDir->setText(cfg.value("upload/ftp_remote_dir", "/dualrecord/").toString());

    m_bandwidthKb->setValue(cfg.value("upload/bandwidth_limit_kb", 500).toInt());
    m_maxRetries->setValue(cfg.value("upload/max_retries", 5).toInt());
    m_retryInterval->setValue(cfg.value("upload/retry_interval_sec", 30).toInt());

    m_logEnabled->setChecked(cfg.value("log/enabled", true).toBool());
    QString logDir = cfg.value("log/dir").toString();
    if (!logDir.isEmpty()) m_logDir->setText(logDir);
}

void InitConfigDialog::onBrowseCachePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择视频缓存路径",
        m_videoCachePath->text());
    if (!dir.isEmpty()) m_videoCachePath->setText(dir);
}

void InitConfigDialog::onBrowseLogDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择日志目录",
        m_logDir->text());
    if (!dir.isEmpty()) m_logDir->setText(dir);
}

void InitConfigDialog::onTestConnection()
{
    // ── 参数预检 ──
    if (m_host->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先填写服务器地址");
        return;
    }
    if (m_user->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "请先填写用户名");
        return;
    }

    QString protocol = m_protocol->currentText().toLower();
    QString host     = m_host->text().trimmed();
    int     port     = m_port->value();
    QString user     = m_user->text().trimmed();
    QString pass     = m_pass->text();

    // ── 禁用按钮 + 显示进度（灰色，避免误以为可点击）──
    m_testBtn->setEnabled(false);
    m_testBtn->setText("⏳ 连接中...");
    m_testBtn->setStyleSheet(
        "QPushButton#testBtn {"
        "  background:transparent; color:white; border:none;"
        "  font-size:14px; font-weight:bold; border-radius:6px;"
        "}"
    );

    if (protocol == "sftp") {
        testSftpConnection(host, port, user, pass);
    } else {
        testFtpConnection(host, port, user, pass);
    }
}

// ─────────────────────────────────────────
// SFTP 连接测试（libssh2）
// ─────────────────────────────────────────
void InitConfigDialog::testSftpConnection(const QString &host, int port,
                                          const QString &user, const QString &pass)
{
#ifdef USE_LIBSSH2
    int rc = libssh2_init(0);
    if (rc != 0) {
        showTestResult(false, "libssh2 初始化失败");
        return;
    }

#ifdef Q_OS_WIN
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    SOCKET sock;
#else
    int sock;
#endif

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    QString portStr   = QString::number(port);

    if (getaddrinfo(host.toStdString().c_str(),
                    portStr.toStdString().c_str(), &hints, &res) != 0) {
        libssh2_exit();
        showTestResult(false, "DNS 解析失败：" + host);
        return;
    }

    sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (::connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        freeaddrinfo(res);
        libssh2_exit();
        showTestResult(false, QString("无法连接到 %1:%2（网络不通或端口未开放）")
                            .arg(host).arg(port));
        return;
    }
    freeaddrinfo(res);

    LIBSSH2_SESSION *session = libssh2_session_init();
    if (!session) {
        libssh2_exit();
        showTestResult(false, "创建 SSH 会话失败");
        return;
    }

    // 设置超时（5秒）
    libssh2_session_set_timeout(session, 5000);

    rc = libssh2_session_handshake(session, sock);
    if (rc != 0) {
        libssh2_session_free(session);
        libssh2_exit();
        showTestResult(false, "SSH 握手失败");
        return;
    }

    // 验证用户名密码
    rc = libssh2_userauth_password(session,
            user.toStdString().c_str(),
            pass.toStdString().c_str());
    if (rc != 0) {
        char *errMsg = nullptr;
        int errMsgLen = 0;
        libssh2_session_last_error(session, &errMsg, &errMsgLen, 0);
        QString msg = QString("认证失败：%1").arg(errMsg ? QString::fromUtf8(errMsg, errMsgLen) : "未知错误");
        libssh2_session_disconnect(session, "Bye");
        libssh2_session_free(session);
        libssh2_exit();
        showTestResult(false, msg);
        return;
    }

    // 尝试打开 SFTP 子系统（验证服务端支持）
    LIBSSH2_SFTP *sftpSession = libssh2_sftp_init(session);
    if (!sftpSession) {
        char *errMsg = nullptr;
        int errMsgLen = 0;
        libssh2_session_last_error(session, &errMsg, &errMsgLen, 0);
        libssh2_session_disconnect(session, "Bye");
        libssh2_session_free(session);
        libssh2_exit();
        showTestResult(false, QString("SFTP 子系统初始化失败：%1").arg(errMsg ? QString::fromUtf8(errMsg, errMsgLen) : "未知错误"));
        return;
    }

    // 全部通过
    libssh2_sftp_shutdown(sftpSession);
    libssh2_session_disconnect(session, "Bye");
    libssh2_session_free(session);
    libssh2_exit();
    showTestResult(true, "SFTP 连接测试通过");
#else
    // libssh2 未编译时，降级提示
    showTestResult(false, "当前版本未编译 libssh2 支持，无法测试 SFTP 连接。\n请确认编译时已启用 USE_LIBSSH2。");
#endif
}

// ─────────────────────────────────────────
// FTP 连接测试（QNetworkAccessManager）
// ─────────────────────────────────────────
void InitConfigDialog::testFtpConnection(const QString &host, int port,
                                         const QString &user, const QString &pass)
{
    QNetworkAccessManager mgr;

    // 构建 FTP URL
    QString url = QString("ftp://%1:%2@%3:%4/")
                      .arg(user)
                      .arg(pass)
                      .arg(host)
                      .arg(port);

    QNetworkRequest req;
    req.setUrl(QUrl(url));

    // 用 list 目录来测试连接
    QNetworkReply *reply = mgr.get(req);

    // 同步等待（最多5秒）
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timeout.start(5000);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        showTestResult(false, "连接超时（5秒），请检查地址和端口");
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        showTestResult(true, "FTP 连接测试通过");
    } else {
        QString err = reply->errorString();
        // 提取常见错误
        if (err.contains("authentication", Qt::CaseInsensitive)
            || err.contains("530", Qt::CaseInsensitive)
            || err.contains("Login", Qt::CaseInsensitive)) {
            showTestResult(false, "认证失败：用户名或密码错误");
        } else if (err.contains("Connection refused", Qt::CaseInsensitive)
                   || err.contains("Connection closed", Qt::CaseInsensitive)) {
            showTestResult(false, QString("无法连接到 %1:%2（网络不通或端口未开放）").arg(host).arg(port));
        } else {
            showTestResult(false, "连接失败：" + err);
        }
    }
    reply->deleteLater();
}

// ─────────────────────────────────────────
// 显示测试结果
// ─────────────────────────────────────────
void InitConfigDialog::showTestResult(bool success, const QString &msg)
{
    m_testBtnResetTimer->stop();   // 取消上次可能残留的复原计时

    m_testBtn->setEnabled(true);

    if (success) {
        // ✅ 成功：深绿色背景 + 白色文字，按钮文字改为"✅ 连接成功"
        m_testBtn->setText("✅  连接成功");
        m_testBtn->setStyleSheet(
            "QPushButton#testBtn {"
            "  background:#166534; color:#ffffff; border:none;"
            "  font-size:14px; font-weight:bold; border-radius:6px;"
            "}"
            "QPushButton#testBtn:hover { background:#14532d; color:white; }"
        );
        QMessageBox::information(this, "连接测试",
            QString("✅ %1\n\n协议：%2\n地址：%3:%4")
                .arg(msg, m_protocol->currentText().toUpper(),
                     m_host->text().trimmed(), QString::number(m_port->value())));
    } else {
        // ❌ 失败：红色背景 + 白色文字，按钮文字改为"❌ 连接失败"
        m_testBtn->setText("❌  连接失败");
        m_testBtn->setStyleSheet(
            "QPushButton#testBtn {"
            "  background:#dc2626; color:#ffffff; border:none;"
            "  font-size:14px; font-weight:bold; border-radius:6px;"
            "}"
            "QPushButton#testBtn:hover { background:#b91c1c; color:white; }"
        );
        QMessageBox::critical(this, "连接测试",
            QString("❌ %1").arg(msg));
    }

    // 3秒后自动恢复为原始绿色"测试连接"状态
    m_testBtnResetTimer->start(3000);
}

bool InitConfigDialog::validateInputs()
{
    if (m_videoCachePath->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "验证失败", "「基础设置」- 请填写视频缓存路径");
        return false;
    }
    return true;
}

void InitConfigDialog::onSaveConfig()
{
    if (!validateInputs()) return;

    QDir().mkpath(QFileInfo(m_configPath).absolutePath());
    QDir().mkpath(m_videoCachePath->text());

    QSettings cfg(m_configPath, QSettings::IniFormat);
    cfg.setValue("cache/video_path",          m_videoCachePath->text());
    cfg.setValue("server/tcp_port",           m_tcpPort->value());
    cfg.setValue("server/http_port",          m_httpPort->value());
    cfg.setValue("upload/protocol",           m_protocol->currentText());
    cfg.setValue("upload/ftp_host",           m_host->text());
    cfg.setValue("upload/ftp_port",           m_port->value());
    cfg.setValue("upload/ftp_user",           m_user->text());
    cfg.setValue("upload/ftp_pass",           m_pass->text());
    cfg.setValue("upload/ftp_remote_dir",     m_remoteDir->text());
    cfg.setValue("upload/bandwidth_limit_kb", m_bandwidthKb->value());
    cfg.setValue("upload/max_retries",        m_maxRetries->value());
    cfg.setValue("upload/retry_interval_sec", m_retryInterval->value());

    // 日志配置
    cfg.setValue("log/enabled", m_logEnabled->isChecked());
    cfg.setValue("log/dir",     m_logDir->text());
    cfg.sync();

    // 同步到 QSettings（供主控件读取）
    QSettings appSettings("BankSystem", "DualRecord");
    appSettings.setValue("cache/video_path", m_videoCachePath->text());
    appSettings.sync();

    accept();
}
