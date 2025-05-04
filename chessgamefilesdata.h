#ifndef CHESSGAMEFILESDATA_H
#define CHESSGAMEFILESDATA_H

#include <QSettings>


class ChessGameFilesData  : public QSettings
{
    Q_OBJECT
public:
    ChessGameFilesData();


    void loadData();
    void saveData();

    void addNewGame(QString gameFile);
    void removeGameFile(QString gameFile);

    QList<QString> getGameFilesList();


protected:

private:
    QString m_dataFile;


private slots:



signals:
};


#endif // CHESSGAMEFILESDATA_H
