#ifndef PGNUPLOADER_H
#define PGNUPLOADER_H

#include <QDialog>

namespace Ui {
class PGNUploader;
}

// PGNUploader is a widget that lets the user select a PGN file locally
class PGNUploader : public QDialog
{
    Q_OBJECT

public:
    explicit PGNUploader(QWidget *parent = nullptr);
    QString getFileName();
    ~PGNUploader() override;

private:
    void getFile();
    Ui::PGNUploader *ui;
};

#endif // PGNUPLOADER_H
