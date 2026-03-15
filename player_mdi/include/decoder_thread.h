#ifndef DECODER_THREAD_H
#define DECODER_THREAD_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <memory>
#include "AvcDecoder.h"

class Avcdec;

/**
 * @brief H.264ビットストリームをバックグラウンドでデコードするクラス。
 */
class DecoderThread : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief デコーダを生成する。
     * @param parent 親QObject。
     */
    DecoderThread(QObject *parent = nullptr);

    /**
     * @brief デコーダを破棄する。
     */
    ~DecoderThread();

    /**
     * @brief 入力ビットストリームのファイルパスを設定する。
     * @param fileName 入力する .264 ファイルのパス。
     */
    void setInputFile(const QString &fileName);

    /**
     * @brief デコード停止を要求する。
     */
    void stopDecoding();

public slots:
    /**
     * @brief 現在設定されている入力ファイルでデコードを開始する。
     */
    void startDecoding();

signals:
    /**
     * @brief 1フレームのデコードが完了したときに通知する。
     * @param pixmap デコード済みフレーム画像。
     * @param frameNumber 現在のデコードフレーム番号。
     * @param totalFrames 現時点で把握している総フレーム数。
     */
    void frameDecoded(const QPixmap &pixmap, int frameNumber, int totalFrames);

    /**
     * @brief デコードセッション開始時に通知する。
     */
    void decodingStarted();

    /**
     * @brief デコードセッション終了時に通知する。
     */
    void decodingFinished();

    /**
     * @brief デコード失敗時に通知する。
     * @param error UI表示用のエラーメッセージ。
     */
    void decodingError(const QString &error);

private:
    /**
     * @brief YUV420生データをQPixmapへ変換する。
     * @param yuvData 入力YUV420バッファ。
     * @param width フレーム幅。
     * @param height フレーム高さ。
     * @return 変換後のpixmap。失敗時はnull pixmap。
     */
    QPixmap yuv420ToQPixmap(const unsigned char *yuvData, int width, int height);

    std::unique_ptr<Avcdec> decoder;
    QString inputFile;
    bool shouldStop;
};

#endif // DECODER_THREAD_H