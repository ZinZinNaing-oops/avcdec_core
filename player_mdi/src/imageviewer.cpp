#include "../include/imageviewer.h"
#include "../include/decoder_thread.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QThread>
#include <QFileInfo>
#include <QResizeEvent>

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent), isDecoding(false)
{
    setupUI();

    // Create decoder thread
    workerThread = new QThread(this);
    decoderThread = new DecoderThread();
    decoderThread->moveToThread(workerThread);

    // Connect signals/slots
    connect(workerThread, &QThread::started,
            decoderThread, &DecoderThread::startDecoding);
    connect(decoderThread, &DecoderThread::frameDecoded,
            this, &ImageViewer::onFrameDecoded);
    connect(decoderThread, &DecoderThread::decodingStarted,
            this, &ImageViewer::onDecodingStarted);
    connect(decoderThread, &DecoderThread::decodingFinished,
            this, &ImageViewer::onDecodingFinished);
    connect(decoderThread, &DecoderThread::decodingError,
            this, &ImageViewer::onDecodingError);
    connect(workerThread, &QThread::finished,
            decoderThread, &QObject::deleteLater);
}

ImageViewer::~ImageViewer()
{
    closeFile();
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void ImageViewer::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Create scroll area
    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setAlignment(Qt::AlignCenter);

    imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(200, 200);
    scrollArea->setWidget(imageLabel);

    layout->addWidget(scrollArea);

    // Create info label
    infoLabel = new QLabel(tr("No file loaded"));
    infoLabel->setMaximumHeight(30);
    infoLabel->setStyleSheet("QLabel { background-color: lightgray; padding: 5px; }");
    layout->addWidget(infoLabel);

    setLayout(layout);
}

bool ImageViewer::openFile(const QString &fileName)
{
    if (fileName.isEmpty()) {
        return false;
    }

    currentFileName = fileName;
    emit statusChanged(tr("Opening: %1").arg(QFileInfo(fileName).fileName()));

    // Set decoder file
    decoderThread->setInputFile(fileName);

    // Start decoding thread if not already running
    if (!workerThread->isRunning()) {
        workerThread->start();
    } else {
        decoderThread->startDecoding();
    }

    return true;
}

void ImageViewer::closeFile()
{
    if (isDecoding) {
        decoderThread->stopDecoding();
        workerThread->quit();
        workerThread->wait();
        workerThread = new QThread(this);
        isDecoding = false;
    }
}

void ImageViewer::onFrameDecoded(const QPixmap &pixmap, int frameNumber, int totalFrames)
{
    currentPixmap = pixmap;
    imageLabel->setPixmap(pixmap);
    scaleImage();

    emit decodingProgress(frameNumber, totalFrames);
}

void ImageViewer::onDecodingStarted()
{
    isDecoding = true;
    emit statusChanged(tr("Decoding..."));
}

void ImageViewer::onDecodingFinished()
{
    isDecoding = false;
    emit statusChanged(tr("Decoding finished"));
    emit decodingProgress(0, 0);
}

void ImageViewer::onDecodingError(const QString &error)
{
    isDecoding = false;
    emit statusChanged(tr("Error: %1").arg(error));
}

void ImageViewer::scaleImage()
{
    if (currentPixmap.isNull()) {
        return;
    }

    QSize viewportSize = scrollArea->viewport()->size();
    
    // Use scaled() instead of scaledToFit()
    QPixmap scaledPixmap = currentPixmap.scaled(
        viewportSize.width() - 4,
        viewportSize.height() - 4,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    imageLabel->setPixmap(scaledPixmap);
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!currentPixmap.isNull()) {
        scaleImage();
    }
}