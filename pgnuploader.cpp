#include "pgnuploader.h"
#include "ui_pgnuploader.h"


#include <QDebug>
#include <QFileDialog>
#include <QPushButton>


PGNUploader::PGNUploader(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PGNUploader)
{
    ui->setupUi(this);

    connect(ui->uploadButton, &QPushButton::released, this, &PGNUploader::getFile);

    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    // this->setWindowModality(Qt::ApplicationModal);
}


void PGNUploader::getFile(){
    QString file_name = QFileDialog::getOpenFileName(this, "Open a file");

    ui->plainTextEdit->setPlainText(file_name);




}

QString PGNUploader::getFileName(){
    return ui->plainTextEdit->toPlainText();
}

PGNUploader::~PGNUploader()
{
    delete ui;
}
