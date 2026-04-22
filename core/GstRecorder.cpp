/**
 * @file GstRecorder.cpp
 * @brief GStreamer C API 录制器实现（仅 Linux/麒麟使用）
 *
 * v2.9-fix9: 绕过 gst-launch-1.0 命令行（ximagesink 属性不兼容 GStreamer 1.16.3），
 * 改用 GStreamer C API 直接构建和管理 pipeline。
 *
 * v2.9-fix11: 放弃 gst_parse_launch 字符串方式，改用 C API 逐个创建元素。
 * 关键改动：在 autovideosink 上连接 "prepare-xwindow-id" 信号，
 * 确保 X11 窗口句柄在 sink 真正准备好时才设置，避免弹出独立窗口。
 *
 * Pipeline 拓扑（C API 构建）：
 *   v4l2src → capsfilter → videoconvert → tee
 *     ├─→ queue → x264enc → h264parse → queue → mp4mux → filesink
 *     └─→ queue → videoconvert → ximagesink  (预览，prepare-xwindow-id 嵌入)
 *   autoaudiosrc → audioconvert → audioresample → queue → avenc_aac → aacparse → mp4mux
 *
 * 注意：使用 ximagesink 而不是 autovideosink，
 * 因为麒麟（X11）环境下 autovideosink 可能选择不支持 GstVideoOverlay 的 sink（如 glsink），
 * 导致 prepare-xwindow-id 回调无法嵌入窗口、视频弹出独立窗口。
 * ximagesink 在 X11 环境下一定支持 GstVideoOverlay 接口。
 */

#include "GstRecorder.h"
#include "AppLogger.h"

#ifdef Q_OS_LINUX

#include <gst/gst.h>
#ifdef signals
#  undef signals  // GLib 的 signals 宏与 Qt 的 signals 关键字冲突，必须在 .cpp 中取消
#endif
#include <gst/video/videooverlay.h>
#include <glib.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QElapsedTimer>

#include <cstring>

// ─────────────────────────────────────────
// 构造 / 析构
// ─────────────────────────────────────────
GstRecorder::GstRecorder(QObject *parent)
    : QObject(parent)
    , m_pipeline(nullptr)
    , m_recording(false)
    , m_eosSent(false)
    , m_previewWId(0)
{
}

GstRecorder::~GstRecorder()
{
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }
}

// ─────────────────────────────────────────
// prepare-xwindow-id 回调
// autovideosink 内部准备好 X11 窗口时触发此信号，
// 此时设置嵌入句柄可以确保不弹出独立窗口
//
// ★ 重要：此回调在 GStreamer 内部线程触发，不是 Qt 主线程！
//   userData 是 GstRecorder* 本身，通过其成员 m_previewWId 读取 WId。
//   直接传 &m_previewWId 地址会有对象销毁后野指针的风险，
//   改为传 GstRecorder* 指针，回调中加 nullptr 检查。
// ─────────────────────────────────────────
void GstRecorder::onPrepareXWindowId(GstElement *elem, GstPtr userData)
{
    // userData 是 GstRecorder* 指针（见下方 g_signal_connect）
    GstRecorder *self = static_cast<GstRecorder *>(userData);
    if (!self || self->m_previewWId == 0) return;

    WId wid = self->m_previewWId;
    qDebug() << "[GstRecorder] prepare-xwindow-id 触发，设置 WId:" << wid;

    if (GST_IS_VIDEO_OVERLAY(elem)) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(elem),
                                             static_cast<gulong>(wid));
        qDebug() << "[GstRecorder] VideoOverlay 嵌入成功";
    } else {
        qWarning() << "[GstRecorder] 元素不支持 VideoOverlay 接口";
    }
}

// ─────────────────────────────────────────
// 辅助：创建元素（失败自动报错返回 nullptr）
// ─────────────────────────────────────────
static GstElement *makeElem(const char *factory, const char *name)
{
    GstElement *elem = gst_element_factory_make(factory, name);
    if (!elem) {
        qWarning() << "[GstRecorder] 无法创建元素:" << factory;
    }
    return elem;
}

// ─────────────────────────────────────────
// 启动录制（C API 逐个创建元素，手动连接）
// ─────────────────────────────────────────
bool GstRecorder::start(const QString &videoDevice,
                        const QString &outputFilePath,
                        WId previewWId,
                        const QSize &resolution)
{
    if (m_recording) {
        m_lastError = "已在录制中";
        return false;
    }

    m_outputFilePath = outputFilePath;
    m_previewWId = previewWId;
    m_eosSent = false;
    m_lastError.clear();

    int width  = resolution.width();
    int height = resolution.height();
    if (width <= 0 || height <= 0) {
        width = 1280;
        height = 720;
    }

    qDebug() << "[GstRecorder] 启动录制:"
             << "device=" << videoDevice
             << "output=" << outputFilePath
             << "resolution=" << width << "x" << height
             << "previewWId=" << previewWId;

    // ── 1. 创建 pipeline ──
    m_pipeline = gst_pipeline_new("dual-record-pipeline");
    if (!m_pipeline) {
        m_lastError = "Pipeline 创建失败";
        return false;
    }

    // ── 2. 创建视频元素 ──
    GstElement *v4l2src   = makeElem("v4l2src",   "v4l2src");
    GstElement *capsfilt  = makeElem("capsfilter", "videocaps");
    GstElement *vconvert1 = makeElem("videoconvert", "vconvert1");
    GstElement *tee       = makeElem("tee",       "videotee");
    GstElement *recq      = makeElem("queue",     "recq");
    GstElement *x264enc   = makeElem("x264enc",   "x264enc");
    GstElement *h264parse = makeElem("h264parse", "h264parse");
    GstElement *muxq      = makeElem("queue",     "muxq");
    GstElement *mp4mux    = makeElem("mp4mux",    "mp4mux");
    GstElement *filesink  = makeElem("filesink",  "filesink");
    GstElement *prevq     = makeElem("queue",     "prevq");
    GstElement *vconvert2 = makeElem("videoconvert", "vconvert2");
    // ximagesink：麒麟 X11 环境专用，确保支持 GstVideoOverlay 嵌入
    // 不用 autovideosink（它可能自动选 glsink/cluttersink，不支持 VideoOverlay）
    GstElement *vsink     = makeElem("ximagesink", "preview-sink");

    // ── 3. 创建音频元素 ──
    GstElement *asrc      = makeElem("autoaudiosrc", "audiosrc");
    GstElement *aconvert  = makeElem("audioconvert", "aconvert");
    GstElement *aresample = makeElem("audioresample", "aresample");
    GstElement *audioq    = makeElem("queue",     "audioq");
    // avenc_aac 需要 gstreamer1.0-libav（麒麟上不一定安装），尝试回退到 faac
    GstElement *aacenc    = makeElem("avenc_aac",  "aacenc");
    if (!aacenc) {
        qWarning() << "[GstRecorder] avenc_aac 不可用（需要 gstreamer1.0-libav），尝试 faac...";
        aacenc = makeElem("faac", "aacenc");
    }
    if (!aacenc) {
        qWarning() << "[GstRecorder] faac 也不可用，尝试 voaacenc...";
        aacenc = makeElem("voaacenc", "aacenc");
    }
    GstElement *aacparse  = makeElem("aacparse",  "aacparse");

    // ── 4. 检查所有元素是否创建成功 ──
    if (!v4l2src || !capsfilt || !vconvert1 || !tee || !recq || !x264enc ||
        !h264parse || !muxq || !mp4mux || !filesink || !prevq || !vconvert2 || !vsink ||
        !asrc || !aconvert || !aresample || !audioq || !aacenc || !aacparse) {
        m_lastError = "Pipeline 元素创建失败，请检查 GStreamer 插件是否安装";
        qWarning() << "[GstRecorder]" << m_lastError;
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
        return false;
    }

    // ── 5. 设置元素属性 ──
    // v4l2src 设备
    g_object_set(v4l2src, "device", videoDevice.toUtf8().constData(), nullptr);

    // capsfilter：视频分辨率
    GstCaps *videoCaps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        nullptr);
    g_object_set(capsfilt, "caps", videoCaps, nullptr);
    gst_caps_unref(videoCaps);

    // 录制队列：限制缓冲，丢旧帧（低延迟）
    g_object_set(recq, "max-size-buffers", 30, "leaky", 2, nullptr);  // 2 = downstream

    // x264enc：超快编码
    // ★ speed-preset 和 tune 是 GLib GEnum 类型，必须用 guint 传递，不能用 int
    //   speed-preset: 1=ultrafast, 2=superfast ... 9=veryslow（枚举值从1开始）
    //   tune: 0x4 = zerolatency（bitfield，可直接传整数）
    g_object_set(x264enc,
        "speed-preset", (guint)1,  // 1 = ultrafast
        "tune",         (guint)4,  // 0x4 = zerolatency
        "bitrate",      (guint)1500,
        "key-int-max",  (guint)30,
        nullptr);

    // mp4mux：流式写入，边录边落盘
    g_object_set(mp4mux, "streamable", TRUE, "fragment-duration", (guint)1000, nullptr);

    // filesink
    g_object_set(filesink, "location", outputFilePath.toUtf8().constData(), nullptr);

    // ximagesink：不同步（预览不卡主时钟）
    g_object_set(vsink, "sync", FALSE, nullptr);

    // ── 6. 添加元素到 pipeline ──
    gst_bin_add_many(GST_BIN(m_pipeline),
        v4l2src, capsfilt, vconvert1, tee,
        recq, x264enc, h264parse, muxq, mp4mux, filesink,
        prevq, vconvert2, vsink,
        asrc, aconvert, aresample, audioq, aacenc, aacparse,
        nullptr);

    // 提前声明（不初始化），避免 goto fail 跨越带初始化的声明（C++ 规则）
    {
        GstBus *bus = nullptr;
        GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

    // ── 7. 连接元素 ──
    // 视频主线：v4l2src → capsfilter → videoconvert → tee
    if (!gst_element_link_many(v4l2src, capsfilt, vconvert1, tee, nullptr)) {
        m_lastError = "视频主线连接失败";
        goto fail;
    }

    // 录制分支：tee → recq → x264enc → h264parse → muxq → mp4mux → filesink
    if (!gst_element_link_many(recq, x264enc, h264parse, muxq, nullptr)) {
        m_lastError = "录制分支连接失败（recq→x264enc→h264parse→muxq）";
        goto fail;
    }
    // muxq → mp4mux：使用 gst_element_link 让 GStreamer 自动协商 pad（
    // mp4mux 的 sink pad 模板名是 "video_%u"，不是 "sink_%u"，
    // 手动指定 pad 名反而会失败；用 gst_element_link 自动匹配更可靠）
    if (!gst_element_link(muxq, mp4mux)) {
        m_lastError = "muxq→mp4mux 连接失败（视频流，请确认 isomp4 插件已安装）";
        goto fail;
    }
    if (!gst_element_link(mp4mux, filesink)) {
        m_lastError = "mp4mux→filesink 连接失败";
        goto fail;
    }

    // 预览分支：tee → prevq → videoconvert → autovideosink
    if (!gst_element_link_many(prevq, vconvert2, vsink, nullptr)) {
        m_lastError = "预览分支连接失败";
        goto fail;
    }

    // 音频分支：autoaudiosrc → audioconvert → audioresample → audioq → aacenc → aacparse → mp4mux
    if (!gst_element_link_many(asrc, aconvert, aresample, audioq, aacenc, aacparse, nullptr)) {
        m_lastError = "音频分支连接失败（asrc→aconvert→aresample→audioq→aacenc→aacparse）";
        goto fail;
    }
    // aacparse → mp4mux：同样让 GStreamer 自动协商（mp4mux 音频 pad 是 "audio_%u"）
    if (!gst_element_link(aacparse, mp4mux)) {
        m_lastError = "aacparse→mp4mux 连接失败（音频流）";
        goto fail;
    }

    // ── 8. 连接 tee 的 request pads ──
    // 注意：GStreamer 1.16.3 没有 gst_element_request_pad_simple（1.18+ 才有），
    // 必须用 gst_element_get_request_pad
    {
        GstPad *teeSrcPad = gst_element_get_request_pad(tee, "src_%u");
        if (!teeSrcPad) {
            m_lastError = "tee 创建 src pad 失败";
            goto fail;
        }
        GstPad *recqSinkPad = gst_element_get_static_pad(recq, "sink");
        GstPadLinkReturn linkRet = gst_pad_link(teeSrcPad, recqSinkPad);
        gst_object_unref(recqSinkPad);
        gst_object_unref(teeSrcPad);
        if (linkRet != GST_PAD_LINK_OK) {
            m_lastError = QString("tee→recq pad 链接失败: %1").arg(linkRet);
            goto fail;
        }
    }

    {
        GstPad *teeSrcPad2 = gst_element_get_request_pad(tee, "src_%u");
        if (!teeSrcPad2) {
            m_lastError = "tee 创建第二个 src pad 失败";
            goto fail;
        }
        GstPad *prevqSinkPad = gst_element_get_static_pad(prevq, "sink");
        GstPadLinkReturn linkRet = gst_pad_link(teeSrcPad2, prevqSinkPad);
        gst_object_unref(prevqSinkPad);
        gst_object_unref(teeSrcPad2);
        if (linkRet != GST_PAD_LINK_OK) {
            m_lastError = QString("tee→prevq pad 链接失败: %1").arg(linkRet);
            goto fail;
        }
    }

    // ── 9. 设置 VideoOverlay 窗口嵌入（在 PLAYING 之前！） ──
    // ★ 关键：不能等 prepare-xwindow-id 信号（ximagesink 可能在信号触发前就创建独立窗口）
    //   直接在 PLAYING 前通过 GstVideoOverlay 设置窗口句柄，
    //   ximagesink 会在 NULL→READY 状态转换时使用已设置的 handle
    if (previewWId != 0 && GST_IS_VIDEO_OVERLAY(vsink)) {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(vsink),
                                             static_cast<gulong>(previewWId));
        APP_LOG(QString("[GstRecorder] ✓ VideoOverlay 已设置 WId:%1 (PLAYING 前)").arg(previewWId));
    } else if (previewWId != 0) {
        APP_LOG(QString("[GstRecorder] ✗ vsink 不支持 VideoOverlay 接口，预览将弹出独立窗口！sink类型=%1")
                .arg(G_OBJECT_TYPE_NAME(vsink)));
    }

    // ── 10. 设置 bus 消息回调 ──
        bus = gst_element_get_bus(m_pipeline);
        gst_bus_add_signal_watch(bus);
        g_signal_connect(bus, "message", G_CALLBACK(onBusMessage), this);
        gst_object_unref(bus);
        bus = nullptr;

    // ── 11. 启动 pipeline ──
        qDebug() << "[GstRecorder] 设置 pipeline 为 PLAYING 状态...";
        ret = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);

        if (ret == GST_STATE_CHANGE_FAILURE) {
            m_lastError = "Pipeline 状态切换失败（PLAYING）";
            qWarning() << "[GstRecorder]" << m_lastError;
            gst_element_set_state(m_pipeline, GST_STATE_NULL);
            gst_object_unref(m_pipeline);
            m_pipeline = nullptr;
            return false;
        }

        qDebug() << "[GstRecorder] Pipeline 已启动:"
                 << (ret == GST_STATE_CHANGE_ASYNC ? "ASYNC" :
                     ret == GST_STATE_CHANGE_SUCCESS ? "SUCCESS" : "FAILURE");

        m_recording = true;
        return true;

    fail:
        qWarning() << "[GstRecorder]" << m_lastError;
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
        return false;
    } // end scope (bus, ret)
} // end GstRecorder::start()

// ─────────────────────────────────────────
// 停止录制（发 EOS 优雅关闭）
// ─────────────────────────────────────────
bool GstRecorder::stop(int timeoutMs)
{
    if (!m_pipeline || !m_recording) {
        return true;  // 已经停了
    }

    if (!m_eosSent) {
        qDebug() << "[GstRecorder] 发送 EOS 事件...";
        GstEvent *eos = gst_event_new_eos();
        gst_element_send_event(m_pipeline, eos);
        m_eosSent = true;
    }

    GMainContext *ctx = g_main_context_default();
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs && m_recording) {
        g_main_context_iteration(ctx, false);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

    if (m_recording) {
        qWarning() << "[GstRecorder] EOS 超时，强制停止 pipeline";
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        m_recording = false;
        emit finished(false, "EOS 等待超时");
        return false;
    }

    qDebug() << "[GstRecorder] 录制已正常停止，文件已落盘";
    return true;
}

// ─────────────────────────────────────────
// 查询状态
// ─────────────────────────────────────────
bool GstRecorder::isRecording() const
{
    return m_recording;
}

QString GstRecorder::lastError() const
{
    return m_lastError;
}

// ─────────────────────────────────────────
// Bus 消息回调（静态函数，转发到实例方法）
// ─────────────────────────────────────────
void GstRecorder::onBusMessage(GstBus *bus, GstMessage *msg, GstPtr userData)
{
    Q_UNUSED(bus)
    GstRecorder *self = static_cast<GstRecorder *>(userData);
    if (self) {
        self->handleMessage(bus, msg);
    }
}

void GstRecorder::handleMessage(GstBus *bus, GstMessage *msg)
{
    Q_UNUSED(bus)

    GstMessageType type = GST_MESSAGE_TYPE(msg);

    switch (type) {
    case GST_MESSAGE_ERROR: {
        GError *err = nullptr;
        gchar *debug_info = nullptr;
        gst_message_parse_error(msg, &err, &debug_info);

        QString errMsg = QString::fromUtf8(err->message);
        QString dbgInfo = debug_info ? QString::fromUtf8(debug_info) : "";

        qWarning() << "[GstRecorder] ERROR:" << errMsg
                   << "debug:" << dbgInfo;

        m_lastError = errMsg;
        emit busMessage("[ERROR] " + errMsg + (dbgInfo.isEmpty() ? "" : " (" + dbgInfo + ")"));

        m_recording = false;
        emit finished(false, errMsg);

        g_error_free(err);
        g_free(debug_info);
        break;
    }

    case GST_MESSAGE_WARNING: {
        GError *err = nullptr;
        gchar *debug_info = nullptr;
        gst_message_parse_warning(msg, &err, &debug_info);

        QString warnMsg = QString::fromUtf8(err->message);
        qDebug() << "[GstRecorder] WARNING:" << warnMsg;

        emit busMessage("[WARNING] " + warnMsg);

        g_error_free(err);
        g_free(debug_info);
        break;
    }

    case GST_MESSAGE_EOS: {
        qDebug() << "[GstRecorder] EOS 收到 — 录制文件已落盘";

        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        m_recording = false;
        emit finished(true, "");
        break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(m_pipeline)) {
            GstState oldState, newState, pending;
            gst_message_parse_state_changed(msg, &oldState, &newState, &pending);
            qDebug() << "[GstRecorder] Pipeline 状态:"
                     << oldState << "->" << newState << "(pending:" << pending << ")";
        }
        break;
    }

    case GST_MESSAGE_STREAM_STATUS: {
        GstStreamStatusType streamType;
        GstElement *owner = nullptr;
        gst_message_parse_stream_status(msg, &streamType, &owner);
        Q_UNUSED(streamType)
        Q_UNUSED(owner)
        break;
    }

    case GST_MESSAGE_LATENCY: {
        gst_bin_recalculate_latency(GST_BIN(m_pipeline));
        break;
    }

    default:
        break;
    }
}

#endif // Q_OS_LINUX
