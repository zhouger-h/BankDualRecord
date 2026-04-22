/**
 * @file TcpServer.h
 * @brief 银行柜面双录控件 - TCP 通信服务头文件
 *
 * 协议格式（JSON over TCP）:
 *   请求: {"cmd":"START","taskId":"T001","custName":"张三","extra":{}}
 *   响应: {"code":0,"msg":"OK","data":{...}}
 *
 * 命令列表:
 *   INIT        初始化参数
 *   START       开始录制
 *   STOP        终止录制
 *   PLAY        播放视频
 *   DELETE      删除视频
 *   STATUS      查询状态
 *   LIST        获取视频列表
 *   PING        心跳检测
 *   SHOW_WINDOW 打开/显示主界面
 *   HIDE_WINDOW 隐藏主界面
 */

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHash>
#include <QTimer>

class DualRecordWidget;

// ─────────────────────────────────────────
// 客户端连接对象
// ─────────────────────────────────────────
class ClientSession : public QObject
{
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientSession();

    void sendResponse(int code, const QString &msg, const QJsonObject &data = {});
    void sendEvent(const QString &event, const QJsonObject &data = {});
    QString clientId() const { return m_clientId; }

signals:
    void commandReceived(const QString &clientId, const QJsonObject &cmd);
    void disconnected(const QString &clientId);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *m_socket;
    QString     m_clientId;
    QByteArray  m_buffer;
};

// ─────────────────────────────────────────
// TCP 服务主类
// ─────────────────────────────────────────
class TcpCommandServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpCommandServer(DualRecordWidget *widget, QObject *parent = nullptr);
    ~TcpCommandServer();

    bool start(quint16 port = 9527);
    void stop();
    bool isRunning() const;
    quint16 listenPort() const;

    // 向所有连接的客户端广播事件
    void broadcastEvent(const QString &event, const QJsonObject &data = {});

    // 命令处理（public 供 HttpServer 复用，无需重复实现业务逻辑）
    QJsonObject handleCommand(const QJsonObject &cmd);

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void clientConnected(const QString &clientId);
    void clientDisconnected(const QString &clientId);

private slots:
    void onNewConnection();
    void onClientCommand(const QString &clientId, const QJsonObject &cmd);
    void onClientDisconnected(const QString &clientId);

    // 来自 DualRecordWidget 的信号转发
    void onRecordStarted(const QString &taskId, const QString &filePath);
    void onRecordStopped(const QString &taskId, const QString &filePath);
    void onRecordError(const QString &taskId, const QString &errorMsg);
    void onUploadQueued(const QString &filePath);

private:
    QJsonObject cmdInit(const QJsonObject &params);
    QJsonObject cmdStart(const QJsonObject &params);
    QJsonObject cmdStop(const QJsonObject &params);
    QJsonObject cmdPlay(const QJsonObject &params);
    QJsonObject cmdDelete(const QJsonObject &params);
    QJsonObject cmdStatus(const QJsonObject &params);
    QJsonObject cmdList(const QJsonObject &params);
    QJsonObject cmdPing(const QJsonObject &params);
    QJsonObject cmdShowWindow(const QJsonObject &params);
    QJsonObject cmdHideWindow(const QJsonObject &params);

    QTcpServer  *m_server;
    DualRecordWidget *m_widget;
    QHash<QString, ClientSession*> m_clients;
    quint16      m_port;
};

#endif // TCPSERVER_H
