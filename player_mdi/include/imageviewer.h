#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QPixmap>
#include <QThread>
#include <memory>

class DecoderThread;

class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer();

    bool openFile(const QString &fileName);
    void closeFile();

signals:
    void statusChanged(const QString &message);
    void decodingProgress(int current, int total);

private slots:
    void onFrameDecoded(const QPixmap &pixmap, int frameNumber, int totalFrames);
    void onDecodingStarted();
    void onDecodingFinished();
    void onDecodingError(const QString &error);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();
    void scaleImage();

    // UI Components
    QScrollArea *scrollArea;
    QLabel *imageLabel;
    QLabel *infoLabel;

    // Decoder thread
    DecoderThread *decoderThread;
    QThread *workerThread;

    // State
    QPixmap currentPixmap;
    QString currentFileName;
    bool isDecoding;
};

#endif // IMAGEVIEWER_H