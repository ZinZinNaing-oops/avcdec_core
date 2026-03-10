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

class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();

    bool openFile(const QString &fileName);
    void closeFile();

signals:
    void statusChanged(const QString &message);

private slots:
    void onFrameDecoded(const QPixmap &pixmap, int frameNumber, int totalFrames);
    void onDecodingFinished();
    void onDecodingError(const QString &error);
    
    void onPlayClicked();
    void onPauseClicked();
    void onStopClicked();
    void onSliderMoved(int value);
    void onPlaybackTimer();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();
    void scaleImage();
    void updateTimeLabel();
    void displayFrame(int frameIndex);
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