#include "databaseuploader.h"
#include "ui_databaseuploader.h"

DatabaseUploader::DatabaseUploader(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DatabaseUploader)
{
    ui->setupUi(this);
}

DatabaseUploader::~DatabaseUploader()
{
    delete ui;
}
