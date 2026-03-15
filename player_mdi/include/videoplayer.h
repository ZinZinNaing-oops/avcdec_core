#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QScrollArea>
#include <QThread>
#include <QTimer>
#include <memory>
#include <map>

class DecoderThread;

/**
 * @brief デコーダスレッド連携を持つ動画再生ウィジェット。
 */
class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 動画再生ウィジェットを生成する。
     * @param parent 親ウィジェット。
     */
    VideoPlayer(QWidget *parent = nullptr);

    /**
     * @brief 動画再生ウィジェットを破棄する。
     */
    ~VideoPlayer();

    /**
     * @brief 入力ビットストリームファイルを開いてデコード処理を開始する。
     * @param fileName 入力する .264/.h264 ファイルパス。
     * @return 開始要求に成功した場合は true、失敗した場合は false。
     */
    bool openFile(const QString &fileName);

    /**
     * @brief 現在のファイルを閉じ、再生/デコードを停止する。
     */
    void closeFile();

signals:
    /**
     * @brief 親UI向けにプレイヤー状態メッセージを通知する。
     * @param message 状態メッセージ。
     */
    void statusChanged(const QString &message);

private slots:
    /**
     * @brief ワーカースレッドから受信したデコード済みフレームを処理する。
     * @param pixmap デコード済みフレーム画像。
     * @param frameNumber 現在のフレーム番号。
     * @param totalFrames 現時点の総フレーム数。
     */
    void onFrameDecoded(const QPixmap &pixmap, int frameNumber, int totalFrames);

    /**
     * @brief デコード完了イベントを処理する。
     */
    void onDecodingFinished();

    /**
     * @brief デコードエラーイベントを処理する。
     * @param error エラーメッセージ。
     */
    void onDecodingError(const QString &error);
    
    /**
     * @brief 再生ボタン押下時の処理。
     */
    void onPlayClicked();

    /**
     * @brief 一時停止ボタン押下時の処理。
     */
    void onPauseClicked();

    /**
     * @brief 停止ボタン押下時の処理。
     */
    void onStopClicked();

    /**
     * @brief フレームスライダー操作時のシーク処理。
     * @param value 目標フレームインデックス。
     */
    void onSliderMoved(int value);

    /**
     * @brief フレーム単位再生用タイマーコールバック。
     */
    void onPlaybackTimer();

protected:
    /**
     * @brief ウィジェットのリサイズイベントを処理する。
     * @param event Qtのリサイズイベント。
     */
    void resizeEvent(QResizeEvent *event) override;

private:
    /**
     * @brief プレイヤーUIを構築し、シグナル/スロットを接続する。
     */
    void setupUI();

    /**
     * @brief 表示用に現在フレームのpixmapをスケーリングする。
     */
    void scaleImage();

    /**
     * @brief フレームインデックスに基づいて経過時間ラベルを更新する。
     */
    void updateTimeLabel();

    /**
     * @brief フレームバッファから指定インデックスのフレームを表示する。
     * @param frameIndex 表示対象フレームインデックス。
     */
    void displayFrame(int frameIndex);

    /**
     * @brief 再生時間ラベルの表示を更新する。
     */
    void updateDurationDisplay();

    // UI Components
    QLabel *videoLabel;
    QLabel *infoLabel;
    QScrollArea *scrollArea;
    
    QPushButton *playButton;
    QPushButton *pauseButton;
    QPushButton *stopButton;
    QSlider *frameSlider;
    QLabel *timeLabel;
    QLabel *durationLabel;

    // Decoder
    DecoderThread *decoderThread;
    QThread *workerThread;

    // Playback state
    int playbackState;  // 0=stopped, 1=playing, 2=paused
    int currentFrameIndex;
    int totalFrames;
    double frameRate;   // Changed to double for precision
    
    QTimer *playbackTimer;
    QPixmap currentPixmap;
    QString currentFileName;
    
    // Frame buffer
    std::map<int, QPixmap> frameBuffer;
    
    // Timing info from first frame
    int firstFrameWidth;
    int firstFrameHeight;
};

#endif // VIDEOPLAYER_H