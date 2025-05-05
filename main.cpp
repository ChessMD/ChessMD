#include "chesstabhost.h"
#include "databaseviewer.h"
#include "databaselibrary.h"

#include <QApplication>
#include <QFile>
#include <QSqlDatabase>



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("library.db");
    db.open();

    ChessTabHost host;
    host.addNewTab(new DatabaseLibrary, "New Tab");
    host.setWindowState(Qt::WindowMaximized);
    host.show();

    return app.exec();
}
