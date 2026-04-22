/**
 * @file DemoWindow.h
 * @brief 双录控件 Demo 演示程序 - 柜面系统集成参考
 *
 * 本程序模拟银行柜面系统，演示如何通过 TCP 接口调用双录控件。
 * 包含：接口调用说明、操作演示、调用范例代码预览
 */

#ifndef DEMOWINDOW_H
#define DEMOWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QJsonObject>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QTabWidget>
#include <QTimer>

class DemoWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit DemoWindow(QWidget *parent = nullptr);
    ~DemoWindow();

private slots:
    void onConnect();
    void onDisconnect();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError);

    // 接口按钮
    void onSendInit();
    void onSendStart();
    void onSendPause();
    void onSendResume();
    void onSendStop();
    void onSendPlay();
    void onSendStatus();
    void onSendList();
    void onSendPing();

    void onHeartbeat();

private:
    void setupUI();
    void setupConnections();
    void sendCommand(const QString &cmd, const QJsonObject &params = {});
    void appendLog(const QString &msg, const QString &color = "#e0e0e0");

    // 网络
    QTcpSocket  *m_socket;
    QByteArray   m_buffer;
    QTimer      *m_heartbeatTimer;
    bool         m_connected;

    // UI
    QLineEdit  *m_hostEdit;
    QSpinBox   *m_portSpin;
    QPushButton *m_connectBtn;
    QPushButton *m_disconnectBtn;
    QLabel     *m_statusLabel;
    QTextEdit  *m_logView;
    QLineEdit  *m_taskIdEdit;
    QLineEdit  *m_custNameEdit;
    QLineEdit  *m_filePathEdit;
    QTabWidget *m_tabs;

    // 配置输入
    QLineEdit  *m_videoCacheEdit;
    QLineEdit  *m_ftpHostEdit;
    QLineEdit  *m_ftpUserEdit;
    QLineEdit  *m_ftpPassEdit;
    QLineEdit  *m_ftpDirEdit;
    QSpinBox   *m_ftpPortSpin;
    QSpinBox   *m_bwLimitSpin;
};

#endif // DEMOWINDOW_H
