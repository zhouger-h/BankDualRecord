/**
 * @file InitConfigDialog.h
 * @brief 双录控件配置界面
 *        - 两个页签：「基础设置」(缓存路径+TCP端口+上传策略) 和
 *                    「FTP/SFTP 上传」(服务器配置+测试连接)
 */

#ifndef INITCONFIGDIALOG_H
#define INITCONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFile>
#include <QCheckBox>
#include <QTimer>

class InitConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InitConfigDialog(QWidget *parent = nullptr);
    QString configFilePath() const { return m_configPath; }

private slots:
    void onBrowseCachePath();
    void onBrowseLogDir();
    void onTestConnection();
    void onSaveConfig();

private:
    void setupUI();
    void loadExistingConfig();
    bool validateInputs();

    // ── 连接测试 ──
    void testSftpConnection(const QString &host, int port,
                            const QString &user, const QString &pass);
    void testFtpConnection(const QString &host, int port,
                           const QString &user, const QString &pass);
    void showTestResult(bool success, const QString &msg);

    // ── 缓存设置 ──
    QLineEdit   *m_videoCachePath;
    QPushButton *m_browseCacheBtn;

    // ── 服务配置 ──
    QSpinBox    *m_tcpPort;
    QSpinBox    *m_httpPort;

    // ── 上传服务器 ──
    QComboBox   *m_protocol;
    QLineEdit   *m_host;
    QSpinBox    *m_port;
    QLineEdit   *m_user;
    QLineEdit   *m_pass;
    QLineEdit   *m_remoteDir;
    QPushButton *m_testBtn;
    QTimer      *m_testBtnResetTimer;  // 连接测试结果颜色自动复原计时器

    // ── 上传策略 ──
    QSpinBox    *m_bandwidthKb;    // 上传带宽限制 (KB/s)
    QSpinBox    *m_maxRetries;     // 最大重试次数
    QSpinBox    *m_retryInterval;  // 重试间隔 (秒)

    // ── 日志配置 ──
    QCheckBox   *m_logEnabled;     // 是否启用日志
    QLineEdit   *m_logDir;         // 日志目录

    // ── 底部按钮 ──
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;

    QString m_configPath;
};

#endif // INITCONFIGDIALOG_H
