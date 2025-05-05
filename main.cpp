#include <QApplication>
#include <QObject>
#include <QFile>
#include <QDockWidget>

#include "chessmainwindow.h"

#include <QSqlDatabase>



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("library.db");
    db.open();

    ChessMainWindow w;
    w.show();

    return app.exec();
}
