/**
 * @file DualRecordWidget.h
 * @brief 银行柜面双录控件 - 主录制界面头文件
 *
 * 需求要点：
 *   - 界面精简：仅保留预览区域 + 设备选择 + 录制控制按钮
 *   - 「配置」按钮随时可打开设置界面
 *   - 录制中禁止切换摄像头（避免多文件问题）
 *   - 无本地文件列表，无当前任务信息栏
 *
 * 录制流程：
 *   开始 → openCamera() → 等待 ActiveStatus → setupRecorder() → record()
 *        → 等 RecordingState 确认（超时5秒）→ UI 显示"录制中"
 *   停止 → recorder->stop() → 等 StoppedState 落盘 → closeCamera()
 *   音频由底层 media service 自动采集（Qt5 无公开 setAudioInput 接口）
 */

#ifndef DUALRECORDWIDGET_H
#define DUALRECORDWIDGET_H

#include <QWidget>
#include <QCamera>
#include <QCameraViewfinder>
#include <QMediaRecorder>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTimer>
#include <QSettings>
#include <QFile>
#include <QCloseEvent>
#include <QProcess>

#ifdef Q_OS_LINUX
class GstRecorder;
#endif

class QVideoEncoderSettings;
class QAudioEncoderSettings;

// RecordState 枚举放在 DualRecordWidget 类定义之前

// ─────────────────────────────────────────
// 录制状态枚举
// ─────────────────────────────────────────
enum class RecordState {
    Idle,       // 空闲（摄像头未开启）
    Recording,  // 录制中（摄像头开启）
    Stopped     // 刚停止（摄像头已关闭，等待下次开始）
};

// ─────────────────────────────────────────
// 双录主控件类
// ─────────────────────────────────────────
class DualRecordWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DualRecordWidget(QWidget *parent = nullptr);
    ~DualRecordWidget();

    // ── 公共接口（供 TCP 服务 / 外部直接调用） ──
    void initConfig(const QString &configPath);
    /**
     * @brief startRecord 开始录制
     * @param taskId    任务ID（可空，空时自动生成）
     * @param custName  客户姓名（可空，仅记录用）
     * @param fileName  指定视频文件名（不含路径，含扩展名可选）。
     *                  为空时按 {taskId}_{yyyyMMdd_HHmmss}.{ext} 自动生成（扩展名匹配实际容器）。
     *                  文件保存在配置的缓存目录下。
     */
    void startRecord(const QString &taskId   = QString(),
                     const QString &custName  = QString(),
                     const QString &fileName  = QString());
    void stopRecord();
    void playVideo(const QString &filePath);
    void showWindow();   // 打开/显示主界面
    void hideWindow();   // 隐藏主界面

    /**
     * @brief setPendingFileName 预设下次录制使用的文件名（吊起时设置，开始录制前可更新）
     *        文件名不含路径；若不含扩展名则自动补匹配容器的扩展名
     */
    void setPendingFileName(const QString &fileName);

    /** @brief clearPendingFileName 清除预设文件名（关闭/退出时调用） */
    void clearPendingFileName();

    RecordState getState() const;
    QString getCurrentFilePath() const;
    /** @brief getPendingFileName 获取当前预设文件名（未录制时有效） */
    QString getPendingFileName() const;

signals:
    void recordStarted(const QString &taskId, const QString &filePath);
    void recordStopped(const QString &taskId, const QString &filePath);
    void recordError(const QString &taskId, const QString &errorMsg);
    void uploadQueued(const QString &filePath);
    void windowShown();    // 主界面已显示
    void windowHidden();   // 主界面已隐藏

private slots:
    void onRecordBtnClicked();
    void onConfigBtnClicked();
    void onExitBtnClicked();
    void onCameraChanged(int index);
    void updateDuration();
    void onMediaRecorderStateChanged(QMediaRecorder::State state);
    void onMediaRecorderStatusChanged(QMediaRecorder::Status status);
    void onMediaRecorderError(QMediaRecorder::Error error);
    void onRecorderReady();      // 摄像头就绪 → 开始录制的槽
    void onRecordStartTimeout(); // 录制启动超时处理

protected:
    void closeEvent(QCloseEvent *event) override;  // 右上角叉：隐藏窗口+清除预设文件名
    bool eventFilter(QObject *watched, QEvent *event) override; // 处理 videoContainer resize

private:
    void setupUI();
    void loadDevices();
    void openCamera();
    void closeCamera();
    void updateButtonStates();
    void saveConfig();
    void loadConfig();
    QString generateFileName(const QString &taskId);
    QString formatDuration(qint64 ms);
    void setupRecorder(const QString &outputPath);
    QString detectContainerFormat();  // 运行时检测实际可用的容器格式
    void selectMatchingCodecs(const QString &container,
                              QVideoEncoderSettings &vSettings,
                              QAudioEncoderSettings &aSettings);  // 根据容器匹配编码器

    // ── UI 组件 ──
    QCameraViewfinder  *m_viewfinder;
    QLabel             *m_camPlaceholder;
    QComboBox          *m_cameraSelector;
    QComboBox          *m_audioSelector;
    QComboBox          *m_resolutionSelector;

    QPushButton        *m_btnRecord;   // 「开始录制」/「停止录制」
    QPushButton        *m_btnConfig;   // 「配置」按钮
    QPushButton        *m_btnExit;     // 「退出」按钮

    QLabel             *m_statusLabel;
    QLabel             *m_durationLabel;

    // ── 核心组件 ──
    QCamera            *m_camera;
    QMediaRecorder     *m_recorder;
    QTimer             *m_durationTimer;
    QTimer             *m_recordStartTimer; // 录制启动超时计时器
    QSettings          *m_settings;

    // ── 状态数据 ──
    RecordState         m_state;
    QString             m_currentTaskId;
    QString             m_currentFilePath;
    QString             m_videoCachePath;
    QString             m_pendingFileName;   // 吊起时预设的文件名（含/不含扩展名均可）
    qint64              m_recordingStartMs;
    bool                m_pendingCloseCamera; // 标记：录制停止后等 StoppedState 再关摄像头
    QString             m_actualContainer;     // setupRecorder 检测到的实际容器格式

    QWidget            *m_videoContainer;    // 预览区域容器（Linux 用于 GStreamer 嵌入定位）

#ifdef Q_OS_LINUX
    // ★ v2.9-fix9: Linux 录制改用 GstRecorder（GStreamer C API 封装） ★
    // 原因：gst-launch-1.0 的 ximagesink embed/window-handle 属性需要 GStreamer 1.18+，
    //       麒麟系统只有 1.16.3。改用 GstVideoOverlay C API（1.0 起可用）设置嵌入窗口。
    GstRecorder        *m_gstRecorder;
    QWidget            *m_gstPreviewWidget;  // GStreamer 视频预览容器（通过 VideoOverlay 嵌入）
#endif
};

#endif // DUALRECORDWIDGET_H
