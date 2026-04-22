/**
 * @file UploadManager.h
 * @brief 银行柜面双录控件 - FTP/SFTP 上传管理器
 *
 * 特性：
 *   - 支持 FTP / SFTP 两种协议
 *   - 上传队列持久化（应用重启后续传）
 *   - 带宽限制（令牌桶算法）
 *   - 上传成功后自动删除本地文件
 *   - 上传失败自动重试（指数退避）
 *
 * 依赖：
 *   - libssh2  (BSD 2-Clause) —— SFTP
 *   - Qt Network (LGPL 2.1)  —— FTP（QNetworkAccessManager）
 */

#ifndef UPLOADMANAGER_H
#define UPLOADMANAGER_H

#include <QObject>
#include <QThread>
#include <QQueue>
#include <QTimer>
#include <QSettings>
#include <QMutex>
#include <QJsonObject>
#include <QJsonArray>

// ─────────────────────────────────────────
// 上传任务结构体
// ─────────────────────────────────────────
struct UploadTask {
    QString localPath;      // 本地文件路径
    QString remotePath;     // 远端相对路径
    int     retryCount;     // 已重试次数
    QString taskId;         // 关联业务任务ID
    qint64  addedAt;        // 加入队列时间戳(ms)
};
Q_DECLARE_METATYPE(UploadTask)

// ─────────────────────────────────────────
// 上传工作线程
// ─────────────────────────────────────────
class UploadWorker : public QObject
{
    Q_OBJECT
public:
    explicit UploadWorker(QObject *parent = nullptr);

    void setConfig(const QSettings *cfg);

    // 令牌桶带宽控制 (bytes/s)
    void setBandwidthLimit(int bytesPerSecond);

signals:
    void uploadSuccess(const QString &localPath, const QString &remotePath);
    void uploadFailed(const QString &localPath, const QString &errorMsg, int retryCount);
    void uploadProgress(const QString &localPath, qint64 sent, qint64 total);

public slots:
    void doUpload(const UploadTask &task);

private:
    bool uploadFtp(const UploadTask &task);
    bool uploadSftp(const UploadTask &task);

    // 令牌桶限速（阻塞式）
    void throttleWrite(int bytes);

    QString  m_protocol;       // "ftp" | "sftp"
    QString  m_host;
    int      m_port;
    QString  m_user;
    QString  m_pass;
    QString  m_remoteDir;

    // 令牌桶参数
    qint64   m_bucketTokens;
    qint64   m_bucketCapacity;    // = 带宽限制 (bytes)
    qint64   m_lastRefillTime;
    QMutex   m_bucketMutex;
};

// ─────────────────────────────────────────
// 上传队列管理器（主线程接口）
// ─────────────────────────────────────────
class UploadManager : public QObject
{
    Q_OBJECT

public:
    explicit UploadManager(QObject *parent = nullptr);
    ~UploadManager();

    void loadConfig(const QString &configPath);

    // 将文件加入上传队列
    void enqueue(const QString &localPath, const QString &taskId = QString());

    // 手动触发（应用启动时继续未完成上传）
    void resumePendingUploads();

    // 状态
    int  pendingCount() const;
    bool isActive() const;

    // 获取待上传列表（for UI/API展示）
    QJsonArray pendingList() const;

signals:
    void uploadStarted(const QString &localPath);
    void uploadSuccess(const QString &localPath);
    void uploadFailed(const QString &localPath, const QString &error);
    void uploadProgress(const QString &localPath, qint64 sent, qint64 total);
    void queueChanged(int count);

private slots:
    void onUploadSuccess(const QString &localPath, const QString &remotePath);
    void onUploadFailed(const QString &localPath, const QString &errorMsg, int retryCount);
    void onUploadProgress(const QString &localPath, qint64 sent, qint64 total);
    void processQueue();

private:
    void saveQueue();
    void loadQueue();
    QString buildRemotePath(const QString &localPath, const QString &taskId);

    QQueue<UploadTask>  m_queue;
    QThread            *m_workerThread;
    UploadWorker       *m_worker;
    QTimer             *m_processTimer;
    QMutex              m_queueMutex;
    bool                m_uploading;
    QString             m_queueFilePath;
    QSettings          *m_config;
    int                 m_bandwidthLimitKb; // KB/s
    int                 m_maxRetries;        // 最大重试次数（从配置读取）
    int                 m_retryBaseSec;      // 指数退避基数(秒)（从配置读取）
};

#endif // UPLOADMANAGER_H
