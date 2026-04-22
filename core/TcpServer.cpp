/**
 * @file TcpServer.cpp
 * @brief 银行柜面双录控件 - TCP 通信服务实现
 */

#include "TcpServer.h"
#include "DualRecordWidget.h"

#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QUuid>

// ─────────────────────────────────────────────────────────────────────────────
// ClientSession
// ─────────────────────────────────────────────────────────────────────────────

ClientSession::ClientSession(QTcpSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket)
{
    m_clientId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    connect(socket, &QTcpSocket::readyRead,    this, &ClientSession::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
}

ClientSession::~ClientSession()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
    }
}

void ClientSession::sendResponse(int code, const QString &msg, const QJsonObject &data)
{
    QJsonObject resp;
    resp["code"] = code;
    resp["msg"]  = msg;
    resp["data"] = data;
    resp["ts"]   = QDateTime::currentMSecsSinceEpoch();

    QByteArray payload = QJsonDocument(resp).toJson(QJsonDocument::Compact);
    // 4字节大端长度头 + JSON
    quint32 len = static_cast<quint32>(payload.size());
    QByteArray frame;
    frame.resize(4);
    frame[0] = (len >> 24) & 0xFF;
    frame[1] = (len >> 16) & 0xFF;
    frame[2] = (len >>  8) & 0xFF;
    frame[3] = (len >>  0) & 0xFF;
    frame += payload;
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->write(frame);
}

void ClientSession::sendEvent(const QString &event, const QJsonObject &data)
{
    QJsonObject evt;
    evt["type"]  = "event";
    evt["event"] = event;
    evt["data"]  = data;
    evt["ts"]    = QDateTime::currentMSecsSinceEpoch();

    QByteArray payload = QJsonDocument(evt).toJson(QJsonDocument::Compact);
    quint32 len = static_cast<quint32>(payload.size());
    QByteArray frame;
    frame.resize(4);
    frame[0] = (len >> 24) & 0xFF;
    frame[1] = (len >> 16) & 0xFF;
    frame[2] = (len >>  8) & 0xFF;
    frame[3] = (len >>  0) & 0xFF;
    frame += payload;
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->write(frame);
}

void ClientSession::onReadyRead()
{
    m_buffer += m_socket->readAll();

    // 解帧：4字节长度头 + JSON body
    while (m_buffer.size() >= 4) {
        quint32 len = ((quint8)m_buffer[0] << 24) |
                      ((quint8)m_buffer[1] << 16) |
                      ((quint8)m_buffer[2] <<  8) |
                      ((quint8)m_buffer[3]);
        if ((quint32)m_buffer.size() < 4 + len) break;

        QByteArray payload = m_buffer.mid(4, len);
        m_buffer.remove(0, 4 + len);

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            emit commandReceived(m_clientId, doc.object());
        }
    }
}

void ClientSession::onDisconnected()
{
    emit disconnected(m_clientId);
}

// ─────────────────────────────────────────────────────────────────────────────
// TcpCommandServer
// ─────────────────────────────────────────────────────────────────────────────

TcpCommandServer::TcpCommandServer(DualRecordWidget *widget, QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_widget(widget)
    , m_port(9527)
{
    connect(m_server, &QTcpServer::newConnection, this, &TcpCommandServer::onNewConnection);

    // 监听 DualRecordWidget 信号
    connect(m_widget, &DualRecordWidget::recordStarted,
            this, &TcpCommandServer::onRecordStarted);
    connect(m_widget, &DualRecordWidget::recordStopped,
            this, &TcpCommandServer::onRecordStopped);
    connect(m_widget, &DualRecordWidget::recordError,
            this, &TcpCommandServer::onRecordError);
    connect(m_widget, &DualRecordWidget::uploadQueued,
            this, &TcpCommandServer::onUploadQueued);
    connect(m_widget, &DualRecordWidget::windowShown,
            this, [this]() { broadcastEvent("WINDOW_SHOWN", {}); });
    connect(m_widget, &DualRecordWidget::windowHidden,
            this, [this]() { broadcastEvent("WINDOW_HIDDEN", {}); });
}

TcpCommandServer::~TcpCommandServer()
{
    stop();
}

bool TcpCommandServer::start(quint16 port)
{
    m_port = port;
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        // 如果本地监听失败，尝试所有接口
        if (!m_server->listen(QHostAddress::Any, port))
            return false;
    }
    emit serverStarted(m_server->serverPort());
    return true;
}

void TcpCommandServer::stop()
{
    m_server->close();
    for (ClientSession *s : m_clients.values()) delete s;
    m_clients.clear();
    emit serverStopped();
}

bool TcpCommandServer::isRunning() const { return m_server->isListening(); }
quint16 TcpCommandServer::listenPort() const { return m_server->serverPort(); }

void TcpCommandServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        auto *session = new ClientSession(socket, this);
        m_clients[session->clientId()] = session;
        connect(session, &ClientSession::commandReceived,
                this, &TcpCommandServer::onClientCommand);
        connect(session, &ClientSession::disconnected,
                this, &TcpCommandServer::onClientDisconnected);
        emit clientConnected(session->clientId());
    }
}

void TcpCommandServer::onClientDisconnected(const QString &clientId)
{
    if (m_clients.contains(clientId)) {
        m_clients[clientId]->deleteLater();
        m_clients.remove(clientId);
    }
    emit clientDisconnected(clientId);
}

// ─────────────────────────────────────────
// 命令分发
// ─────────────────────────────────────────
void TcpCommandServer::onClientCommand(const QString &clientId, const QJsonObject &cmd)
{
    QJsonObject result = handleCommand(cmd);
    if (m_clients.contains(clientId)) {
        int code = result["code"].toInt();
        QString msg = result["msg"].toString();
        result.remove("code");
        result.remove("msg");
        m_clients[clientId]->sendResponse(code, msg, result);
    }
}

QJsonObject TcpCommandServer::handleCommand(const QJsonObject &cmd)
{
    QString command = cmd["cmd"].toString().toUpper();
    QJsonObject params = cmd["params"].toObject();

    if (command == "INIT")        return cmdInit(params);
    if (command == "START")       return cmdStart(params);
    if (command == "STOP")        return cmdStop(params);
    if (command == "PLAY")        return cmdPlay(params);
    if (command == "DELETE")      return cmdDelete(params);
    if (command == "STATUS")      return cmdStatus(params);
    if (command == "LIST")        return cmdList(params);
    if (command == "PING")        return cmdPing(params);
    if (command == "SHOW_WINDOW") return cmdShowWindow(params);
    if (command == "HIDE_WINDOW") return cmdHideWindow(params);

    return {{"code", -1}, {"msg", "未知命令: " + command}};
}

// ─────────────────────────────────────────
// 具体命令处理
// ─────────────────────────────────────────
QJsonObject TcpCommandServer::cmdInit(const QJsonObject &params)
{
    QJsonObject r;
    // 写临时配置文件
    QString configPath = QDir::tempPath() + "/dualrecord_init.ini";
    QSettings cfg(configPath, QSettings::IniFormat);
    if (params.contains("video_cache_path"))
        cfg.setValue("cache/video_path", params["video_cache_path"].toString());
    if (params.contains("ftp_host"))
        cfg.setValue("upload/ftp_host", params["ftp_host"].toString());
    if (params.contains("ftp_port"))
        cfg.setValue("upload/ftp_port", params["ftp_port"].toInt(21));
    if (params.contains("ftp_user"))
        cfg.setValue("upload/ftp_user", params["ftp_user"].toString());
    if (params.contains("ftp_pass"))
        cfg.setValue("upload/ftp_pass", params["ftp_pass"].toString());
    if (params.contains("ftp_remote_dir"))
        cfg.setValue("upload/ftp_remote_dir", params["ftp_remote_dir"].toString());
    if (params.contains("upload_protocol"))
        cfg.setValue("upload/protocol", params["upload_protocol"].toString());
    if (params.contains("bandwidth_limit_kb"))
        cfg.setValue("upload/bandwidth_limit_kb", params["bandwidth_limit_kb"].toInt(500));
    cfg.sync();

    m_widget->initConfig(configPath);
    r["code"] = 0;
    r["msg"]  = "初始化成功";
    return r;
}

QJsonObject TcpCommandServer::cmdStart(const QJsonObject &params)
{
    QString taskId   = params["task_id"].toString();
    QString custName = params["cust_name"].toString();
    QString fileName = params["file_name"].toString().trimmed();  // 可选：指定录制文件名

    m_widget->startRecord(taskId, custName, fileName);

    // 录制启动是异步的（需等摄像头就绪），此处返回预期文件名（含路径）
    // 实际文件名在 RECORD_STARTED 事件中确认
    QJsonObject r;
    r["code"]    = 0;
    r["msg"]     = "录制已启动";
    r["task_id"] = taskId;
    // 返回预设文件名（供调用方确认），空表示将使用自动生成规则
    r["file_name"] = fileName.isEmpty() ? QString() : fileName;
    return r;
}

QJsonObject TcpCommandServer::cmdStop(const QJsonObject &params)
{
    Q_UNUSED(params)
    m_widget->stopRecord();
    return {{"code", 0}, {"msg", "录制已停止"}, {"file_path", m_widget->getCurrentFilePath()}};
}

QJsonObject TcpCommandServer::cmdPlay(const QJsonObject &params)
{
    QString path = params["file_path"].toString();
    if (path.isEmpty()) return {{"code", -1}, {"msg", "file_path 不能为空"}};
    m_widget->playVideo(path);
    return {{"code", 0}, {"msg", "播放器已启动"}, {"file_path", path}};
}

QJsonObject TcpCommandServer::cmdDelete(const QJsonObject &params)
{
    QString path = params["file_path"].toString();
    if (path.isEmpty()) return {{"code", -1}, {"msg", "file_path 不能为空"}};
    if (!QFile::exists(path)) return {{"code", -2}, {"msg", "文件不存在"}, {"file_path", path}};
    if (!QFile::remove(path)) return {{"code", -3}, {"msg", "文件删除失败"}, {"file_path", path}};
    return {{"code", 0}, {"msg", "文件已删除"}, {"file_path", path}};
}

QJsonObject TcpCommandServer::cmdStatus(const QJsonObject &params)
{
    Q_UNUSED(params)
    QJsonObject data;
    auto state = m_widget->getState();
    QString stateStr;
    switch (state) {
    case RecordState::Idle:      stateStr = "IDLE"; break;
    case RecordState::Recording: stateStr = "RECORDING"; break;
    case RecordState::Stopped:   stateStr = "STOPPED"; break;
    }
    data["state"]     = stateStr;
    data["file_path"] = m_widget->getCurrentFilePath();
    return {{"code", 0}, {"msg", "OK"}, {"data", data}};
}

QJsonObject TcpCommandServer::cmdList(const QJsonObject &params)
{
    Q_UNUSED(params)
    // 扫描本地缓存目录
    QJsonArray arr;
    QSettings s("BankSystem", "DualRecord");
    QString cachePath = s.value("cache/video_path").toString();
    if (cachePath.isEmpty())
        return {{"code", 0}, {"msg", "OK"}, {"list", arr}};
    QDir dir(cachePath);
    if (dir.exists()) {
        QStringList filters{"*.mp4","*.mov","*.mkv","*.ogv","*.ogg","*.webm","*.avi"};
        for (const QFileInfo &fi : dir.entryInfoList(filters, QDir::Files, QDir::Time)) {
            QJsonObject item;
            item["name"]     = fi.fileName();
            item["path"]     = fi.absoluteFilePath();
            item["size"]     = fi.size();
            item["modified"] = fi.lastModified().toString(Qt::ISODate);
            arr.append(item);
        }
    }
    return {{"code", 0}, {"msg", "OK"}, {"list", arr}};
}

QJsonObject TcpCommandServer::cmdPing(const QJsonObject &params)
{
    Q_UNUSED(params)
    return {{"code", 0}, {"msg", "pong"}, {"ts", QDateTime::currentMSecsSinceEpoch()}};
}

QJsonObject TcpCommandServer::cmdShowWindow(const QJsonObject &params)
{
    // 吊起界面时可同步预设录制文件名
    QString fileName = params["file_name"].toString().trimmed();
    if (!fileName.isEmpty())
        m_widget->setPendingFileName(fileName);

    // 在 GUI 线程中执行（TCP 回调本就在主线程）
    m_widget->showWindow();
    QJsonObject r;
    r["code"] = 0;
    r["msg"]  = "主界面已显示";
    r["file_name"] = fileName;  // 原样回显，方便调用方确认
    return r;
}

QJsonObject TcpCommandServer::cmdHideWindow(const QJsonObject &params)
{
    Q_UNUSED(params)
    m_widget->clearPendingFileName();   // 隐藏界面时清除预设文件名
    m_widget->hideWindow();
    return {{"code", 0}, {"msg", "主界面已隐藏"}};
}

// ─────────────────────────────────────────
// 事件广播
// ─────────────────────────────────────────
void TcpCommandServer::broadcastEvent(const QString &event, const QJsonObject &data)
{
    for (ClientSession *s : m_clients.values())
        s->sendEvent(event, data);
}

void TcpCommandServer::onRecordStarted(const QString &taskId, const QString &filePath)
{
    broadcastEvent("RECORD_STARTED", {{"task_id", taskId}, {"file_path", filePath}});
}

void TcpCommandServer::onRecordStopped(const QString &taskId, const QString &filePath)
{
    broadcastEvent("RECORD_STOPPED", {{"task_id", taskId}, {"file_path", filePath}});
}

void TcpCommandServer::onRecordError(const QString &taskId, const QString &errorMsg)
{
    broadcastEvent("RECORD_ERROR", {{"task_id", taskId}, {"error", errorMsg}});
}

void TcpCommandServer::onUploadQueued(const QString &filePath)
{
    broadcastEvent("UPLOAD_QUEUED", {{"file_path", filePath}});
}
