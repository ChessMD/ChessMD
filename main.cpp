#include "mainwindow.h"
#include "testwidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    // w.show();

    testWidget test;
    test.show();

    // Form form;
    // form.show();


    return app.exec();
}
