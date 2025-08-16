/*
ChessMD
ICS4U
Michael Qu, Daniel Xu
Main file
History:
Jan 15, 2025 - Program Creation
*/

#include <QApplication>
#include <QCoreApplication>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QDockWidget>
#include "mainwindow.h"
#include "theme.h"



int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
#endif

    QCoreApplication::setOrganizationName("ChessMD");
    QCoreApplication::setApplicationName("ChessMD");
    QCoreApplication::setApplicationVersion("v1.0-beta.3");
    QApplication app(argc, argv);
    
    Theme::applyTheme(app);

    QDir dir;
    if (!dir.exists("./opening")) dir.mkdir("./opening");
    if (!dir.exists("./engine")) dir.mkdir("./engine");

    app.setWindowIcon(QIcon(":/resource/img/logo.png"));


    // render the main window
    MainWindow w;
    w.show();

    // process user and system events
    return app.exec();
}
