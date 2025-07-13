#include "helpers.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

// Reads a qss file into a QString
QString getStyle(QString s){
    QFile styleFile(s);
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());

    return style;
}

QString getDatabasePathForPGN(const QString &pgnFilePath) {
    QFileInfo fileInfo(pgnFilePath);
    
    //store in AppData
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString dbDir = appDataPath + "/databases";
    
    // create directory if not exist
    QDir().mkpath(dbDir);
    
    return dbDir + "/" + fileInfo.baseName() + ".db";
}
