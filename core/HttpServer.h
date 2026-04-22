/**
 * @file HttpServer.h
 * @brief 银行柜面双录控件 - HTTP 通信服务头文件
 *
 * 协议：HTTP/1.1，JSON Body（UTF-8）
 *
 * 请求格式（POST /cmd）：
 *   Content-Type: application/json
 *   {"cmd":"START","params":{"task_id":"T001","cust_name":"张三"}}
 *
 * 响应格式：
 *   {"code":0,"msg":"录制已启动","ts":1713600000000,...}
 *
 * 支持的路径：
 *   POST /cmd          通用命令入口（cmd 字段同 TCP 协议）
 *   GET  /status       等同 STATUS 命令
 *   GET  /list         等同 LIST 命令
 *   GET  /ping         等同 PING 命令
 *
 * 事件推送（SSE）：
 *   GET  /events       Server-Sent Events，长连接实时推送录制状态事件
 *                      事件格式：data: {"event":"RECORD_STARTED","data":{...}}\n\n
 *
 * SSE 事件类型：
 *   RECORD_STARTED  录制开始
 *   RECORD_STOPPED  录制结束
 *   RECORD_ERROR    录制异常
 *   UPLOAD_QUEUED   文件加入上传队列
 *   UPLOAD_SUCCESS  上传完成
 *   UPLOAD_FAILED   上传失败
 *   WINDOW_SHOWN    主界面已显示
 *   WINDOW_HIDDEN   主界面已隐藏
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QList>
#include <QByteArray>

class TcpCommandServer;

// ─────────────────────────────────────────
// HTTP 服务主类
// ─────────────────────────────────────────
class HttpCommandServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpCommandServer(TcpCommandServer *cmdServer, QObject *parent = nullptr);
    ~HttpCommandServer();

    bool start(quint16 port = 9528);
    void stop();
    bool isRunning() const;
    quint16 listenPort() const;

    // 向所有 SSE 客户端推送事件（由 TcpCommandServer 事件信号触发）
    void broadcastSseEvent(const QString &event, const QJsonObject &data = {});

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    // HTTP 请求解析结果
    struct HttpRequest {
        QString method;    // GET / POST
        QString path;      // /cmd, /status, /ping, /events ...
        QByteArray body;   // POST body
        bool complete = false;
    };

    HttpRequest parseRequest(const QByteArray &raw);
    void handleRequest(QTcpSocket *socket, const HttpRequest &req);
    void sendJsonResponse(QTcpSocket *socket, int httpStatus,
                          const QJsonObject &json);
    void sendSseHeaders(QTcpSocket *socket);
    void sendSseMessage(QTcpSocket *socket, const QString &event,
                        const QJsonObject &data);

    QTcpServer           *m_server;
    TcpCommandServer     *m_cmdServer;
    quint16               m_port;

    // 普通请求：socket -> 累积 buffer
    QHash<QTcpSocket*, QByteArray> m_buffers;

    // SSE 长连接客户端列表
    QList<QTcpSocket*> m_sseClients;
};

#endif // HTTPSERVER_H
