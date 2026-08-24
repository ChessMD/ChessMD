#include <QApplication>
#include <QOperatingSystemVersion>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#if defined(Q_OS_WIN)
#include <QNtfsPermissionCheckGuard>
#endif
#include "chessqsettings.h"

ChessQSettings::ChessQSettings()
{ 
#if defined(Q_OS_WIN)
    QNtfsPermissionCheckGuard permissionGuard;
#endif
 
	QString applicationPath = QApplication::applicationDirPath();
    QFileInfo programDirInfo(applicationPath);
    if (!programDirInfo.isWritable()) {
		m_settingsFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/settings.ini";
    } else {
    	m_settingsFile = applicationPath + "/settings.ini";
    }
}

void ChessQSettings::loadSettings()
{
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    m_engineFile = settings.value("engineFile", "").toString();

}

void ChessQSettings::saveSettings()
{
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.setValue("engineFile", m_engineFile);
    settings.sync();
}


void ChessQSettings::setEngineFile(QString file)
{
    m_engineFile = file;
}

QString ChessQSettings::getEngineFile()
{
    return m_engineFile;
}

QString ChessQSettings::defaultOpeningDirectory()
{
    QDir dir(QDir::current());

    if (QOperatingSystemVersion::current().type() == QOperatingSystemVersion::MacOS) {
        dir.setPath(QApplication::applicationDirPath());
        for (int i = 0; i < 3; i++){
            dir.cdUp();
        }
    }

    return dir.filePath("./opening");
}

QString ChessQSettings::getOpeningDirectory() const
{
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    const QString directory = settings.value("openingDirectory", defaultOpeningDirectory()).toString();
    return directory.isEmpty() ? defaultOpeningDirectory() : directory;
}

void ChessQSettings::setOpeningDirectory(const QString &directory)
{
    QSettings settings(m_settingsFile, QSettings::IniFormat);

    if (directory.isEmpty()) {
        settings.remove("openingDirectory"); // reset to default behavior
    } else {
        settings.setValue("openingDirectory", directory);
    }

    settings.sync();
}
