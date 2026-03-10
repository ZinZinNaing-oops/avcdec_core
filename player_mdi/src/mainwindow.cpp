#include "../include/mainwindow.h"
#include "../include/videoplayer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QFileInfo>
#include <QCloseEvent>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), windowMenu(nullptr)
{
    createCentralWidget();
    createMenuBar();
    createToolBar();
    createStatusBar();

    connect(mdiArea, &QMdiArea::subWindowActivated,
            this, &MainWindow::updateWindowMenu);

    setWindowTitle(tr("H.264 Video Player"));
    resize(1024, 768);
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    closeAllWindows();
    event->accept();
}

void MainWindow::createCentralWidget()
{
    mdiArea = new QMdiArea;
    mdiArea->setBackground(QBrush(Qt::gray));
    setCentralWidget(mdiArea);
}

void MainWindow::createMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openAction = fileMenu->addAction(tr("&Open H.264 File..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    fileMenu->addSeparator();

    QAction *closeAction = fileMenu->addAction(tr("&Close Current Window"));
    closeAction->setShortcut(QKeySequence::Close);
    connect(closeAction, &QAction::triggered, this, &MainWindow::closeCurrentWindow);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::exit);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    QAction *cascadeAction = viewMenu->addAction(tr("&Cascade"));
    connect(cascadeAction, &QAction::triggered, this, &MainWindow::cascade);

    QAction *tileAction = viewMenu->addAction(tr("&Tile"));
    connect(tileAction, &QAction::triggered, this, &MainWindow::tile);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About H.264 Video Player MDI"),
            tr("H.264 Video Player MDI v1.0\n\n"
               "A cross-platform video player for H.264 encoded videos\n"
               "Using Qt6/Qt5 with AvcDecoder\n\n"
               "© 2026"));
    });
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Standard"));
    toolBar->setObjectName("StandardToolBar");

    QAction *openAction = toolBar->addAction(tr("Open"));
    openAction->setToolTip(tr("Open H.264 File"));
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    toolBar->addSeparator();

    QAction *closeAction = toolBar->addAction(tr("Close"));
    closeAction->setToolTip(tr("Close Current Window"));
    connect(closeAction, &QAction::triggered, this, &MainWindow::closeCurrentWindow);
}

void MainWindow::createStatusBar()
{
    statusLabel = new QLabel(tr("Ready"));
    statusBar()->addWidget(statusLabel, 1);

    progressBar = new QProgressBar;
    progressBar->setMaximumWidth(200);
    progressBar->setVisible(false);
    statusBar()->addWidget(progressBar);

    decoderStatusLabel = new QLabel(tr("No file loaded"));
    statusBar()->addPermanentWidget(decoderStatusLabel, 1);
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open H.264 Video"), "",
        tr("H.264 Files (*.h264 *.264 *.avc);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        VideoPlayer *videoPlayer = new VideoPlayer();
        QMdiSubWindow *subWindow = mdiArea->addSubWindow(videoPlayer);
        
        connect(videoPlayer, &VideoPlayer::statusChanged,
                this, &MainWindow::onDecoderStatusChanged);
        
        QString baseName = QFileInfo(fileName).fileName();
        subWindow->setWindowTitle(tr("Video - %1").arg(baseName));
        subWindow->setAttribute(Qt::WA_DeleteOnClose);
        subWindow->resize(640, 480);
        
        videoPlayer->openFile(fileName);
        videoPlayer->show();
    }
}

void MainWindow::closeCurrentWindow()
{
    if (mdiArea->currentSubWindow())
        mdiArea->currentSubWindow()->close();
}

void MainWindow::closeAllWindows()
{
    QList<QMdiSubWindow*> windows = mdiArea->subWindowList();
    for (QMdiSubWindow *window : windows)
        window->close();
}

void MainWindow::exit()
{
    QApplication::quit();
}

void MainWindow::cascade()
{
    mdiArea->cascadeSubWindows();
}

void MainWindow::tile()
{
    mdiArea->tileSubWindows();
}

void MainWindow::updateWindowMenu()
{
    windowMenu->clear();
    
    QList<QMdiSubWindow*> windows = mdiArea->subWindowList();
    
    if (windows.count() == 0) {
        windowMenu->addAction(tr("No windows"));
        return;
    }
    
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *mdiSubWindow = windows.at(i);
        QString text = tr("&%1 %2").arg(i + 1).arg(mdiSubWindow->windowTitle());
        
        QAction *action = windowMenu->addAction(text);
        action->setCheckable(true);
        action->setChecked(mdiArea->activeSubWindow() == mdiSubWindow);
        
        connect(action, &QAction::triggered, mdiSubWindow, [mdiSubWindow]() {
            mdiSubWindow->showNormal();
            mdiSubWindow->setFocus();
        });
    }
}

void MainWindow::onDecoderStatusChanged(const QString &message)
{
    statusLabel->setText(message);
}

void MainWindow::onDecodingProgress(int current, int total)
{
    if (total > 0) {
        progressBar->setVisible(true);
        progressBar->setValue((current * 100) / total);
    } else {
        progressBar->setVisible(false);
    }
}