/**
 * @file HttpServer.cpp
 * @brief 银行柜面双录控件 - HTTP 通信服务实现
 *
 * 功能概述：
 *  - 基于 QTcpServer 手写轻量 HTTP/1.1 解析器（无需 Qt WebSocket/HTTP Server 模块）
 *  - POST /cmd   接收 JSON 命令，复用 TcpCommandServer::handleCommand() 处理业务逻辑
 *  - GET /status /list /ping  快捷 GET 接口
 *  - GET /events  Server-Sent Events（SSE）长连接，实时推送录制事件
 */

#include "HttpServer.h"
#include "TcpServer.h"

#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

// ═══════════════════════════════════════════════════════════════════
// 构造 / 析构
// ═══════════════════════════════════════════════════════════════════

HttpCommandServer::HttpCommandServer(TcpCommandServer *cmdServer, QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_cmdServer(cmdServer)
    , m_port(9528)
{
    connect(m_server, &QTcpServer::newConnection,
            this, &HttpCommandServer::onNewConnection);
}

HttpCommandServer::~HttpCommandServer()
{
    stop();
}

// ═══════════════════════════════════════════════════════════════════
// 启动 / 停止
// ═══════════════════════════════════════════════════════════════════

bool HttpCommandServer::start(quint16 port)
{
    m_port = port;
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        if (!m_server->listen(QHostAddress::Any, port))
            return false;
    }
    return true;
}

void HttpCommandServer::stop()
{
    m_server->close();
    for (QTcpSocket *s : m_sseClients) { s->close(); }
    m_sseClients.clear();
    for (QTcpSocket *s : m_buffers.keys()) { s->close(); }
    m_buffers.clear();
}

bool HttpCommandServer::isRunning() const { return m_server->isListening(); }
quint16 HttpCommandServer::listenPort() const { return m_server->serverPort(); }

// ═══════════════════════════════════════════════════════════════════
// 新连接
// ═══════════════════════════════════════════════════════════════════

void HttpCommandServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        m_buffers[socket] = QByteArray();

        connect(socket, &QTcpSocket::readyRead,
                this, &HttpCommandServer::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected,
                this, &HttpCommandServer::onClientDisconnected);
    }
}

void HttpCommandServer::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    m_sseClients.removeAll(socket);
    m_buffers.remove(socket);
    socket->deleteLater();
}

// ═══════════════════════════════════════════════════════════════════
// 读取并解析 HTTP 请求
// ═══════════════════════════════════════════════════════════════════

void HttpCommandServer::onClientReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    m_buffers[socket] += socket->readAll();

    HttpRequest req = parseRequest(m_buffers[socket]);
    if (!req.complete) return;   // 请求未完整到达，继续等待

    m_buffers.remove(socket);    // 请求已完整，清空缓冲（HTTP/1.0 短连接风格）
    handleRequest(socket, req);
}

// ─────────────────────────────────────────
// 极简 HTTP/1.1 请求解析
// 只解析：请求行 / Content-Length / Body
// ─────────────────────────────────────────
HttpCommandServer::HttpRequest HttpCommandServer::parseRequest(const QByteArray &raw)
{
    HttpRequest req;

    // 头部结束标记
    int headerEnd = raw.indexOf("\r\n\r\n");
    if (headerEnd < 0) return req;  // 头部还未收完

    QByteArray headerPart = raw.left(headerEnd);
    QByteArray body       = raw.mid(headerEnd + 4);

    QList<QByteArray> lines = headerPart.split('\n');
    if (lines.isEmpty()) return req;

    // 请求行：METHOD PATH HTTP/1.x
    QList<QByteArray> requestLine = lines[0].trimmed().split(' ');
    if (requestLine.size() < 2) return req;

    req.method = QString::fromLatin1(requestLine[0]).toUpper();
    req.path   = QString::fromLatin1(requestLine[1]);   // 包含 query string

    // Content-Length
    int contentLength = 0;
    for (int i = 1; i < lines.size(); ++i) {
        QByteArray line = lines[i].trimmed().toLower();
        if (line.startsWith("content-length:")) {
            contentLength = line.mid(15).trimmed().toInt();
            break;
        }
    }

    // POST 需要等 body 收齐
    if (req.method == "POST" && body.size() < contentLength)
        return req;

    req.body     = body.left(contentLength);
    req.complete = true;
    return req;
}

// ═══════════════════════════════════════════════════════════════════
// 路由分发
// ═══════════════════════════════════════════════════════════════════

void HttpCommandServer::handleRequest(QTcpSocket *socket, const HttpRequest &req)
{
    // 去除 query string
    QString path = req.path.section('?', 0, 0);

    // ── CORS 预检 ──
    if (req.method == "OPTIONS") {
        QByteArray resp =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        socket->write(resp);
        socket->disconnectFromHost();
        return;
    }

    // ── SSE 长连接：GET /events ──
    if (req.method == "GET" && path == "/events") {
        sendSseHeaders(socket);
        m_sseClients.append(socket);
        // socket 不关闭，保持长连接
        return;
    }

    // ── 快捷 GET 接口 ──
    if (req.method == "GET") {
        QJsonObject cmdObj;
        if (path == "/ping" || path == "/") {
            cmdObj["cmd"] = "PING";
        } else if (path == "/status") {
            cmdObj["cmd"] = "STATUS";
        } else if (path == "/list") {
            cmdObj["cmd"] = "LIST";
        } else {
            sendJsonResponse(socket, 404,
                {{"code", -404}, {"msg", "未知路径: " + path}});
            return;
        }
        cmdObj["params"] = QJsonObject{};
        QJsonObject result = m_cmdServer->handleCommand(cmdObj);
        sendJsonResponse(socket, 200, result);
        return;
    }

    // ── 通用命令入口：POST /cmd ──
    if (req.method == "POST" && path == "/cmd") {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(req.body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendJsonResponse(socket, 400,
                {{"code", -400}, {"msg", "请求体必须是合法的 JSON 对象"}});
            return;
        }
        QJsonObject cmdObj = doc.object();
        QJsonObject result = m_cmdServer->handleCommand(cmdObj);
        sendJsonResponse(socket, 200, result);
        return;
    }

    // ── 其余情况 ──
    sendJsonResponse(socket, 404,
        {{"code", -404}, {"msg", "未知路径或方法: " + req.method + " " + path}});
}

// ═══════════════════════════════════════════════════════════════════
// 响应辅助函数
// ═══════════════════════════════════════════════════════════════════

void HttpCommandServer::sendJsonResponse(QTcpSocket *socket,
                                         int httpStatus,
                                         const QJsonObject &json)
{
    // 补充时间戳字段
    QJsonObject body = json;
    body["ts"] = QDateTime::currentMSecsSinceEpoch();

    QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QString statusText;
    switch (httpStatus) {
    case 200: statusText = "OK";          break;
    case 400: statusText = "Bad Request"; break;
    case 404: statusText = "Not Found";   break;
    default:  statusText = "Error";       break;
    }

    QByteArray resp;
    resp += QString("HTTP/1.1 %1 %2\r\n").arg(httpStatus).arg(statusText).toLatin1();
    resp += "Content-Type: application/json; charset=utf-8\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n";
    resp += QString("Content-Length: %1\r\n").arg(payload.size()).toLatin1();
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += payload;

    socket->write(resp);
    socket->disconnectFromHost();
}

// ─────────────────────────────────────────
// SSE 响应头（长连接，不加 Content-Length）
// ─────────────────────────────────────────
void HttpCommandServer::sendSseHeaders(QTcpSocket *socket)
{
    QByteArray resp;
    resp += "HTTP/1.1 200 OK\r\n";
    resp += "Content-Type: text/event-stream; charset=utf-8\r\n";
    resp += "Cache-Control: no-cache\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n";
    resp += "Connection: keep-alive\r\n";
    resp += "\r\n";
    // 立即发送一个 "connected" 事件，让客户端确认连接成功
    resp += "data: {\"event\":\"CONNECTED\",\"ts\":";
    resp += QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    resp += "}\n\n";
    socket->write(resp);
}

// ─────────────────────────────────────────
// 向单个 SSE 客户端发送事件帧
// ─────────────────────────────────────────
void HttpCommandServer::sendSseMessage(QTcpSocket *socket,
                                       const QString &event,
                                       const QJsonObject &data)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QJsonObject payload;
    payload["event"] = event;
    payload["data"]  = data;
    payload["ts"]    = QDateTime::currentMSecsSinceEpoch();

    QByteArray json = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray frame = "data: " + json + "\n\n";
    socket->write(frame);
}

// ═══════════════════════════════════════════════════════════════════
// 广播 SSE 事件（供外部调用：录制开始/停止/错误等）
// ═══════════════════════════════════════════════════════════════════

void HttpCommandServer::broadcastSseEvent(const QString &event, const QJsonObject &data)
{
    // 遍历时先检测断开的连接，清理无效 socket
    QList<QTcpSocket*> dead;
    for (QTcpSocket *s : m_sseClients) {
        if (s->state() != QAbstractSocket::ConnectedState) {
            dead.append(s);
        } else {
            sendSseMessage(s, event, data);
        }
    }
    for (QTcpSocket *s : dead) {
        m_sseClients.removeAll(s);
        s->deleteLater();
    }
}
