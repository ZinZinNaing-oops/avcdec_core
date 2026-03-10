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
      frameRate(29.97)
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
    scrollArea->setWidgetResizable(true);

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

    decoderThread->setInputFile(fileName);

    if (!workerThread->isRunning()) {
        std::cout << "[VideoPlayer] Starting worker thread" << std::endl;
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
    
    std::cout << "[VideoPlayer::onFrameDecoded]" << std::endl;
    std::cout << "  Frame number: " << frameNumber << std::endl;
    std::cout << "  Frame index: " << frameIndex << std::endl;
    std::cout << "  Pixmap size: " << pixmap.width() << "x" << pixmap.height() << std::endl;
    std::cout << "  Pixmap null: " << (pixmap.isNull() ? "YES" : "NO") << std::endl;
    
    // ✅ DEEP COPY the pixmap
    QPixmap pixmapCopy = pixmap.copy();
    frameBuffer[frameIndex] = pixmapCopy;
    
    std::cout << "  Stored in frameBuffer[" << frameIndex << "]" << std::endl;
    std::cout << "  Buffer now contains " << frameBuffer.size() << " frames" << std::endl;
    
    this->totalFrames = totalFrames;
    frameSlider->setMaximum(qMax(0, this->totalFrames - 1));
    
    if (frameIndex == 0) {
        displayFrame(0);
    }
}

void VideoPlayer::onDecodingFinished()
{
    std::cout << "\n[VideoPlayer::onDecodingFinished]" << std::endl;
    std::cout << "  Total frames in buffer: " << frameBuffer.size() << std::endl;
    std::cout << "  Total frames variable: " << totalFrames << std::endl;
    
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
    scaleImage();
}

void VideoPlayer::displayFrame(int frameIndex)
{
    std::cout << "\n[VideoPlayer::displayFrame]" << std::endl;
    std::cout << "  Requested frame index: " << frameIndex << std::endl;
    std::cout << "  Total frames (totalFrames): " << totalFrames << std::endl;
    std::cout << "  Buffer size: " << frameBuffer.size() << std::endl;
    
    if (frameIndex < 0 || frameIndex >= totalFrames) {
        std::cout << "  ERROR: frameIndex out of range!" << std::endl;
        return;
    }
    
    if (frameBuffer.find(frameIndex) != frameBuffer.end()) {
        currentPixmap = frameBuffer[frameIndex];
        
        // ✅ DEBUG: Check if pixmap actually changed
        static QPixmap lastPixmap;
        if (currentPixmap.toImage().bits() == lastPixmap.toImage().bits()) {
            std::cout << "  ⚠️ WARNING: Same pixmap data as last frame!" << std::endl;
        } else {
            std::cout << "  ✓ Different pixmap data!" << std::endl;
        }
        lastPixmap = currentPixmap;
        
        std::cout << "  ✓ Frame FOUND in buffer" << std::endl;
        std::cout << "  Pixmap size: " << currentPixmap.width() << "x" << currentPixmap.height() << std::endl;
        std::cout << "  Pixmap null: " << (currentPixmap.isNull() ? "YES" : "NO") << std::endl;
        
        videoLabel->setPixmap(currentPixmap);
        scaleImage();
        
        std::cout << "  → Called videoLabel->setPixmap() and scaleImage()" << std::endl;
    } else {
        std::cout << "  ERROR: Frame NOT in buffer!" << std::endl;
    }
}

void VideoPlayer::scaleImage()
{
    if (currentPixmap.isNull()) return;

    QSize availableSize = scrollArea->viewport()->size();
    if (availableSize.isEmpty()) return;

    QPixmap scaledPixmap = currentPixmap.scaled(
        availableSize.width(),
        availableSize.height(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    
    videoLabel->setPixmap(scaledPixmap);
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