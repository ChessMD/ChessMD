#include "helpers.h"

#include <QFile>
#include <QSettings>



// Reads a qss file into a QString
QString getStyle(QString s){
    QFile styleFile(s);
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());

    return style;
}


QString getIconPath(const QString& name){
    QSettings settings;
    QString theme = settings.value("theme").toString();
    if (theme == "dark") return QString(":/resource/img/white_icons/%1").arg(name);
    else return QString(":/resource/img/%1").arg(name);

}


