#include "pgnuploader.h"
#include "ui_pgnuploader.h"


#include <QDebug>
#include <QFileDialog>
#include <QPushButton>

// Construct the widget
PGNUploader::PGNUploader(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PGNUploader)
{

    // connect ui and init
    ui->setupUi(this);
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    // makes it so you must complete the action before interacting elsewhere
    this->setWindowModality(Qt::ApplicationModal);

    // signals and slots
    connect(ui->uploadButton, &QPushButton::released, this, &PGNUploader::getFile);


}

// Lets users pick a file from their local computer
void PGNUploader::getFile(){


    QString file_name = QFileDialog::getOpenFileName(this, "Select a chess PGN file", "", "PGN files (*.pgn)");

    ui->plainTextEdit->setPlainText(file_name);




}

QString PGNUploader::getFileName(){
    return ui->plainTextEdit->toPlainText();
}

PGNUploader::~PGNUploader()
{
    delete ui;
}
