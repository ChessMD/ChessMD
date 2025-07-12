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
#include <QDockWidget>
#include "mainwindow.h"
#include <QSqlDatabase>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //render the main window
    MainWindow w;
    w.show();

    //process user and system events
    return app.exec();
}
