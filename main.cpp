/*
ChessMD
ICS4U
Michael Qu, Daniel Xu
Main file
History:
Jan 15, 2025 - Program Creation
*/

#include <QApplication>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QDockWidget>
#include "mainwindow.h"


int main(int argc, char *argv[])
{

    qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
    QApplication app(argc, argv);

    QDir dir;
    if (!dir.exists("./opening")) dir.mkdir("./opening");
    


    app.setWindowIcon(QIcon(":/resource/img/logo.png"));

    // render the main window
    MainWindow w;
    w.show();

    // process user and system events
    return app.exec();
}
