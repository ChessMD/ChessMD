#include "helpers.h"

#include <QFile>

QString getStyle(QString s){
    QFile styleFile(s);
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());

    return style;
}
