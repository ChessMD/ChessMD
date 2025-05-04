#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "chessgamewindow.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // QWidget *centralWidget = new QWidget(this);
    // setCentralWidget(centralWidget);

    // ChessGameWindow* gameWindow = new ChessGameWindow;
    // gameWindow->show();


    // QWidget* container = new QWidget;
    // QVBoxLayout* containerLayout = new QVBoxLayout(container);
    // containerLayout->setContentsMargins(0, 0, 0, 0);
    // containerLayout->addWidget(gameWindow);


}

MainWindow::~MainWindow()
{
    delete ui;
}
