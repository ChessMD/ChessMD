#include <QApplication>
#include <QOperatingSystemVersion>
#include <QStandardPaths>
#include <QFileInfo>
#include <QNtfsPermissionCheckGuard>
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
