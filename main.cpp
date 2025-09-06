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
#include "helpers.h"
#include "chessposition.h"

int main(int argc, char *argv[])
{
    QSettings settings;
    QString theme = settings.value("theme", "light").toString();

    QCoreApplication::setOrganizationName("ChessMD");
    QCoreApplication::setApplicationName("ChessMD");
    QCoreApplication::setApplicationVersion("v1.0-beta.3");
    QApplication app(argc, argv);
    
    Theme::applyTheme(app);

    //global style cuz of weird default styling
    QString globalStyle = getStyle(":/resource/styles/globalstyle.qss");
    app.setStyleSheet(globalStyle);

    QDir dir;
    if (!dir.exists("./opening")) dir.mkdir("./opening");
    if (!dir.exists("./engine")) dir.mkdir("./engine");

    app.setWindowIcon(QIcon(":/resource/img/logo.png"));

    initZobristTables();
    qRegisterMetaType<SimpleMove>("SimpleMove");

    // render the main window
    MainWindow w;
    w.show();

    // process user and system events
    return app.exec();
}
