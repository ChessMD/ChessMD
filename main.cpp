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
#include <QSettings>
#include "mainwindow.h"
#include "theme.h"



int main(int argc, char *argv[])
{
    QSettings settings;
    QString theme = settings.value("theme", "light").toString();

    QCoreApplication::setOrganizationName("ChessMD");
    QCoreApplication::setApplicationName("ChessMD");
    QApplication app(argc, argv);
    
    Theme::applyTheme(app);

    //global style cuz of weird default styling
    app.setStyleSheet(
        "QPushButton {"
        "  border: 1px solid gray;"
        "  border-radius: 6px;"
        "  padding: 3px 8px;"
        "  background-color: palette(button);"
        "  color: palette(button-text);"
        "  font-size: 12px;"
        "}"
        "QPushButton:focus {"
        "  border: 2px solid palette(highlight);"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(128,128,128,0.3);"
        "}"
        "QPushButton:pressed {"
        "  background-color: palette(mid);"
        "}"
    );

    QDir dir;
    if (!dir.exists("./opening")) dir.mkdir("./opening");
    
    app.setWindowIcon(QIcon(":/resource/img/logo.png"));


    // render the main window
    MainWindow w;
    w.show();

    // process user and system events
    return app.exec();
}
