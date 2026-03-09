#include "../include/decoder_thread.h"
#include "AvcDecoder.h"
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDebug>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

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
        param.disp_format = 0;             // YUV420
        param.disp_max_width = 1920;
        param.disp_max_height = 1080;
        param.target_profile = 100;        // High Profile
        param.target_level = 42;           // Level 4.2

        // Create decoder
        decoder = std::make_unique<Avcdec>(&param);

        // Start decoder
        decoder->vdec_start(0, 0);

        std::ifstream file(inputFile.toStdString(), std::ios::binary);
        if (!file.is_open()) {
            emit decodingError(QString("Cannot open file: %1").arg(inputFile));
            emit decodingFinished();
            return;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // Feed data to decoder
        std::vector<unsigned char> chunk(65536);
        shouldStop = false;

        while (file && !shouldStop) {
            file.read((char*)chunk.data(), chunk.size());
            std::streamsize bytesRead = file.gcount();

            if (bytesRead <= 0)
                break;

            // Feed to decoder
            unsigned int ret = decoder->vdec_put_bs(
                chunk.data(),
                bytesRead,
                file.eof() ? 1 : 0,  // END_OF_AU when EOF
                0,                    // PTS
                0,                    // ERR_FLAG
                0                     // ERR_SN_SKIP
            );

            if (ret == (unsigned int)-1) {
                emit decodingError("Error feeding data to decoder");
                break;
            }
        }

        file.close();

        // Get decoded pictures
        int picCount = 0;

        while (!shouldStop) {
            PICMETAINFO_AVC picInfo = {};
            unsigned char* yuv = decoder->vdec_get_picture(&picInfo);

            if (!yuv)
                break;

            picCount++;

            QPixmap pixmap = yuv420ToQPixmap(yuv, picInfo.pic_width, picInfo.pic_height);

            emit frameDecoded(pixmap, picCount, 0);

            // Release picture buffer
            decoder->vdec_release_pic_buffer(yuv);
            QCoreApplication::processEvents();
        }

        // Stop decoder
        decoder->vdec_stop();
        decoder.reset();

    } catch (const std::exception &e) {
        emit decodingError(QString("Decoding error: %1").arg(e.what()));
    }

    emit decodingFinished();
}

/**
 * @brief Converts YUV420 planar data to RGB QPixmap using decoder's built-in function
 * 
 * Parameters:
 *   yuvData  - Pointer to YUV420 planar buffer (Y plane followed by U and V planes)
 *   width    - Frame width in pixels
 *   height   - Frame height in pixels
 * 
 * Returns: QPixmap containing the RGB image, or null pixmap on error
 */
QPixmap DecoderThread::yuv420ToQPixmap(const unsigned char *yuvData, int width, int height)
{
    if (!yuvData || width <= 0 || height <= 0) {
        qWarning() << "Invalid YUV data or dimensions:" << width << "x" << height;
        return QPixmap();
    }

    try {
        // Allocate RGB buffer (3 bytes per pixel)
        int rgbSize = width * height * 3;
        std::vector<unsigned char> rgbData(rgbSize);

        // Call decoder's built-in YUV420 to RGB24 conversion
        // Parameters:
        //   Mode=0: Standard conversion
        //   rgbData: Output RGB buffer
        //   yuvData: Input YUV420 buffer
        //   width: Frame width
        //   height: Frame height
        int ret = decoder->vdec_YUV420toRGB24(
            0,
            rgbData.data(),
            (unsigned char*)yuvData,
            width,
            height
        );

        if (ret <= 0) {
            qWarning() << "vdec_YUV420toRGB24 failed with return code:" << ret;
            return QPixmap();
        }

        // Create QImage from RGB data
        QImage image(rgbData.data(), width, height, width * 3, QImage::Format_RGB888);

        if (image.isNull()) {
            qWarning() << "Failed to create QImage from RGB data";
            return QPixmap();
        }

        // Convert to QPixmap (copy data to ensure ownership)
        QPixmap pixmap = QPixmap::fromImage(image.copy());

        if (pixmap.isNull()) {
            qWarning() << "Failed to convert QImage to QPixmap";
            return QPixmap();
        }

        return pixmap;

    } catch (const std::exception &e) {
        qWarning() << "Exception in yuv420ToQPixmap:" << e.what();
        return QPixmap();
    }
}