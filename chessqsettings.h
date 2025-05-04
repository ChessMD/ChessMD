#ifndef CHESSQSETTINGS_H
#define CHESSQSETTINGS_H

#include <QSettings>


class ChessQSettings  : public QSettings
{
    Q_OBJECT
public:
    ChessQSettings();

    void setEngineFile(QString file);
    void loadSettings();
    void saveSettings();
    QString getEngineFile();


protected:

private:
    QString m_settingsFile;
    QString m_engineFile;

private slots:



signals:
};



#endif // CHESSQSETTINGS_H
