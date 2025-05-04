#ifndef PGNUPLOADER_H
#define PGNUPLOADER_H

#include <QDialog>

namespace Ui {
class PGNUploader;
}

class PGNUploader : public QDialog
{
    Q_OBJECT

public:
    explicit PGNUploader(QWidget *parent = nullptr);
    QString getFileName();
    ~PGNUploader();

private:
    void getFile();
    Ui::PGNUploader *ui;
};

#endif // PGNUPLOADER_H
