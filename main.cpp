#include <QApplication>
#include <QObject>
#include <QFile>
#include <QDockWidget>

#include "chessmainwindow.h"



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    // app.setStyleSheet(style);

    // MainWindow w;
    // w.show();

    ChessMainWindow w;

    w.show();


    // testWidget test;
    // test.show();

    // Form form;
    // form.show();


    return app.exec();
}
