#include "../include/videoplayer.h"
#include "../include/decoder_thread.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QTimer>
#include <QFileInfo>
#include <QResizeEvent>
#include <QMessageBox>
#include <QDebug>
#include <iostream>

VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent), 
      playbackState(0),
      currentFrameIndex(0),
      totalFrames(0),
    frameRate(29.97),
    firstFrameWidth(0),
    firstFrameHeight(0)
{
    setWindowTitle(tr("Video Player"));
    setupUI();
    
    workerThread = new QThread(this);
    decoderThread = new DecoderThread();
    decoderThread->moveToThread(workerThread);
    
    connect(workerThread, &QThread::started,
            decoderThread, &DecoderThread::startDecoding);
    connect(decoderThread, &DecoderThread::frameDecoded,
            this, &VideoPlayer::onFrameDecoded, Qt::QueuedConnection);
    connect(decoderThread, &DecoderThread::decodingFinished,
            this, &VideoPlayer::onDecodingFinished, Qt::QueuedConnection);
    connect(decoderThread, &DecoderThread::decodingError,
            this, &VideoPlayer::onDecodingError, Qt::QueuedConnection);
    
    playbackTimer = new QTimer(this);
    connect(playbackTimer, &QTimer::timeout, this, &VideoPlayer::onPlaybackTimer);
    
    setMinimumSize(400, 300);
}

VideoPlayer::~VideoPlayer()
{
    closeFile();
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void VideoPlayer::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setWidgetResizable(false);

    videoLabel = new QLabel;
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setMinimumSize(320, 240);
    videoLabel->setStyleSheet("QLabel { background-color: #000000; }");
    scrollArea->setWidget(videoLabel);

    mainLayout->addWidget(scrollArea, 1);

    // Frame slider
    QHBoxLayout *sliderLayout = new QHBoxLayout;
    timeLabel = new QLabel("00:00");
    timeLabel->setFixedWidth(40);
    
    frameSlider = new QSlider(Qt::Horizontal);
    frameSlider->setMinimum(0);
    frameSlider->setMaximum(0);
    connect(frameSlider, &QSlider::sliderMoved, this, &VideoPlayer::onSliderMoved);
    
    durationLabel = new QLabel("00:00");
    durationLabel->setFixedWidth(40);
    
    sliderLayout->addWidget(timeLabel);
    sliderLayout->addWidget(frameSlider, 1);
    sliderLayout->addWidget(durationLabel);
    mainLayout->addLayout(sliderLayout);

    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(5);

    playButton = new QPushButton(tr("Play"));
    playButton->setFixedWidth(60);
    connect(playButton, &QPushButton::clicked, this, &VideoPlayer::onPlayClicked);
    buttonLayout->addWidget(playButton);

    pauseButton = new QPushButton(tr("Pause"));
    pauseButton->setFixedWidth(60);
    pauseButton->setEnabled(false);
    connect(pauseButton, &QPushButton::clicked, this, &VideoPlayer::onPauseClicked);
    buttonLayout->addWidget(pauseButton);

    stopButton = new QPushButton(tr("Stop"));
    stopButton->setFixedWidth(60);
    stopButton->setEnabled(false);
    connect(stopButton, &QPushButton::clicked, this, &VideoPlayer::onStopClicked);
    buttonLayout->addWidget(stopButton);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    infoLabel = new QLabel(tr("No file loaded"));
    infoLabel->setMaximumHeight(25);
    infoLabel->setStyleSheet("QLabel { background-color: #e0e0e0; padding: 3px; font-size: 11px; }");
    mainLayout->addWidget(infoLabel);

    setLayout(mainLayout);
}

bool VideoPlayer::openFile(const QString &fileName)
{
    if (fileName.isEmpty()) return false;

    currentFileName = fileName;
    QFileInfo fileInfo(fileName);
    
    std::cout << "\n[VideoPlayer] Opening file: " << fileName.toStdString() << std::endl;
    
    infoLabel->setText(tr("Opening: %1...").arg(fileInfo.fileName()));
    frameBuffer.clear();
    currentFrameIndex = 0;
    totalFrames = 0;
    firstFrameWidth = 0;
    firstFrameHeight = 0;

    decoderThread->setInputFile(fileName);

    if (!workerThread->isRunning()) {
        workerThread->start();
    } else {
        std::cout << "[VideoPlayer] Worker thread already running" << std::endl;
        decoderThread->startDecoding();
    }

    return true;
}

void VideoPlayer::closeFile()
{
    if (playbackState == 1) {
        onStopClicked();
    }
    
    decoderThread->stopDecoding();
    if (workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }
    frameBuffer.clear();
}

void VideoPlayer::onPlayClicked()
{
    if (totalFrames <= 0) {
        infoLabel->setText(tr("Please load a video first"));
        return;
    }

    playbackState = 1;
    playButton->setEnabled(false);
    pauseButton->setEnabled(true);
    stopButton->setEnabled(true);
    frameSlider->setEnabled(false);

    int timerInterval = (int)(1000.0 / frameRate);
    playbackTimer->start(timerInterval);

    infoLabel->setText(tr("Playing..."));
}

void VideoPlayer::onPauseClicked()
{
    playbackState = 2;
    playbackTimer->stop();
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(true);

    infoLabel->setText(tr("Paused at frame %1 / %2").arg(currentFrameIndex).arg(totalFrames));
}

void VideoPlayer::onStopClicked()
{
    playbackTimer->stop();
    playbackState = 0;
    currentFrameIndex = 0;
    
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
    frameSlider->setSliderPosition(0);

    infoLabel->setText(tr("Stopped"));
    updateTimeLabel();
}

void VideoPlayer::onSliderMoved(int value)
{
    if (playbackState != 1) {
        currentFrameIndex = value;
        updateTimeLabel();
        displayFrame(value);
    }
}

void VideoPlayer::onPlaybackTimer()
{
    currentFrameIndex++;
    
    if (currentFrameIndex >= totalFrames) {
        onStopClicked();
        return;
    }

    frameSlider->setSliderPosition(currentFrameIndex);
    updateTimeLabel();
    displayFrame(currentFrameIndex);
}

void VideoPlayer::onFrameDecoded(const QPixmap &pixmap, int frameNumber, int totalFrames)
{
    int frameIndex = frameNumber - 1;
    
    // DEEP COPY the pixmap
    QPixmap pixmapCopy = pixmap.copy();
    frameBuffer[frameIndex] = pixmapCopy;
    
    this->totalFrames = totalFrames;
    frameSlider->setMaximum(qMax(0, this->totalFrames - 1));

    if (frameIndex == 0) {
        firstFrameWidth = pixmap.width();
        firstFrameHeight = pixmap.height();
    }
    
    if (frameIndex == 0) {
        displayFrame(0);
    }
}

void VideoPlayer::onDecodingFinished()
{ 
    playButton->setEnabled(true);
    frameSlider->setEnabled(true);
}

void VideoPlayer::onDecodingError(const QString &error)
{
    playbackTimer->stop();
    playbackState = 0;
    infoLabel->setText(tr("Error: %1").arg(error));
    QMessageBox::critical(this, tr("Error"), error);
}

void VideoPlayer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void VideoPlayer::displayFrame(int frameIndex)
{ 
    if (frameIndex < 0 || frameIndex >= totalFrames) {
        std::cout << "  ERROR: frameIndex out of range!" << std::endl;
        return;
    }
    
    if (frameBuffer.find(frameIndex) != frameBuffer.end()) {
        currentPixmap = frameBuffer[frameIndex];
        
        // Check if pixmap actually changed
        static QPixmap lastPixmap;

        lastPixmap = currentPixmap;

        // Keep display dimensions identical to decoded frame dimensions.
        videoLabel->setFixedSize(currentPixmap.size());
        videoLabel->setPixmap(currentPixmap);

        infoLabel->setText(tr("Frame %1 / %2 | Original: %3x%4 | Displayed: %5x%6")
                           .arg(frameIndex + 1)
                           .arg(totalFrames)
                           .arg(currentPixmap.width())
                           .arg(currentPixmap.height())
                           .arg(videoLabel->width())
                           .arg(videoLabel->height()));
    } else {
        std::cout << "  ERROR: Frame NOT in buffer!" << std::endl;
    }
}

void VideoPlayer::scaleImage()
{
    if (currentPixmap.isNull()) {
        return;
    }

    // Preserve original decoded dimensions; do not scale to viewport.
    videoLabel->setFixedSize(currentPixmap.size());
    videoLabel->setPixmap(currentPixmap);
}

void VideoPlayer::updateTimeLabel()
{
    if (frameRate <= 0) frameRate = 29.97;
    
    double currentSeconds = (double)currentFrameIndex / frameRate;
    int minutes = (int)(currentSeconds / 60);
    int seconds = (int)(currentSeconds) % 60;
    timeLabel->setText(QString::asprintf("%02d:%02d", minutes, seconds));
    
    double durationSeconds = (double)totalFrames / frameRate;
    int totalMinutes = (int)(durationSeconds / 60);
    int totalSeconds = (int)(durationSeconds) % 60;
    durationLabel->setText(QString::asprintf("%02d:%02d", totalMinutes, totalSeconds));
}