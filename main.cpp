#include <QApplication>
#include <QCoreApplication>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QDockWidget>
#include <QLockFile>

#include "mainwindow.h"
#include "theme.h"
#include "helpers.h"
#include "chessposition.h"
#include "translationmanager.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("ChessMD");
    QCoreApplication::setApplicationName("ChessMD");
    QCoreApplication::setApplicationVersion("v1.1");
    QApplication app(argc, argv);
    QString lockPath = QDir::temp().absoluteFilePath(QCoreApplication::applicationName() + ".lock");
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(30000); // time taken for lock expiry after program unresponsiveness
    if (!lockFile.tryLock(100)) {
        return 0;
    }

    TranslationManager::instance().initializeLanguage();
    Theme::applyTheme(app);

    // global style cuz of weird default styling
    QString globalStyle = getStyle(":/resource/styles/globalstyle.qss");
    app.setStyleSheet(globalStyle);

    app.setWindowIcon(QIcon(":/resource/img/logo.png"));

    initZobristTables();
    qRegisterMetaType<SimpleMove>("SimpleMove");

    // render the main window
    MainWindow w;
    w.show();

    // process user and system events
    return app.exec();
}
