#include "chessgamefilesdata.h"
#include <QApplication>
#include <QOperatingSystemVersion>
#include <QStandardPaths>
#include <QDir>

ChessGameFilesData::ChessGameFilesData()
{
#if defined(Q_OS_WIN)
	QNtfsPermissionCheckGuard permissionGuard; 
#endif

	QString applicationPath = QApplication::applicationDirPath();
    QFileInfo programDirInfo(applicationPath);
    if (!programDirInfo.isWritable()) {
		m_dataFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/chessgamefiles.ini";
    } else {
    	m_dataFile = applicationPath + "/chessgamefiles.ini";
    }
}

void ChessGameFilesData::saveData()
{
    QSettings settings(m_dataFile, QSettings::IniFormat);
    settings.sync();
}

void ChessGameFilesData::addNewGame(QString gameFile)
{
    QList<QString> gameFilesList;
    int size;

    if (gameFile.isEmpty())
        return;

    QSettings data(m_dataFile, QSettings::IniFormat);

    size = data.beginReadArray("GameFilesList");
    for (int i = 0; i < size; ++i) {
        data.setArrayIndex(i);
        QString name = data.value("name").toString();
        gameFilesList.append(name);
    }
    data.endArray();

    if (gameFilesList.contains(gameFile))
        return;

    gameFilesList.append(gameFile);
    size =  gameFilesList.size();

    data.beginWriteArray("GameFilesList");
    for (int i = 0; i < size; ++i) {
        data.setArrayIndex(i);
        data.setValue("name", gameFilesList[i]);
    }
    data.endArray();

    data.sync();
}

void ChessGameFilesData::removeGameFile(QString gameFile)
{
    QList<QString> gameFilesList;
    int size;

    if (gameFile.isEmpty())
        return;

    QSettings data(m_dataFile, QSettings::IniFormat);

    size = data.beginReadArray("GameFilesList");
    for (int i = 0; i < size; ++i) {
        data.setArrayIndex(i);
        QString name = data.value("name").toString();
        gameFilesList.append(name);
    }
    data.endArray();

    if (gameFilesList.contains(gameFile) == false)
        return;

    gameFilesList.removeOne(gameFile);

    size =  gameFilesList.size();

    data.beginWriteArray("GameFilesList");
    data.remove(""); // remove all data under gameFilesList

    // rebuild data for gameFilesList
    for (int i = 0; i < size; ++i) {
        data.setArrayIndex(i);
        data.setValue("name", gameFilesList[i]);
    }
    data.endArray();

    data.sync();
}


QList<QString> ChessGameFilesData::getGameFilesList()
{
    QList<QString> gameFilesList;
    int size;

    QSettings data(m_dataFile, QSettings::IniFormat);

    size = data.beginReadArray("GameFilesList");
    for (int i = 0; i < size; ++i) {
        data.setArrayIndex(i);
        QString name = data.value("name").toString();
        QFileInfo fileInfo(name);
        if (fileInfo.exists()) gameFilesList.append(name);
    }
    data.endArray();

    return gameFilesList;
}


