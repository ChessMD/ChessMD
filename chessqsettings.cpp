#include <QApplication>
#include <QOperatingSystemVersion>
#include <QStandardPaths>
#include <QFileInfo>
#include "chessqsettings.h"

ChessQSettings::ChessQSettings()
{ 
    QFileInfo program_path(QApplication::applicationDirPath());
    if (!program_path.isWritable()) {

    	QFileInfo info(".");
   	if (info.isWritable())
	    m_settingsFile = "./settings.ini";
    	else
	    m_settingsFile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/settings.ini";
    } else {
    	m_settingsFile = QApplication::applicationDirPath() + "/settings.ini";
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
