#include "chesstabhost.h"
#include "databaseviewer.h"

#include <QApplication>
#include <QFile>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    // app.setStyleSheet(style);

    // MainWindow w;
    // w.show();

    ChessTabHost host;
    host.setWindowState(Qt::WindowMaximized);
    host.show();



    // testWidget test;
    // test.show();

    // Form form;
    // form.show();


    return app.exec();
}
