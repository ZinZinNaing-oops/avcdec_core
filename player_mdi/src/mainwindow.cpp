#include "../include/mainwindow.h"
#include "../include/imageviewer.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), windowMenu(nullptr)
{
    // Create central MDI area
    createCentralWidget();

    // Create UI components
    createMenuBar();
    createToolBar();
    createStatusBar();

    // Connect signals
    connect(mdiArea, &QMdiArea::subWindowActivated,
            this, &MainWindow::updateWindowMenu);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createCentralWidget()
{
    mdiArea = new QMdiArea;
    mdiArea->setBackground(QBrush(Qt::gray));
    setCentralWidget(mdiArea);
}

void MainWindow::createMenuBar()
{
    // File Menu
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

    // Window Menu
    windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

    // View Menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    QAction *cascadeAction = viewMenu->addAction(tr("&Cascade"));
    connect(cascadeAction, &QAction::triggered, this, &MainWindow::cascade);

    QAction *tileAction = viewMenu->addAction(tr("&Tile"));
    connect(tileAction, &QAction::triggered, this, &MainWindow::tile);

    // Help Menu
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
    statusBar()->addWidget(statusLabel);

    progressBar = new QProgressBar;
    progressBar->setMaximumWidth(150);
    progressBar->setVisible(false);
    statusBar()->addPermanentWidget(progressBar);

    decoderStatusLabel = new QLabel(tr("Idle"));
    decoderStatusLabel->setMinimumWidth(200);
    statusBar()->addPermanentWidget(decoderStatusLabel);
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open H.264 File"), QString(),
        tr("H.264 Files (*.264 *.h264);;All Files (*)"));

    if (!fileName.isEmpty()) {
        QMdiSubWindow *child = createChildWindow();
        ImageViewer *viewer = qobject_cast<ImageViewer*>(child->widget());

        if (viewer) {
            connect(viewer, &ImageViewer::statusChanged,
                    this, &MainWindow::onDecoderStatusChanged);
            connect(viewer, &ImageViewer::decodingProgress,
                    this, &MainWindow::onDecodingProgress);

            if (viewer->openFile(fileName)) {
                child->setWindowTitle(QFileInfo(fileName).fileName());
                statusLabel->setText(tr("Opened: %1").arg(QFileInfo(fileName).fileName()));
            } else {
                QMessageBox::warning(this, tr("Error"),
                    tr("Failed to open file: %1").arg(fileName));
                child->close();
            }
        }
    }
}

QMdiSubWindow* MainWindow::createChildWindow()
{
    ImageViewer *viewer = new ImageViewer;
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(viewer);
    subWindow->setAttribute(Qt::WA_DeleteOnClose);
    viewer->show();
    return subWindow;
}

void MainWindow::closeCurrentWindow()
{
    if (QMdiSubWindow *activeWindow = mdiArea->activeSubWindow()) {
        activeWindow->close();
    }
}

void MainWindow::closeAllWindows()
{
    mdiArea->closeAllSubWindows();
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

    QAction *cascadeAction = windowMenu->addAction(tr("&Cascade"));
    connect(cascadeAction, &QAction::triggered, this, &MainWindow::cascade);

    QAction *tileAction = windowMenu->addAction(tr("&Tile"));
    connect(tileAction, &QAction::triggered, this, &MainWindow::tile);

    windowMenu->addSeparator();

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    int numWindows = windows.count();

    for (int i = 0; i < numWindows; ++i) {
        QMdiSubWindow *subWindow = windows.at(i);
        QString text = tr("&%1 %2").arg(i + 1).arg(subWindow->windowTitle());
        QAction *action = windowMenu->addAction(text);
        action->setCheckable(true);
        action->setChecked(subWindow == mdiArea->activeSubWindow());
        connect(action, &QAction::triggered, [subWindow]() {
            subWindow->setFocus();
        });
    }
}

void MainWindow::onDecoderStatusChanged(const QString &message)
{
    decoderStatusLabel->setText(message);
}

void MainWindow::onDecodingProgress(int current, int total)
{
    if (total > 0) {
        progressBar->setMaximum(total);
        progressBar->setValue(current);
        progressBar->setVisible(true);
    } else {
        progressBar->setVisible(false);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    closeAllWindows();
    event->accept();
}