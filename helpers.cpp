#include "helpers.h"

#include <QFile>

// Reads a qss file into a QString
QString getStyle(QString s){
    QFile styleFile(s);
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());

    return style;
}
