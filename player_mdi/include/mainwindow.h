#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QLabel>
#include <QProgressBar>
#include <QCloseEvent>
#include <memory>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu actions
    void openFile();
    void closeCurrentWindow();
    void closeAllWindows();
    void exit();

    // Window menu actions
    void cascade();
    void tile();
    void updateWindowMenu();

    // Decoder status updates
    void onDecoderStatusChanged(const QString &message);
    void onDecodingProgress(int current, int total);

private:
    // UI creation methods
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createCentralWidget();

    // Helper methods
    QMdiSubWindow* createChildWindow();

    // UI Components
    QMdiArea *mdiArea;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QLabel *decoderStatusLabel;

    // Window menu
    QMenu *windowMenu;
};

#endif // MAINWINDOW_H