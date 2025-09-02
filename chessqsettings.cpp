#include <QApplication>

#include "chessqsettings.h"

ChessQSettings::ChessQSettings()
{
    m_settingsFile = "./settings.ini";
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
    qDebug() << m_engineFile << settings.value("engineFile", "").toString();
}


void ChessQSettings::setEngineFile(QString file)
{
    m_engineFile = file;
}

QString ChessQSettings::getEngineFile()
{
    return m_engineFile;
}
