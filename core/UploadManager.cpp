/**
 * @file UploadManager.cpp
 * @brief 银行柜面双录控件 - FTP/SFTP 上传管理器实现
 *
 * SFTP 使用 libssh2 (BSD 2-Clause, 无需商业授权)
 * FTP  使用 Qt Network (LGPL 2.1)
 */

#include "UploadManager.h"
#include "AppLogger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QThread>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

// libssh2 (SFTP实现)
#ifdef USE_LIBSSH2
#  include <libssh2.h>
#  include <libssh2_sftp.h>
#  ifdef Q_OS_WIN
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    pragma comment(lib, "ws2_32.lib")
#  else
#    include <sys/socket.h>
#    include <netinet/in.h>
#    include <arpa/inet.h>
#    include <netdb.h>
#    include <unistd.h>
#  endif
#endif

// FTP (Qt Network)
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>

// ─────────────────────────────────────────────────────────────────────────────
// UploadWorker
// ─────────────────────────────────────────────────────────────────────────────

UploadWorker::UploadWorker(QObject *parent)
    : QObject(parent)
    , m_port(21)
    , m_bucketTokens(0)
    , m_bucketCapacity(512 * 1024)   // 默认512KB/s
    , m_lastRefillTime(0)
{
}

void UploadWorker::setConfig(const QSettings *cfg)
{
    m_protocol  = cfg->value("upload/protocol", "ftp").toString().toLower();
    m_host      = cfg->value("upload/ftp_host").toString();
    m_port      = cfg->value("upload/ftp_port", (m_protocol == "sftp") ? 22 : 21).toInt();
    m_user      = cfg->value("upload/ftp_user").toString();
    m_pass      = cfg->value("upload/ftp_pass").toString();
    m_remoteDir = cfg->value("upload/ftp_remote_dir", "/dualrecord/").toString();
    int bwKb    = cfg->value("upload/bandwidth_limit_kb", 500).toInt();

    APP_LOG(QString("[UploadWorker] setConfig: protocol=%1 host=%2 port=%3 user=%4 remoteDir=%5 bwKb=%6")
            .arg(m_protocol).arg(m_host).arg(m_port).arg(m_user).arg(m_remoteDir).arg(bwKb));

    setBandwidthLimit(bwKb * 1024);
}

void UploadWorker::setBandwidthLimit(int bytesPerSecond)
{
    QMutexLocker lk(&m_bucketMutex);
    m_bucketCapacity = bytesPerSecond;
    m_bucketTokens   = bytesPerSecond;
    m_lastRefillTime = QDateTime::currentMSecsSinceEpoch();
}

// 令牌桶限速（在工作线程中调用，阻塞式等待令牌）
void UploadWorker::throttleWrite(int bytes)
{
    if (m_bucketCapacity <= 0) return; // 无限制

    while (true) {
        QMutexLocker lk(&m_bucketMutex);
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 elapsed = now - m_lastRefillTime;
        if (elapsed > 0) {
            qint64 refill = (m_bucketCapacity * elapsed) / 1000;
            m_bucketTokens = qMin(m_bucketTokens + refill, m_bucketCapacity);
            m_lastRefillTime = now;
        }
        if (m_bucketTokens >= bytes) {
            m_bucketTokens -= bytes;
            return;
        }
        lk.unlock();
        QThread::msleep(10);
    }
}

void UploadWorker::doUpload(const UploadTask &task)
{
    APP_LOG(QString("[UploadWorker] doUpload: === v2.9-fix14 开始 === protocol=%1 localPath=%2 remotePath=%3")
            .arg(m_protocol).arg(task.localPath).arg(task.remotePath));

    // 检查本地文件是否存在
    QFileInfo fi(task.localPath);
    if (!fi.exists()) {
        APP_LOG(QString("[UploadWorker] doUpload: ✗ 本地文件不存在! %1").arg(task.localPath));
        emit uploadFailed(task.localPath, "本地文件不存在", task.retryCount);
        return;
    }
    APP_LOG(QString("[UploadWorker] doUpload: 文件大小=%1 bytes").arg(fi.size()));

    bool ok = false;
    if (m_protocol == "sftp") {
        APP_LOG("[UploadWorker] doUpload: 使用 SFTP 协议上传");
        ok = uploadSftp(task);
    } else if (m_protocol == "ftp") {
        APP_LOG("[UploadWorker] doUpload: 使用 FTP 协议上传");
        ok = uploadFtp(task);
    } else {
        APP_LOG(QString("[UploadWorker] doUpload: ✗ 未知协议! protocol=%1（应在 upload/protocol 设置为 ftp 或 sftp）").arg(m_protocol));
    }

    if (ok) {
        APP_LOG(QString("[UploadWorker] doUpload: ✓ 上传成功 localPath=%1").arg(task.localPath));
        emit uploadSuccess(task.localPath, task.remotePath);
    } else {
        APP_LOG(QString("[UploadWorker] doUpload: ✗ 上传失败 localPath=%1").arg(task.localPath));
        emit uploadFailed(task.localPath, "上传失败，请检查服务器连接", task.retryCount);
    }
}

// ─────────────────────────────────────────
// FTP 上传（使用 Qt FTP）
// ─────────────────────────────────────────
bool UploadWorker::uploadFtp(const UploadTask &task)
{
    QFile file(task.localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[UploadWorker] uploadFtp: 无法打开文件:" << task.localPath;
        return false;
    }

    // 确保 m_remoteDir 以 / 结尾
    QString remoteDir = m_remoteDir;
    if (!remoteDir.endsWith('/')) remoteDir += '/';
    QString remoteFileName = QFileInfo(task.localPath).fileName();

    QString url = QString("ftp://%1:%2@%3:%4%5%6")
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(m_user)))
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(m_pass)))
        .arg(m_host)
        .arg(m_port)
        .arg(remoteDir)
        .arg(remoteFileName);

    qDebug() << "[UploadWorker] uploadFtp:"
             << "url=ftp://<user>:<pass>@" << m_host << ":" << m_port
             << remoteDir + remoteFileName
             << "fileSize=" << file.size();

    QNetworkAccessManager mgr;
    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setHeader(QNetworkRequest::ContentLengthHeader, file.size());

    // 读取文件内容（分块，模拟限速）
    QByteArray data;
    const int CHUNK = 64 * 1024;
    while (!file.atEnd()) {
        QByteArray chunk = file.read(CHUNK);
        throttleWrite(chunk.size());
        data += chunk;
        emit uploadProgress(task.localPath, data.size(), file.size());
    }
    file.close();

    QEventLoop loop;
    QNetworkReply *reply = mgr.put(req, data);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = (reply->error() == QNetworkReply::NoError);
    if (!success) {
        qWarning() << "[UploadWorker] uploadFtp: 失败 error=" << reply->errorString()
                   << "httpCode=" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    } else {
        qDebug() << "[UploadWorker] uploadFtp: 成功";
    }
    reply->deleteLater();
    return success;
}

// ─────────────────────────────────────────
// SFTP 上传（libssh2）
// ─────────────────────────────────────────
bool UploadWorker::uploadSftp(const UploadTask &task)
{
    APP_LOG(QString("[UploadWorker] uploadSftp: ENTRY protocol=%1 host=%2:%3 USE_LIBSSH2=%4")
            .arg(m_protocol).arg(m_host).arg(m_port)
#ifdef USE_LIBSSH2
            .arg("yes")
#else
            .arg("no!!!")
#endif
            );

#ifdef USE_LIBSSH2
    // ── libssh2 SFTP 上传完整实现 ──
    int rc = libssh2_init(0);
    if (rc != 0) return false;

#ifdef Q_OS_WIN
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,2), &wsadata);
    SOCKET sock;
#else
    int sock;
#endif

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    QString portStr   = QString::number(m_port);
    if (getaddrinfo(m_host.toStdString().c_str(), portStr.toStdString().c_str(), &hints, &res) != 0) {
        libssh2_exit();
        return false;
    }

    sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // ── 设置 socket 超时（防止 libssh2 各阶段卡死）──
    // SO_RCVTIMEO / SO_SNDTIMEO：对 recv/send/handshake/write 均生效
    // 超时后 libssh2 调用会返回 LIBSSH2_ERROR_SOCKET_RECV / SEND 错误
#ifdef Q_OS_WIN
    DWORD tv_ms = 60000; // 60秒
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_ms, sizeof(tv_ms));
    ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv_ms, sizeof(tv_ms));
#else
    struct timeval tv;
    tv.tv_sec  = 60; // 60秒超时
    tv.tv_usec = 0;
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    APP_LOG(QString("[UploadWorker] uploadSftp: 连接 %1:%2 ...").arg(m_host).arg(m_port));
    if (::connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        APP_LOG(QString("[UploadWorker] uploadSftp: ✗ TCP 连接失败 %1:%2").arg(m_host).arg(m_port));
        freeaddrinfo(res);
#ifdef Q_OS_WIN
        closesocket(sock);
#else
        ::close(sock);
#endif
        libssh2_exit();
        return false;
    }
    freeaddrinfo(res);
    APP_LOG(QString("[UploadWorker] uploadSftp: ✓ TCP 连接成功"));

    LIBSSH2_SESSION *session = libssh2_session_init();
    if (!session) { libssh2_exit(); return false; }

    APP_LOG("[UploadWorker] uploadSftp: SSH 握手中...");
    int handshakeRc = libssh2_session_handshake(session, sock);
    if (handshakeRc != 0) {
        char *errMsg = nullptr; int errMsgLen = 0;
        libssh2_session_last_error(session, &errMsg, &errMsgLen, 0);
        APP_LOG(QString("[UploadWorker] uploadSftp: ✗ SSH 握手失败 rc=%1 msg=%2")
                .arg(handshakeRc).arg(errMsg ? QString::fromUtf8(errMsg, errMsgLen) : "未知"));
        libssh2_session_disconnect(session, "Bye");
        libssh2_session_free(session);
#ifdef Q_OS_WIN
        closesocket(sock);
#else
        ::close(sock);
#endif
        libssh2_exit();
        return false;
    }
    APP_LOG("[UploadWorker] uploadSftp: ✓ SSH 握手成功，认证中...");
    if (libssh2_userauth_password(session,
            m_user.toStdString().c_str(),
            m_pass.toStdString().c_str()) != 0) {
        char *errMsg = nullptr; int errMsgLen = 0;
        libssh2_session_last_error(session, &errMsg, &errMsgLen, 0);
        APP_LOG(QString("[UploadWorker] uploadSftp: ✗ 认证失败 user=%1 msg=%2")
                .arg(m_user).arg(errMsg ? QString::fromUtf8(errMsg, errMsgLen) : "密码错误"));
        libssh2_session_disconnect(session, "Bye");
        libssh2_session_free(session);
#ifdef Q_OS_WIN
        closesocket(sock);
#else
        ::close(sock);
#endif
        libssh2_exit();
        return false;
    }
    APP_LOG("[UploadWorker] uploadSftp: ✓ 认证成功，初始化 SFTP 子系统...");

    LIBSSH2_SFTP *sftpSession = libssh2_sftp_init(session);
    if (!sftpSession) {
        char *errMsg = nullptr; int errMsgLen = 0;
        libssh2_session_last_error(session, &errMsg, &errMsgLen, 0);
        APP_LOG(QString("[UploadWorker] uploadSftp: ✗ SFTP 子系统初始化失败 msg=%1")
                .arg(errMsg ? QString::fromUtf8(errMsg, errMsgLen) : "未知"));
        libssh2_session_disconnect(session, "Bye");
        libssh2_session_free(session);
#ifdef Q_OS_WIN
        closesocket(sock);
#else
        ::close(sock);
#endif
        libssh2_exit();
        return false;
    }

    // 使用 enqueue 时计算好的 remotePath（含日期子目录）
    QString remotePath = task.remotePath;
    if (remotePath.isEmpty()) {
        // 回退：直接拼 remoteDir + 文件名
        QString remoteDir = m_remoteDir;
        if (!remoteDir.endsWith('/')) remoteDir += '/';
        remotePath = remoteDir + QFileInfo(task.localPath).fileName();
    }

    // 先尝试创建远端目录（目录已存在时 mkdir 返回错误，忽略即可）
    QString remoteDir2 = remotePath.left(remotePath.lastIndexOf('/'));
    if (!remoteDir2.isEmpty()) {
        libssh2_sftp_mkdir(sftpSession,
            remoteDir2.toStdString().c_str(),
            LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP |
            LIBSSH2_SFTP_S_IXGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH);
        APP_LOG(QString("[UploadWorker] uploadSftp: mkdir %1").arg(remoteDir2));
    }

    APP_LOG(QString("[UploadWorker] uploadSftp: 打开远端文件 %1").arg(remotePath));
    LIBSSH2_SFTP_HANDLE *fileHandle = libssh2_sftp_open(sftpSession,
        remotePath.toStdString().c_str(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
        LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);

    if (!fileHandle) {
        unsigned long sftpErr = libssh2_sftp_last_error(sftpSession);
        APP_LOG(QString("[UploadWorker] uploadSftp: ✗ 打开远端文件失败 sftpErr=%1 path=%2")
                .arg(sftpErr).arg(remotePath));
        libssh2_sftp_shutdown(sftpSession);
        libssh2_session_disconnect(session, "Bye");
        libssh2_session_free(session);
#ifdef Q_OS_WIN
        closesocket(sock);
#else
        ::close(sock);
#endif
        libssh2_exit();
        return false;
    }
    APP_LOG("[UploadWorker] uploadSftp: ✓ 远端文件已打开，开始传输...");

    QFile file(task.localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        APP_LOG(QString("[UploadWorker] uploadSftp: ✗ 无法打开本地文件 %1").arg(task.localPath));
        libssh2_sftp_close(fileHandle);
        libssh2_sftp_shutdown(sftpSession);
        libssh2_session_disconnect(session, "Bye");
        libssh2_session_free(session);
#ifdef Q_OS_WIN
        closesocket(sock);
#else
        ::close(sock);
#endif
        libssh2_exit();
        return false;
    }

    qint64 total = file.size();
    qint64 sent  = 0;
    char buf[32768];
    bool success = true;

    while (!file.atEnd()) {
        qint64 nread = file.read(buf, sizeof(buf));
        if (nread <= 0) break;
        throttleWrite(static_cast<int>(nread));
        char *ptr = buf;
        qint64 rem = nread;
        while (rem > 0) {
            rc = libssh2_sftp_write(fileHandle, ptr, rem);
            if (rc < 0) {
                APP_LOG(QString("[UploadWorker] uploadSftp: ✗ sftp_write 失败 rc=%1").arg(rc));
                success = false;
                break;
            }
            ptr += rc;
            rem -= rc;
            sent += rc;
        }
        if (!success) break;
        emit uploadProgress(task.localPath, sent, total);
    }
    file.close();

    if (success)
        APP_LOG(QString("[UploadWorker] uploadSftp: ✓ 传输完成 sent=%1 bytes").arg(sent));
    else
        APP_LOG(QString("[UploadWorker] uploadSftp: ✗ 传输中断 sent=%1/%2 bytes").arg(sent).arg(total));

    libssh2_sftp_close(fileHandle);
    libssh2_sftp_shutdown(sftpSession);
    libssh2_session_disconnect(session, "Bye");
    libssh2_session_free(session);
#ifdef Q_OS_WIN
    closesocket(sock);
    WSACleanup();
#else
    ::close(sock);
#endif
    libssh2_exit();
    return success;

#else
    // libssh2 未编译时，降级到 FTP
    Q_UNUSED(task)
    return false;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// UploadManager
// ─────────────────────────────────────────────────────────────────────────────

UploadManager::UploadManager(QObject *parent)
    : QObject(parent)
    , m_workerThread(new QThread(this))
    , m_worker(new UploadWorker)
    , m_processTimer(new QTimer(this))
    , m_uploading(false)
    , m_bandwidthLimitKb(500)
    , m_maxRetries(5)
    , m_retryBaseSec(30)
{
    // 注册自定义类型到元类型系统（跨线程 invokeMethod 传参必须）
    qRegisterMetaType<UploadTask>("UploadTask");

    m_worker->moveToThread(m_workerThread);
    m_workerThread->start();

    connect(m_worker, &UploadWorker::uploadSuccess,
            this, &UploadManager::onUploadSuccess);
    connect(m_worker, &UploadWorker::uploadFailed,
            this, &UploadManager::onUploadFailed);
    connect(m_worker, &UploadWorker::uploadProgress,
            this, &UploadManager::onUploadProgress);

    m_processTimer->setInterval(5000); // 每5秒检查队列
    connect(m_processTimer, &QTimer::timeout, this, &UploadManager::processQueue);
    m_processTimer->start();

    // 队列持久化路径
    m_queueFilePath = QDir::homePath() + "/BankDualRecord/.upload_queue.json";
    loadQueue();
}

UploadManager::~UploadManager()
{
    m_processTimer->stop();
    m_workerThread->quit();
    m_workerThread->wait(5000);
    saveQueue();
    delete m_worker;
}

void UploadManager::loadConfig(const QString &configPath)
{
    m_config = new QSettings(configPath, QSettings::IniFormat, this);
    m_bandwidthLimitKb = m_config->value("upload/bandwidth_limit_kb", 500).toInt();
    m_maxRetries       = m_config->value("upload/max_retries", 5).toInt();
    m_retryBaseSec     = m_config->value("upload/retry_interval_sec", 30).toInt();

    APP_LOG(QString("[UploadManager] loadConfig: configPath=%1 protocol=%2 host=%3 port=%4 user=%5 remoteDir=%6 bwKb=%7 maxRetries=%8 retryBaseSec=%9")
            .arg(configPath)
            .arg(m_config->value("upload/protocol", "ftp").toString())
            .arg(m_config->value("upload/ftp_host").toString())
            .arg(m_config->value("upload/ftp_port").toString())
            .arg(m_config->value("upload/ftp_user").toString())
            .arg(m_config->value("upload/ftp_remote_dir", "/dualrecord/").toString())
            .arg(m_bandwidthLimitKb)
            .arg(m_maxRetries)
            .arg(m_retryBaseSec));

    m_worker->setConfig(m_config);
    m_worker->setBandwidthLimit(m_bandwidthLimitKb * 1024);
}

void UploadManager::enqueue(const QString &localPath, const QString &taskId)
{
    QMutexLocker lk(&m_queueMutex);
    // 防止重复入队
    for (const auto &t : m_queue)
        if (t.localPath == localPath) {
    APP_LOG(QString("[UploadManager] enqueue: 已在队列中，跳过 localPath=%1").arg(localPath));
            return;
        }

    UploadTask task;
    task.localPath  = localPath;
    task.remotePath = buildRemotePath(localPath, taskId);
    task.retryCount = 0;
    task.taskId     = taskId;
    task.addedAt    = QDateTime::currentMSecsSinceEpoch();
    m_queue.enqueue(task);
    saveQueue();

    APP_LOG(QString("[UploadManager] enqueue: ✓ 已入队 localPath=%1 remotePath=%2 taskId=%3 queueSize=%4")
            .arg(localPath).arg(task.remotePath).arg(taskId).arg(m_queue.size()));

    emit queueChanged(m_queue.size());

    // 入队后立即尝试触发上传（不依赖5秒定时器）
    QTimer::singleShot(0, this, &UploadManager::processQueue);
}

void UploadManager::processQueue()
{
    if (m_uploading || m_queue.isEmpty()) {
        if (!m_queue.isEmpty())
            APP_LOG(QString("[UploadManager] processQueue: 已在上传中，跳过 queueSize=%1").arg(m_queue.size()));
        return;
    }
    QMutexLocker lk(&m_queueMutex);
    if (m_queue.isEmpty()) return;

    UploadTask task = m_queue.head();
    m_uploading = true;

    APP_LOG(QString("[UploadManager] processQueue: → 开始上传 localPath=%1 remotePath=%2 retryCount=%3")
            .arg(task.localPath).arg(task.remotePath).arg(task.retryCount));

    emit uploadStarted(task.localPath);
    QMetaObject::invokeMethod(m_worker, "doUpload",
        Qt::QueuedConnection, Q_ARG(UploadTask, task));
}

void UploadManager::onUploadSuccess(const QString &localPath, const QString &remotePath)
{
    APP_LOG(QString("[UploadManager] ✓ 上传成功! localPath=%1 remotePath=%2").arg(localPath).arg(remotePath));
    {
        QMutexLocker lk(&m_queueMutex);
        if (!m_queue.isEmpty() && m_queue.head().localPath == localPath)
            m_queue.dequeue();
        saveQueue();
    }
    // 上传成功，删除本地文件
    if (QFile::remove(localPath)) {
        qDebug() << "[UploadManager] 本地文件已删除:" << localPath;
    } else {
        qWarning() << "[UploadManager] 本地文件删除失败:" << localPath;
    }
    m_uploading = false;
    emit uploadSuccess(localPath);
    emit queueChanged(m_queue.size());

    // 上传成功后立即尝试处理下一个任务
    QTimer::singleShot(0, this, &UploadManager::processQueue);
}

void UploadManager::onUploadFailed(const QString &localPath,
                                   const QString &errorMsg, int retryCount)
{
    APP_LOG(QString("[UploadManager] ✗ 上传失败! localPath=%1 error=%2 retryCount=%3")
            .arg(localPath).arg(errorMsg).arg(retryCount));

    QMutexLocker lk(&m_queueMutex);
    if (!m_queue.isEmpty() && m_queue.head().localPath == localPath) {
        UploadTask &task = m_queue.head();
        task.retryCount++;
        if (task.retryCount >= m_maxRetries) {
            APP_LOG(QString("[UploadManager] 达到最大重试次数(%1)，移到队尾: %2")
                    .arg(m_maxRetries).arg(localPath));
            UploadTask t = m_queue.dequeue();
            t.retryCount = 0;
            m_queue.enqueue(t);
        } else {
            int delay = m_retryBaseSec * (1 << qMin(retryCount, 4)) * 1000;
            APP_LOG(QString("[UploadManager] 将在 %1 秒后重试 (retryCount=%2/%3)")
                    .arg(delay / 1000).arg(task.retryCount).arg(m_maxRetries));
        }
        saveQueue();
    }
    m_uploading = false;
    emit uploadFailed(localPath, errorMsg);
    // 指数退避：延迟后重试
    int delay = m_retryBaseSec * (1 << qMin(retryCount, 4)) * 1000;
    QTimer::singleShot(delay, this, &UploadManager::processQueue);
}

void UploadManager::onUploadProgress(const QString &localPath, qint64 sent, qint64 total)
{
    emit uploadProgress(localPath, sent, total);
}

void UploadManager::resumePendingUploads()
{
    loadQueue();
    if (!m_queue.isEmpty())
        processQueue();
}

int  UploadManager::pendingCount() const { return m_queue.size(); }
bool UploadManager::isActive()     const { return m_uploading; }

QJsonArray UploadManager::pendingList() const
{
    QJsonArray arr;
    for (const auto &t : m_queue) {
        QJsonObject o;
        o["local_path"]   = t.localPath;
        o["remote_path"]  = t.remotePath;
        o["retry_count"]  = t.retryCount;
        o["task_id"]      = t.taskId;
        arr.append(o);
    }
    return arr;
}

// ─────────────────────────────────────────
// 队列持久化
// ─────────────────────────────────────────
void UploadManager::saveQueue()
{
    QDir().mkpath(QFileInfo(m_queueFilePath).absolutePath());
    QJsonArray arr;
    for (const auto &t : m_queue) {
        QJsonObject o;
        o["local_path"]  = t.localPath;
        o["remote_path"] = t.remotePath;
        o["retry_count"] = t.retryCount;
        o["task_id"]     = t.taskId;
        o["added_at"]    = t.addedAt;
        arr.append(o);
    }
    QFile f(m_queueFilePath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(arr).toJson());
    }
}

void UploadManager::loadQueue()
{
    QFile f(m_queueFilePath);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;
    QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const QJsonValue &v : arr) {
        QJsonObject o = v.toObject();
        if (!QFile::exists(o["local_path"].toString())) continue; // 文件已不存在
        UploadTask t;
        t.localPath  = o["local_path"].toString();
        t.remotePath = o["remote_path"].toString();
        t.retryCount = o["retry_count"].toInt();
        t.taskId     = o["task_id"].toString();
        t.addedAt    = o["added_at"].toVariant().toLongLong();
        m_queue.enqueue(t);
    }
}

QString UploadManager::buildRemotePath(const QString &localPath, const QString &taskId)
{
    Q_UNUSED(taskId)
    QString date = QDate::currentDate().toString("yyyyMMdd");
    QString fname = QFileInfo(localPath).fileName();
    // 确保 ftp_remote_dir 以 / 结尾
    QString remoteDir = m_config ? m_config->value("upload/ftp_remote_dir", "/dualrecord/").toString()
                                  : "/dualrecord/";
    if (!remoteDir.endsWith('/'))
        remoteDir += '/';
    QString remotePath = QString("%1%2/%3").arg(remoteDir, date, fname);
    qDebug() << "[UploadManager] buildRemotePath:"
             << "localPath=" << localPath
             << "remoteDir=" << remoteDir
             << "date=" << date
             << "remotePath=" << remotePath;
    return remotePath;
}
