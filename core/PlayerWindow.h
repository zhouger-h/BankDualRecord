/**
 * @file PlayerWindow.h
 * @brief 独立视频播放器窗口（需求8：与录制界面分离）
 *
 * 用法：
 *   PlayerWindow *w = new PlayerWindow("/path/to/video.mp4");
 *   w->setAttribute(Qt::WA_DeleteOnClose);
 *   w->show();
 *
 * 支持：进度拖动、音量调节、全屏、键盘快捷键
 */

#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

class PlayerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerWindow(const QString &filePath = QString(),
                          QWidget *parent = nullptr);
    ~PlayerWindow();

    void openFile(const QString &filePath);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onPlayPause();
    void onPositionChanged(qint64 pos);
    void onDurationChanged(qint64 dur);
    void onSliderMoved(int value);
    void onVolumeChanged(int value);
    void onStateChanged(QMediaPlayer::State state);
    void onMediaError(QMediaPlayer::Error error);

private:
    void setupUI();
    QString formatTime(qint64 ms) const;

    QMediaPlayer  *m_player;
    QVideoWidget  *m_videoWidget;
    QSlider       *m_posSlider;
    QSlider       *m_volSlider;
    QPushButton   *m_btnPlay;
    QLabel        *m_posLabel;
    QLabel        *m_titleLabel;
    bool           m_userSeeking;
};

#endif // PLAYERWINDOW_H
