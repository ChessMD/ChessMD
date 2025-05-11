#ifndef DATABASEUPLOADER_H
#define DATABASEUPLOADER_H

#include <QDialog>

namespace Ui {
class DatabaseUploader;
}

// Simple widget for database upload
class DatabaseUploader : public QDialog
{
    Q_OBJECT

public:
    explicit DatabaseUploader(QWidget *parent = nullptr);
    ~DatabaseUploader();

private:
    Ui::DatabaseUploader *ui;
};

#endif // DATABASEUPLOADER_H
