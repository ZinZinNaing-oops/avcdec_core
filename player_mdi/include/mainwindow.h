#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>
#include <QMdiArea>

class VideoPlayer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void openFile();
    void closeCurrentWindow();
    void cascade();
    void tile();
    void updateWindowMenu();
    void onDecoderStatusChanged(const QString &message);
    void onDecodingProgress(int current, int total);

private:
    void createCentralWidget();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void closeAllWindows();
    void exit();

    QMdiArea *mdiArea;
    QMenu *windowMenu;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QLabel *decoderStatusLabel;
};

#endif // MAINWINDOW_H