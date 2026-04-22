/**
 * @file AppLogger.h
 * @brief 全局日志工具 — 支持开关控制、自定义路径、续写模式
 *
 * 用法：
 *   // 程序启动时初始化（只需一次）
 *   AppLogger::init(configPath);   // 从 config.ini 读取日志配置
 *
 *   // 写日志（所有模块统一使用 APP_LOG 宏）
 *   APP_LOG("hello %1", someVar);
 *   APP_LOG("plain text");
 *
 * 配置项（config.ini）：
 *   [log]
 *   enabled=true              # 是否记录日志文件（false 则只输出 qDebug）
 *   dir=~/BankDualRecord/logs # 日志目录（默认 ~/BankDualRecord/logs）
 *
 * 日志文件命名：dualrecord_YYYYMMDD.log（按日期滚动，自动续写）
 */

#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QMutex>
#include <QSettings>

namespace AppLogger {

// 全局状态（init 时设置）
inline bool     g_enabled = false;   // 是否写日志文件
inline QString  g_logDir;            // 日志目录
inline QFile    g_logFile;           // 当前日志文件句柄
inline QMutex   g_mutex;             // 线程安全

/**
 * @brief 初始化日志系统
 * @param configPath 配置文件路径（如 ~/BankDualRecord/config.ini）
 *
 * 从 config.ini 读取 [log] 节的 enabled 和 dir，
 * 如果 enabled=true，创建日志目录并打开当天的日志文件（追加模式）。
 */
inline void init(const QString &configPath)
{
    QSettings cfg(configPath, QSettings::IniFormat);
    g_enabled = cfg.value("log/enabled", true).toBool();

    // 默认路径：~/BankDualRecord/logs
    QString dir = cfg.value("log/dir").toString();
    if (dir.isEmpty()) {
        dir = QDir::homePath() + "/BankDualRecord/logs";
    }
    // 支持 ~ 展开为用户主目录
    if (dir.startsWith("~/")) {
        dir = QDir::homePath() + dir.mid(1);
    }

    g_logDir = dir;

    if (!g_enabled) {
        qDebug() << "[AppLogger] 日志已禁用（log/enabled=false），仅输出到控制台";
        return;
    }

    // 创建日志目录
    QDir().mkpath(g_logDir);

    // 按日期命名日志文件
    QString logPath = g_logDir + "/dualrecord_" +
                      QDate::currentDate().toString("yyyyMMdd") + ".log";

    g_logFile.setFileName(logPath);
    if (!g_logFile.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "[AppLogger] 无法打开日志文件:" << logPath;
        g_enabled = false;
        return;
    }

    qDebug().noquote() << QString("[AppLogger] 日志已启用，文件: %1（续写模式）").arg(logPath);

    // 写入 session 分隔线
    QTextStream ts(&g_logFile);
    ts << "\n========== " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
       << " 新 session ==========\n";
    ts.flush();
}

/**
 * @brief 初始化日志系统（简化版，默认路径）
 * @param enabled 是否启用日志
 * @param logDir  日志目录
 */
inline void init(bool enabled, const QString &logDir)
{
    g_enabled = enabled;
    g_logDir = logDir;

    if (!g_enabled) return;

    QDir().mkpath(g_logDir);

    QString logPath = g_logDir + "/dualrecord_" +
                      QDate::currentDate().toString("yyyyMMdd") + ".log";

    g_logFile.setFileName(logPath);
    if (!g_logFile.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "[AppLogger] 无法打开日志文件:" << logPath;
        g_enabled = false;
        return;
    }

    QTextStream ts(&g_logFile);
    ts << "\n========== " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
       << " 新 session ==========\n";
    ts.flush();
}

/**
 * @brief 检查是否需要切换日期文件（跨天自动滚动）
 */
inline void checkDateRoll()
{
    if (!g_enabled || !g_logFile.isOpen()) return;

    QString todayFile = g_logDir + "/dualrecord_" +
                        QDate::currentDate().toString("yyyyMMdd") + ".log";

    if (g_logFile.fileName() != todayFile) {
        g_logFile.close();

        g_logFile.setFileName(todayFile);
        if (g_logFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream ts(&g_logFile);
            ts << "\n========== " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
               << " 新 session（日期滚动）==========\n";
            ts.flush();
        }
    }
}

/**
 * @brief 写一行日志
 */
inline void write(const QString &msg)
{
    qDebug().noquote() << msg;

    if (!g_enabled) return;

    QMutexLocker locker(&g_mutex);
    checkDateRoll();

    if (g_logFile.isOpen()) {
        QTextStream ts(&g_logFile);
        ts << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << " " << msg << "\n";
        ts.flush();
    }
}

/**
 * @brief 关闭日志文件（程序退出前调用）
 */
inline void shutdown()
{
    if (g_logFile.isOpen()) {
        g_logFile.close();
    }
}

} // namespace AppLogger

// 全局便捷宏
#define APP_LOG(msg) AppLogger::write(msg)
