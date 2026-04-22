/**
 * @file PlayerWindow.cpp
 * @brief 独立视频播放器实现
 */

#include "PlayerWindow.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QStyle>

PlayerWindow::PlayerWindow(const QString &filePath, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_userSeeking(false)
{
    setWindowTitle("视频播放器");
    setWindowIcon(QApplication::windowIcon());  // 继承全局应用图标
    setMinimumSize(720, 540);
    resize(800, 580);
    setupUI();

    if (!filePath.isEmpty())
        openFile(filePath);
}

PlayerWindow::~PlayerWindow()
{
    m_player->stop();
}

void PlayerWindow::setupUI()
{
    setStyleSheet(R"(
        QWidget { background:#111827; color:#f9fafb;
                  font-family:'Microsoft YaHei',sans-serif; font-size:13px; }
        QPushButton { background:#1a56db; color:#fff; border:none; border-radius:6px;
                      padding:7px 16px; font-weight:bold; }
        QPushButton:hover  { background:#1340a8; }
        QPushButton:disabled { background:#374151; color:#6b7280; }
        QSlider::groove:horizontal {
            height:5px; background:#374151; border-radius:3px;
        }
        QSlider::sub-page:horizontal {
            background:#1a56db; border-radius:3px;
        }
        QSlider::handle:horizontal {
            width:14px; height:14px; margin:-5px 0;
            background:#60a5fa; border-radius:7px;
        }
        QLabel { color:#d1d5db; }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 标题栏
    m_titleLabel = new QLabel("  — 无文件 —");
    m_titleLabel->setStyleSheet(
        "background:#1f2937;padding:8px 16px;font-size:13px;color:#9ca3af;");
    mainLayout->addWidget(m_titleLabel);

    // 视频区域
    m_player = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    m_videoWidget = new QVideoWidget;
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_player->setVideoOutput(m_videoWidget);
    mainLayout->addWidget(m_videoWidget, 1);

    // 进度条
    m_posSlider = new QSlider(Qt::Horizontal);
    m_posSlider->setRange(0, 0);
    m_posSlider->setStyleSheet("margin:0;border-radius:0;");
    mainLayout->addWidget(m_posSlider);

    // 控制栏
    QWidget *ctrlBar = new QWidget;
    ctrlBar->setStyleSheet("background:#1f2937;padding:6px 12px;");
    QHBoxLayout *ctrlLayout = new QHBoxLayout(ctrlBar);
    ctrlLayout->setContentsMargins(12, 6, 12, 6);
    ctrlLayout->setSpacing(12);

    m_btnPlay = new QPushButton("▶  播放");
    m_btnPlay->setFixedWidth(90);
    ctrlLayout->addWidget(m_btnPlay);

    m_posLabel = new QLabel("00:00 / 00:00");
    m_posLabel->setStyleSheet("color:#9ca3af;font-family:Consolas,monospace;font-size:12px;");
    ctrlLayout->addWidget(m_posLabel);

    ctrlLayout->addStretch(1);

    QLabel *volIcon = new QLabel("🔊");
    ctrlLayout->addWidget(volIcon);

    m_volSlider = new QSlider(Qt::Horizontal);
    m_volSlider->setRange(0, 100);
    m_volSlider->setValue(80);
    m_volSlider->setFixedWidth(100);
    ctrlLayout->addWidget(m_volSlider);

    QPushButton *btnFull = new QPushButton("⛶ 全屏");
    btnFull->setFixedWidth(80);
    connect(btnFull, &QPushButton::clicked, this, [this]() {
        if (isFullScreen()) showNormal(); else showFullScreen();
    });
    ctrlLayout->addWidget(btnFull);

    mainLayout->addWidget(ctrlBar);

    // 快捷键提示
    QLabel *hint = new QLabel("  ⌨ 空格=播放/暂停  ←/→=±5s  Esc=退出全屏  F=全屏");
    hint->setStyleSheet("background:#111827;color:#4b5563;font-size:11px;padding:3px 12px;");
    mainLayout->addWidget(hint);

    // 信号连接
    m_player->setVolume(80);
    connect(m_btnPlay,   &QPushButton::clicked,
            this, &PlayerWindow::onPlayPause);
    connect(m_posSlider, &QSlider::sliderMoved,
            this, &PlayerWindow::onSliderMoved);
    connect(m_posSlider, &QSlider::sliderPressed,
            this, [this](){ m_userSeeking = true; });
    connect(m_posSlider, &QSlider::sliderReleased,
            this, [this](){ m_userSeeking = false; });
    connect(m_volSlider, &QSlider::valueChanged,
            this, &PlayerWindow::onVolumeChanged);
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &PlayerWindow::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &PlayerWindow::onDurationChanged);
    connect(m_player, &QMediaPlayer::stateChanged,
            this, &PlayerWindow::onStateChanged);
    connect(m_player,
            QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
            this, &PlayerWindow::onMediaError);
}

void PlayerWindow::openFile(const QString &filePath)
{
    m_player->stop();
    m_player->setMedia(QUrl::fromLocalFile(filePath));
    m_titleLabel->setText(QString("  %1").arg(QFileInfo(filePath).fileName()));
    setWindowTitle(QString("播放器 — %1").arg(QFileInfo(filePath).fileName()));
    m_player->play();
    m_btnPlay->setText("⏸  暂停");
}

void PlayerWindow::onPlayPause()
{
    if (m_player->state() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}

void PlayerWindow::onPositionChanged(qint64 pos)
{
    if (!m_userSeeking)
        m_posSlider->setValue(static_cast<int>(pos / 1000));

    qint64 dur = m_player->duration();
    m_posLabel->setText(QString("%1 / %2")
        .arg(formatTime(pos))
        .arg(formatTime(dur)));
}

void PlayerWindow::onDurationChanged(qint64 dur)
{
    m_posSlider->setRange(0, static_cast<int>(dur / 1000));
}

void PlayerWindow::onSliderMoved(int value)
{
    m_player->setPosition(static_cast<qint64>(value) * 1000);
}

void PlayerWindow::onVolumeChanged(int value)
{
    m_player->setVolume(value);
}

void PlayerWindow::onStateChanged(QMediaPlayer::State state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        m_btnPlay->setText("⏸  暂停"); break;
    case QMediaPlayer::PausedState:
        m_btnPlay->setText("▶  播放"); break;
    case QMediaPlayer::StoppedState:
        m_btnPlay->setText("▶  重播"); break;
    }
}

void PlayerWindow::onMediaError(QMediaPlayer::Error /*error*/)
{
    m_titleLabel->setText(QString("  ⚠ 播放失败: %1").arg(m_player->errorString()));
    m_titleLabel->setStyleSheet(
        "background:#1f2937;padding:8px 16px;font-size:13px;color:#f87171;");
}

void PlayerWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        onPlayPause(); break;
    case Qt::Key_Left:
        m_player->setPosition(qMax(0LL, m_player->position() - 5000)); break;
    case Qt::Key_Right:
        m_player->setPosition(qMin(m_player->duration(), m_player->position() + 5000)); break;
    case Qt::Key_F:
        if (isFullScreen()) showNormal(); else showFullScreen(); break;
    case Qt::Key_Escape:
        if (isFullScreen()) showNormal(); else close(); break;
    default:
        QWidget::keyPressEvent(event);
    }
}

QString PlayerWindow::formatTime(qint64 ms) const
{
    int s = static_cast<int>(ms / 1000);
    return QString("%1:%2:%3")
        .arg(s / 3600,        2, 10, QChar('0'))
        .arg((s % 3600) / 60, 2, 10, QChar('0'))
        .arg(s % 60,          2, 10, QChar('0'));
}
