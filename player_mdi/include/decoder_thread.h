#ifndef DECODER_THREAD_H
#define DECODER_THREAD_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <memory>
#include "AvcDecoder.h"

class Avcdec;

class DecoderThread : public QObject
{
    Q_OBJECT

public:
    DecoderThread(QObject *parent = nullptr);
    ~DecoderThread();

    void setInputFile(const QString &fileName);
    void stopDecoding();

public slots:
    void startDecoding();

signals:
    void frameDecoded(const QPixmap &pixmap, int frameNumber, int totalFrames);
    void decodingStarted();
    void decodingFinished();
    void decodingError(const QString &error);

private:
    QPixmap yuv420ToQPixmap(const unsigned char *yuvData, int width, int height);

    std::unique_ptr<Avcdec> decoder;
    QString inputFile;
    bool shouldStop;
};

#endif // DECODER_THREAD_H