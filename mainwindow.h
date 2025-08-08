#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pgngamedata.h"

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>

class DatabaseLibrary;


class MainWindow  : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

    void setStatusBarText(const QString &text);

public slots:
    void onSaveRequested(const QString &filePath, const QVector<PGNGame> &database);

protected:
    void showEvent(QShowEvent *ev) override;

private:
    void setupToolbar();
    QWidget* setupSidebar();

    void fetchChesscomGamesAndSave(const QString &username, const int maxGames);

    DatabaseLibrary* m_dbLibrary;
    QMenuBar* m_menuBar;
    QHash<QString, QThread*> m_saveThreads;


private slots:
    void onAddGame();
    void onSettings();
    void onImportOnlineDatabase();
    void onPGNReady(const QString &combinedPGN, const QString &suggestedFilename);

signals:
    void PGNFetchError(const QString &errorMessage);
    void PGNReady(const QString &combinedPGN, const QString &suggestedFilename);

};


#endif // MAINWINDOW_H
