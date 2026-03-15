#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application information
    QApplication::setApplicationName("H.264 Video Player MDI");

    // Create and show main window
    MainWindow window;
    window.setWindowTitle("H.264 Video Player");
    window.resize(1280, 720);
    window.show();

    return app.exec();
}