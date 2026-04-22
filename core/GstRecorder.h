/**
 * @file GstRecorder.h
 * @brief GStreamer C API 录制器封装（仅 Linux/麒麟使用）
 *
 * v2.9-fix9: 麒麟 GStreamer 1.16.3 太旧，ximagesink 不支持
 * embed/window-handle 属性（gst-launch 命令行方案失败）。
 * 改用 GStreamer C API 直接管理 pipeline，通过 GstVideoOverlay
 * 接口将视频预览嵌入到 Qt QWidget 的原生窗口句柄中。
 *
 * GstVideoOverlay 在 GStreamer 1.0 起就可用（1.16.3 完全支持），
 * 不依赖 ximagesink 的高版本属性。
 *
 * Pipeline 拓扑：
 *   v4l2src → videoconvert → tee(分流)
 *     ├─→ queue(recq) → x264enc → h264parse → queue(muxq) → mp4mux → filesink
 *     └─→ queue(prevq) → videoconvert → autovideosink  (预览，通过 VideoOverlay 嵌入)
 *   autoaudiosrc → audioconvert → audioresample → queue → avenc_aac → aacparse → mp4mux
 */

#ifndef GSTRECORDER_H
#define GSTRECORDER_H

#include <QObject>
#include <QString>
#include <QSize>
#include <QWidget>    // WId 类型定义

// GStreamer C API 前向声明（避免在头文件中 include GLib/GStreamer 头文件，
// 因为 GLib 的 signals 宏会与 Qt 的 signals 关键字冲突）
typedef struct _GstElement GstElement;
typedef struct _GstBus    GstBus;
typedef struct _GstMessage GstMessage;
typedef void *GstPtr;     // 等价于 gpointer，避免引入 GLib 头文件

class GstRecorder : public QObject
{
    Q_OBJECT

public:
    explicit GstRecorder(QObject *parent = nullptr);
    ~GstRecorder();

    /**
     * @brief 启动录制
     * @param videoDevice  视频设备节点（如 "/dev/video0"）
     * @param outputFilePath  输出 MP4 文件完整路径
     * @param previewWId  预览嵌入的 Qt 窗口句柄（WId）
     * @param resolution  视频分辨率（默认 1280x720）
     * @return true 启动成功
     */
    bool start(const QString &videoDevice,
               const QString &outputFilePath,
               WId previewWId,
               const QSize &resolution = QSize(1280, 720));

    /**
     * @brief 优雅停止录制（发送 EOS，等待落盘）
     * @param timeoutMs  等待超时毫秒数（默认 10000ms）
     * @return true 正常停止并落盘
     */
    bool stop(int timeoutMs = 10000);

    /** @brief 是否正在录制 */
    bool isRecording() const;

    /** @brief 获取最后的错误信息 */
    QString lastError() const;

signals:
    /** @brief 录制已停止（正常停止或异常退出） */
    void finished(bool success, const QString &errorMsg);

    /** @brief 收到 GStreamer 日志消息（用于诊断） */
    void busMessage(const QString &msg);

private:
    // GStreamer 回调（必须是静态函数或全局函数）
    static void onBusMessage(GstBus *bus, GstMessage *msg, GstPtr userData);
    void handleMessage(GstBus *bus, GstMessage *msg);

    // prepare-xwindow-id 回调：autovideosink 准备好窗口时调用，设置嵌入句柄
    static void onPrepareXWindowId(GstElement *elem, GstPtr userData);

    GstElement *m_pipeline;     // 主 pipeline
    bool       m_recording;     // 录制状态标志
    bool       m_eosSent;       // 是否已发送 EOS
    WId        m_previewWId;    // 预览嵌入窗口句柄（给回调用）
    QString    m_lastError;     // 最后错误信息
    QString    m_outputFilePath; // 输出文件路径
};

#endif // GSTRECORDER_H
