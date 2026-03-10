#include "../include/decoder_thread.h"
#include <QCoreApplication>
#include <QDebug>
#include <fstream>
#include <vector>
#include <cstring>

DecoderThread::DecoderThread(QObject *parent)
    : QObject(parent), shouldStop(false)
{
}

DecoderThread::~DecoderThread()
{
    stopDecoding();
}

void DecoderThread::setInputFile(const QString &fileName)
{
    inputFile = fileName;
}

void DecoderThread::stopDecoding()
{
    shouldStop = true;
    if (decoder) {
        decoder->vdec_stop();
    }
}

void DecoderThread::startDecoding()
{
    if (inputFile.isEmpty()) {
        emit decodingError("No input file specified");
        return;
    }

    emit decodingStarted();

    try {
        DECPARAM_AVC param = {};
        param.bs_buf_size = 11718750;
        param.disp_buf_num = 16;
        param.disp_format = 0;
        param.disp_max_width = 1920;
        param.disp_max_height = 1080;
        param.target_profile = 100;
        param.target_level = 42;

        decoder = std::make_unique<Avcdec>(&param);
        decoder->vdec_start(0, 0);

        std::ifstream file(inputFile.toStdString(), std::ios::binary);
        if (!file.is_open()) {
            emit decodingError(QString("Cannot open file: %1").arg(inputFile));
            emit decodingFinished();
            return;
        }

        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> chunk(65536);
        shouldStop = false;

        qDebug() << "DecoderThread: Starting to feed data";

        // FEED PHASE
        while (file && !shouldStop) {
            file.read((char*)chunk.data(), chunk.size());
            std::streamsize bytesRead = file.gcount();

            if (bytesRead <= 0) break;

            unsigned int ret = decoder->vdec_put_bs(
                chunk.data(),
                bytesRead,
                file.eof() ? 1 : 0,
                0, 0, 0
            );

            if (ret == (unsigned int)-1) {
                emit decodingError("Error feeding data to decoder");
                break;
            }
        }

        file.close();

        // RETRIEVAL PHASE
        int picCount = 0;

        qDebug() << "DecoderThread: Retrieving decoded frames";

        while (!shouldStop) {
            PICMETAINFO_AVC picInfo = {};
            unsigned char* yuv = decoder->vdec_get_picture(&picInfo);

            if (!yuv) break;

            picCount++;

            // ✅ CRITICAL: Convert to independent pixmap
            QPixmap pixmap = yuv420ToQPixmap(yuv, picInfo.pic_width, picInfo.pic_height);

            // ✅ Ensure deep copy
            pixmap = pixmap.copy();

            // ✅ EMIT BEFORE RELEASE
            emit frameDecoded(pixmap, picCount, picCount);

            // ✅ RELEASE IMMEDIATELY AFTER CONVERSION
            decoder->vdec_release_pic_buffer(yuv);

            qDebug() << "Frame" << picCount << "- converted and released";

            QCoreApplication::processEvents();
        }

        qDebug() << "DecoderThread: Total frames decoded:" << picCount;

        decoder->vdec_stop();
        decoder.reset();

    } catch (const std::exception &e) {
        emit decodingError(QString("Decoding error: %1").arg(e.what()));
    }

    emit decodingFinished();
}

QPixmap DecoderThread::yuv420ToQPixmap(const unsigned char *yuvData, int width, int height)
{
    if (!yuvData || width <= 0 || height <= 0) {
        return QPixmap();
    }

    try {
        // ✅ OWN MEMORY for RGB (not decoder's)
        std::vector<unsigned char> rgbData(width * height * 3);

        int ret = decoder->vdec_YUV420toRGB24(
            0,
            rgbData.data(),
            (unsigned char*)yuvData,
            width,
            height
        );

        if (ret <= 0) {
            qWarning() << "Conversion failed";
            return QPixmap();
        }

        // ✅ Create image from OUR buffer
        QImage image(rgbData.data(), width, height, width * 3, QImage::Format_RGB888);

        if (image.isNull()) {
            return QPixmap();
        }

        // ✅ COPY to ensure independence
        QImage imageCopy = image.copy();

        // ✅ Create QPixmap (owns its data now)
        QPixmap pixmap = QPixmap::fromImage(imageCopy);

        return pixmap;

    } catch (const std::exception &e) {
        qWarning() << "Exception:" << e.what();
        return QPixmap();
    }
}