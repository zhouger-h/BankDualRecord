/**
 * @file DemoWindow.cpp
 * @brief 双录控件 Demo 演示程序实现
 */

#include "DemoWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QDir>

DemoWindow::DemoWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_socket(new QTcpSocket(this))
    , m_heartbeatTimer(new QTimer(this))
    , m_connected(false)
{
    setupUI();
    setupConnections();
    setWindowTitle("银行柜面双录控件 — Demo 演示程序 v1.0");
    setMinimumSize(1100, 700);
    resize(1200, 780);
}

DemoWindow::~DemoWindow()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();
}

// ─────────────────────────────────────────
// UI 搭建
// ─────────────────────────────────────────
void DemoWindow::setupUI()
{
    setStyleSheet(R"(
        QMainWindow,QWidget { background:#0f172a; color:#e2e8f0;
            font-family:'Microsoft YaHei',Consolas,sans-serif; font-size:13px; }
        QGroupBox { border:1px solid #334155; border-radius:8px;
                    margin-top:8px; padding-top:16px; color:#38bdf8; font-weight:bold; }
        QGroupBox::title { subcontrol-origin:margin; left:10px; padding:0 6px; }
        QPushButton { background:#1e40af; color:white; border:none; border-radius:6px;
                      padding:8px 16px; font-weight:bold; min-width:90px; }
        QPushButton:hover  { background:#2563eb; }
        QPushButton:disabled { background:#334155; color:#64748b; }
        QPushButton#btnConnect    { background:#059669; }
        QPushButton#btnConnect:hover { background:#10b981; }
        QPushButton#btnDisconnect { background:#dc2626; }
        QPushButton#btnDisconnect:hover { background:#ef4444; }
        QPushButton#btnStart  { background:#15803d; }
        QPushButton#btnStart:hover  { background:#16a34a; }
        QPushButton#btnStop   { background:#b91c1c; }
        QPushButton#btnStop:hover   { background:#dc2626; }
        QLineEdit,QSpinBox { background:#1e293b; border:1px solid #334155;
            border-radius:5px; padding:5px 10px; color:#e2e8f0; }
        QTextEdit { background:#0a0f1e; border:1px solid #1e293b;
                    border-radius:6px; color:#94a3b8; font-family:Consolas,monospace; font-size:12px; }
        QTabWidget::pane { border:1px solid #334155; border-radius:6px; }
        QTabBar::tab { background:#1e293b; color:#94a3b8;
                       padding:8px 18px; border-radius:4px 4px 0 0; }
        QTabBar::tab:selected { background:#1e40af; color:white; }
        QLabel#statusConnected    { color:#10b981; font-weight:bold; }
        QLabel#statusDisconnected { color:#ef4444; font-weight:bold; }
    )");

    QWidget *central = new QWidget;
    setCentralWidget(central);
    QHBoxLayout *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    // ══════════════════════════════════════
    // 左栏：连接控制 + 操作面板
    // ══════════════════════════════════════
    QWidget *leftPanel = new QWidget;
    leftPanel->setMaximumWidth(380);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // ── 标题 ──
    QLabel *titleLabel = new QLabel("🎬 双录控件集成 Demo");
    titleLabel->setStyleSheet("font-size:18px;font-weight:bold;color:#38bdf8;padding:6px;");
    leftLayout->addWidget(titleLabel);

    // ── 连接设置 ──
    QGroupBox *connGroup = new QGroupBox("连接设置");
    QFormLayout *connForm = new QFormLayout(connGroup);
    m_hostEdit = new QLineEdit("127.0.0.1");
    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(9527);
    connForm->addRow("服务地址:", m_hostEdit);
    connForm->addRow("TCP端口:", m_portSpin);

    QHBoxLayout *connBtnRow = new QHBoxLayout;
    m_connectBtn    = new QPushButton("🔌 连接");    m_connectBtn->setObjectName("btnConnect");
    m_disconnectBtn = new QPushButton("✖ 断开");   m_disconnectBtn->setObjectName("btnDisconnect");
    m_disconnectBtn->setEnabled(false);
    connBtnRow->addWidget(m_connectBtn);
    connBtnRow->addWidget(m_disconnectBtn);
    connForm->addRow(connBtnRow);

    m_statusLabel = new QLabel("● 未连接");
    m_statusLabel->setObjectName("statusDisconnected");
    connForm->addRow("状态:", m_statusLabel);
    leftLayout->addWidget(connGroup);

    // ── Tabs: 操作 + 初始化 ──
    m_tabs = new QTabWidget;

    // Tab1: 录制操作
    QWidget *opTab = new QWidget;
    QVBoxLayout *opLayout = new QVBoxLayout(opTab);

    QGroupBox *taskGroup = new QGroupBox("任务信息");
    QFormLayout *taskForm = new QFormLayout(taskGroup);
    m_taskIdEdit   = new QLineEdit("TASK_20260330_001");
    m_custNameEdit = new QLineEdit("张三");
    taskForm->addRow("任务ID:", m_taskIdEdit);
    taskForm->addRow("客户姓名:", m_custNameEdit);
    opLayout->addWidget(taskGroup);

    QGroupBox *ctrlGroup = new QGroupBox("录制控制");
    QGridLayout *ctrlGrid = new QGridLayout(ctrlGroup);

    auto mkBtn = [&](const QString &text, const QString &objName) -> QPushButton* {
        auto *b = new QPushButton(text); b->setObjectName(objName); return b;
    };

    auto *btnStart  = mkBtn("▶ 开始录制", "btnStart");
    auto *btnPause  = mkBtn("⏸ 暂停", "");
    auto *btnResume = mkBtn("▶ 继续", "");
    auto *btnStop   = mkBtn("■ 终止录制", "btnStop");
    auto *btnStatus = mkBtn("📊 查询状态", "");
    auto *btnList   = mkBtn("📋 文件列表", "");
    auto *btnPing   = mkBtn("💓 心跳测试", "");

    ctrlGrid->addWidget(btnStart,  0, 0);
    ctrlGrid->addWidget(btnPause,  0, 1);
    ctrlGrid->addWidget(btnResume, 1, 0);
    ctrlGrid->addWidget(btnStop,   1, 1);
    ctrlGrid->addWidget(btnStatus, 2, 0);
    ctrlGrid->addWidget(btnList,   2, 1);
    ctrlGrid->addWidget(btnPing,   3, 0, 1, 2);
    opLayout->addWidget(ctrlGroup);

    QGroupBox *playGroup = new QGroupBox("播放 / 删除");
    QFormLayout *playForm = new QFormLayout(playGroup);
    m_filePathEdit = new QLineEdit;
    m_filePathEdit->setPlaceholderText("视频完整路径 ...");
    auto *btnPlay   = new QPushButton("▶ 播放");
    auto *btnDelete = new QPushButton("🗑 删除");
    QHBoxLayout *playRow = new QHBoxLayout;
    playRow->addWidget(btnPlay);
    playRow->addWidget(btnDelete);
    playForm->addRow("文件路径:", m_filePathEdit);
    playForm->addRow(playRow);
    opLayout->addWidget(playGroup);

    // 信号
    connect(btnStart,  &QPushButton::clicked, this, &DemoWindow::onSendStart);
    connect(btnPause,  &QPushButton::clicked, this, &DemoWindow::onSendPause);
    connect(btnResume, &QPushButton::clicked, this, &DemoWindow::onSendResume);
    connect(btnStop,   &QPushButton::clicked, this, &DemoWindow::onSendStop);
    connect(btnStatus, &QPushButton::clicked, this, &DemoWindow::onSendStatus);
    connect(btnList,   &QPushButton::clicked, this, &DemoWindow::onSendList);
    connect(btnPing,   &QPushButton::clicked, this, &DemoWindow::onSendPing);
    connect(btnPlay,   &QPushButton::clicked, this, &DemoWindow::onSendPlay);
    connect(btnDelete, &QPushButton::clicked, [this](){ sendCommand("DELETE",{{"file_path", m_filePathEdit->text()}}); });

    m_tabs->addTab(opTab, "🎬 录制操作");

    // Tab2: 初始化配置
    QWidget *initTab = new QWidget;
    QFormLayout *initForm = new QFormLayout(initTab);
    m_videoCacheEdit = new QLineEdit(QDir::homePath() + "/BankDualRecord/videos");
    m_ftpHostEdit    = new QLineEdit("192.168.1.100");
    m_ftpPortSpin    = new QSpinBox; m_ftpPortSpin->setRange(1,65535); m_ftpPortSpin->setValue(21);
    m_ftpUserEdit    = new QLineEdit("ftpuser");
    m_ftpPassEdit    = new QLineEdit; m_ftpPassEdit->setEchoMode(QLineEdit::Password);
    m_ftpDirEdit     = new QLineEdit("/dualrecord/");
    m_bwLimitSpin    = new QSpinBox; m_bwLimitSpin->setRange(10,102400); m_bwLimitSpin->setValue(500); m_bwLimitSpin->setSuffix(" KB/s");

    initForm->addRow("视频缓存路径:", m_videoCacheEdit);
    initForm->addRow("FTP/SFTP地址:", m_ftpHostEdit);
    initForm->addRow("端口:", m_ftpPortSpin);
    initForm->addRow("用户名:", m_ftpUserEdit);
    initForm->addRow("密码:", m_ftpPassEdit);
    initForm->addRow("远端目录:", m_ftpDirEdit);
    initForm->addRow("带宽限制:", m_bwLimitSpin);

    auto *initBtn = new QPushButton("📤 发送初始化命令");
    connect(initBtn, &QPushButton::clicked, this, &DemoWindow::onSendInit);
    initForm->addRow(initBtn);
    m_tabs->addTab(initTab, "⚙ 初始化");

    leftLayout->addWidget(m_tabs);

    // ══════════════════════════════════════
    // 右栏：日志 + 协议说明
    // ══════════════════════════════════════
    QTabWidget *rightTabs = new QTabWidget;

    // Tab: 交互日志
    m_logView = new QTextEdit;
    m_logView->setReadOnly(true);
    m_logView->setPlaceholderText("TCP交互日志将显示在此处...");
    auto *clearBtn = new QPushButton("🗑 清空日志");
    clearBtn->setMaximumWidth(120);
    connect(clearBtn, &QPushButton::clicked, m_logView, &QTextEdit::clear);
    QWidget *logWidget = new QWidget;
    QVBoxLayout *logLayout = new QVBoxLayout(logWidget);
    logLayout->addWidget(m_logView);
    QHBoxLayout *logBtnRow = new QHBoxLayout;
    logBtnRow->addStretch();
    logBtnRow->addWidget(clearBtn);
    logLayout->addLayout(logBtnRow);
    rightTabs->addTab(logWidget, "📋 交互日志");

    // Tab: 协议说明
    QTextEdit *protocolDoc = new QTextEdit;
    protocolDoc->setReadOnly(true);
    protocolDoc->setHtml(R"(
<style>body{background:#0a0f1e;color:#94a3b8;font-family:'Microsoft YaHei',Consolas;}
h2{color:#38bdf8;} h3{color:#7dd3fc;} code{background:#1e293b;padding:2px 6px;border-radius:3px;color:#a5f3fc;}
pre{background:#1e293b;padding:12px;border-radius:6px;color:#86efac;overflow:auto;}
.cmd{color:#fde68a;font-weight:bold;} .note{color:#f59e0b;}</style>

<h2>🔌 双录控件 TCP 接口协议说明</h2>

<h3>帧格式</h3>
<pre>| 4字节大端长度 | JSON负载 |
例：00 00 00 2F {"cmd":"PING","params":{}}</pre>

<h3>📤 命令请求格式</h3>
<pre>{"cmd": "命令名", "params": {参数对象}}</pre>

<h3>📥 响应格式</h3>
<pre>{"code": 0, "msg": "OK", "data": {数据}, "ts": 1711111111111}</pre>

<h3>📡 事件推送格式</h3>
<pre>{"type": "event", "event": "RECORD_STARTED", "data": {...}, "ts": ...}</pre>

<h3>命令列表</h3>

<p><span class="cmd">INIT</span> — 初始化参数配置</p>
<pre>{"cmd":"INIT","params":{
  "video_cache_path": "D:/BankRecord/videos",
  "upload_protocol":  "sftp",
  "ftp_host":         "192.168.1.100",
  "ftp_port":         22,
  "ftp_user":         "upload",
  "ftp_pass":         "P@ssw0rd",
  "ftp_remote_dir":   "/dualrecord/",
  "bandwidth_limit_kb": 500
}}</pre>

<p><span class="cmd">START</span> — 开始录制</p>
<pre>{"cmd":"START","params":{"task_id":"TRX_20260101_0001","cust_name":"张三"}}</pre>

<p><span class="cmd">PAUSE</span> — 暂停录制</p>
<pre>{"cmd":"PAUSE","params":{}}</pre>

<p><span class="cmd">RESUME</span> — 继续录制</p>
<pre>{"cmd":"RESUME","params":{}}</pre>

<p><span class="cmd">STOP</span> — 终止录制（自动保存到本地，加入上传队列）</p>
<pre>{"cmd":"STOP","params":{}}</pre>

<p><span class="cmd">PLAY</span> — 播放指定视频文件</p>
<pre>{"cmd":"PLAY","params":{"file_path":"D:/BankRecord/videos/REC_TRX001_20260101.mp4"}}</pre>

<p><span class="cmd">DELETE</span> — 删除视频文件</p>
<pre>{"cmd":"DELETE","params":{"file_path":"D:/BankRecord/videos/REC_TRX001.mp4"}}</pre>

<p><span class="cmd">STATUS</span> — 查询当前录制状态</p>
<pre>{"cmd":"STATUS","params":{}}</pre>

<p><span class="cmd">LIST</span> — 获取本地视频列表</p>
<pre>{"cmd":"LIST","params":{}}</pre>

<p><span class="cmd">PING</span> — 心跳检测</p>
<pre>{"cmd":"PING","params":{}}</pre>

<h3>🔔 服务端主动推送事件</h3>
<pre>RECORD_STARTED  录制开始（含文件路径）
RECORD_PAUSED   录制暂停
RECORD_RESUMED  录制继续
RECORD_STOPPED  录制停止（含文件路径）
RECORD_ERROR    录制错误（含错误信息）
UPLOAD_QUEUED   文件已加入上传队列</pre>

<h3 class="note">⚠ 集成注意事项</h3>
<ul>
<li>双录服务默认监听 127.0.0.1:9527（可配置）</li>
<li>同一时刻只能进行一路录制</li>
<li>录制未停止时，关闭界面不影响录制（服务后台继续）</li>
<li>上传失败时文件不删除，重启服务后自动续传</li>
</ul>
)");
    rightTabs->addTab(protocolDoc, "📖 接口说明");

    // Tab: 代码示例
    QTextEdit *codeExample = new QTextEdit;
    codeExample->setReadOnly(true);
    codeExample->setHtml(R"(
<style>body{background:#0a0f1e;color:#94a3b8;font-family:Consolas,monospace;font-size:12px;}
h3{color:#38bdf8;} pre{background:#1e293b;padding:12px;border-radius:6px;color:#86efac;}</style>

<h3>Java 调用示例</h3>
<pre>
import java.net.*;
import java.io.*;
import org.json.*;

public class DualRecordClient {
    private Socket socket;
    private DataOutputStream out;
    private DataInputStream  in;

    public void connect(String host, int port) throws IOException {
        socket = new Socket(host, port);
        out = new DataOutputStream(socket.getOutputStream());
        in  = new DataInputStream(socket.getInputStream());
    }

    public JSONObject sendCommand(String cmd, JSONObject params)
            throws IOException, JSONException {
        JSONObject req = new JSONObject();
        req.put("cmd", cmd);
        req.put("params", params);
        byte[] body = req.toString().getBytes("UTF-8");
        // 写4字节大端长度
        out.writeInt(body.length);
        out.write(body);
        out.flush();
        // 读响应
        int len = in.readInt();
        byte[] buf = new byte[len];
        in.readFully(buf);
        return new JSONObject(new String(buf, "UTF-8"));
    }

    // 开始录制示例
    public void startRecord(String taskId, String custName)
            throws Exception {
        JSONObject p = new JSONObject();
        p.put("task_id",   taskId);
        p.put("cust_name", custName);
        JSONObject resp = sendCommand("START", p);
        System.out.println("START resp: " + resp);
    }
}
</pre>

<h3>C# 调用示例</h3>
<pre>
using System.Net.Sockets;
using System.Text;
using System.Text.Json;

public class DualRecordClient : IDisposable {
    private TcpClient _client;
    private NetworkStream _stream;

    public void Connect(string host, int port) {
        _client = new TcpClient(host, port);
        _stream = _client.GetStream();
    }

    public async Task&lt;JsonDocument&gt; SendCommandAsync(
            string cmd, object parameters) {
        var payload = JsonSerializer.SerializeToUtf8Bytes(
            new { cmd, @params = parameters });
        // 大端4字节长度头
        byte[] lenBytes = BitConverter.GetBytes(
            System.Net.IPAddress.HostToNetworkOrder(payload.Length));
        await _stream.WriteAsync(lenBytes, 0, 4);
        await _stream.WriteAsync(payload, 0, payload.Length);
        // 读响应
        byte[] hdr = new byte[4];
        await _stream.ReadAsync(hdr, 0, 4);
        int rlen = System.Net.IPAddress.NetworkToHostOrder(
            BitConverter.ToInt32(hdr, 0));
        byte[] body = new byte[rlen];
        await _stream.ReadAsync(body, 0, rlen);
        return JsonDocument.Parse(body);
    }

    public void Dispose() {
        _stream?.Dispose(); _client?.Dispose();
    }
}
</pre>

<h3>Python 调用示例</h3>
<pre>
import socket, struct, json

class DualRecordClient:
    def __init__(self, host='127.0.0.1', port=9527):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))

    def send_command(self, cmd, params=None):
        body = json.dumps({'cmd': cmd, 'params': params or {}}).encode()
        self.sock.sendall(struct.pack('>I', len(body)) + body)
        hdr = self.sock.recv(4)
        rlen = struct.unpack('>I', hdr)[0]
        return json.loads(self.sock.recv(rlen))

# 使用示例
client = DualRecordClient()
print(client.send_command('PING'))
print(client.send_command('START', {'task_id': 'T001', 'cust_name': '张三'}))
</pre>
)");
    rightTabs->addTab(codeExample, "💻 代码示例");

    rootLayout->addWidget(leftPanel);
    rootLayout->addWidget(rightTabs, 1);

    // ── 状态栏 ──
    statusBar()->setStyleSheet("background:#0f172a;color:#64748b;");
    statusBar()->showMessage("双录控件 Demo — 就绪，请连接服务");
}

void DemoWindow::setupConnections()
{
    connect(m_connectBtn,    &QPushButton::clicked, this, &DemoWindow::onConnect);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &DemoWindow::onDisconnect);
    connect(m_socket, &QTcpSocket::connected,    this, &DemoWindow::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &DemoWindow::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,    this, &DemoWindow::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &DemoWindow::onSocketError);
    m_heartbeatTimer->setInterval(30000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &DemoWindow::onHeartbeat);
}

// ─────────────────────────────────────────
// 网络操作
// ─────────────────────────────────────────
void DemoWindow::onConnect()
{
    m_socket->connectToHost(m_hostEdit->text(), (quint16)m_portSpin->value());
    appendLog(QString("正在连接 %1:%2 ...").arg(m_hostEdit->text()).arg(m_portSpin->value()), "#f59e0b");
}

void DemoWindow::onDisconnect()
{
    m_socket->disconnectFromHost();
}

void DemoWindow::onConnected()
{
    m_connected = true;
    m_statusLabel->setText("● 已连接");
    m_statusLabel->setObjectName("statusConnected");
    m_connectBtn->setEnabled(false);
    m_disconnectBtn->setEnabled(true);
    m_heartbeatTimer->start();
    appendLog("✔ 连接成功！", "#10b981");
    statusBar()->showMessage(QString("已连接到双录服务 %1:%2")
        .arg(m_hostEdit->text()).arg(m_portSpin->value()));
}

void DemoWindow::onDisconnected()
{
    m_connected = false;
    m_statusLabel->setText("● 未连接");
    m_statusLabel->setObjectName("statusDisconnected");
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);
    m_heartbeatTimer->stop();
    appendLog("✖ 已断开连接", "#ef4444");
    statusBar()->showMessage("已断开连接");
}

void DemoWindow::onSocketError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err)
    appendLog("❌ 连接错误: " + m_socket->errorString(), "#ef4444");
    onDisconnected();
}

void DemoWindow::onReadyRead()
{
    m_buffer += m_socket->readAll();
    while (m_buffer.size() >= 4) {
        quint32 len = ((quint8)m_buffer[0] << 24) | ((quint8)m_buffer[1] << 16) |
                      ((quint8)m_buffer[2] <<  8) | ((quint8)m_buffer[3]);
        if ((quint32)m_buffer.size() < 4 + len) break;
        QByteArray payload = m_buffer.mid(4, len);
        m_buffer.remove(0, 4 + len);

        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(payload, &err);
        if (err.error == QJsonParseError::NoError) {
            QString json = doc.toJson(QJsonDocument::Indented);
            if (doc.object().contains("event")) {
                appendLog("📡 [EVENT] " + json, "#a78bfa");
            } else {
                appendLog("← [RESP]  " + json, "#86efac");
            }
        }
    }
}

void DemoWindow::sendCommand(const QString &cmd, const QJsonObject &params)
{
    if (!m_connected) {
        appendLog("⚠ 未连接到服务，请先连接", "#fbbf24");
        return;
    }
    QJsonObject req;
    req["cmd"]    = cmd;
    req["params"] = params;
    QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);
    quint32 len = static_cast<quint32>(payload.size());
    QByteArray frame;
    frame.resize(4);
    frame[0] = (len >> 24) & 0xFF; frame[1] = (len >> 16) & 0xFF;
    frame[2] = (len >>  8) & 0xFF; frame[3] = (len >>  0) & 0xFF;
    frame += payload;
    m_socket->write(frame);

    appendLog("→ [SEND]  " + QJsonDocument(req).toJson(QJsonDocument::Indented), "#7dd3fc");
}

// ─────────────────────────────────────────
// 命令按钮处理
// ─────────────────────────────────────────
void DemoWindow::onSendInit()
{
    sendCommand("INIT", {
        {"video_cache_path",   m_videoCacheEdit->text()},
        {"upload_protocol",    "ftp"},
        {"ftp_host",           m_ftpHostEdit->text()},
        {"ftp_port",           m_ftpPortSpin->value()},
        {"ftp_user",           m_ftpUserEdit->text()},
        {"ftp_pass",           m_ftpPassEdit->text()},
        {"ftp_remote_dir",     m_ftpDirEdit->text()},
        {"bandwidth_limit_kb", m_bwLimitSpin->value()}
    });
}
void DemoWindow::onSendStart()
{
    sendCommand("START", {{"task_id", m_taskIdEdit->text()},{"cust_name", m_custNameEdit->text()}});
}
void DemoWindow::onSendPause()   { sendCommand("PAUSE");  }
void DemoWindow::onSendResume()  { sendCommand("RESUME"); }
void DemoWindow::onSendStop()    { sendCommand("STOP");   }
void DemoWindow::onSendPlay()    { sendCommand("PLAY",    {{"file_path", m_filePathEdit->text()}}); }
void DemoWindow::onSendStatus()  { sendCommand("STATUS"); }
void DemoWindow::onSendList()    { sendCommand("LIST");   }
void DemoWindow::onSendPing()    { sendCommand("PING");   }
void DemoWindow::onHeartbeat()   { sendCommand("PING");   }

void DemoWindow::appendLog(const QString &msg, const QString &color)
{
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    m_logView->append(QString("<span style='color:#475569;'>[%1]</span> "
                               "<span style='color:%2;white-space:pre;'>%3</span>")
                       .arg(ts, color, msg.toHtmlEscaped()));
}
