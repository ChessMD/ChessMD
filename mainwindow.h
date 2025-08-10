#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pgngamedata.h"

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QQueue>

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
    void enqueueChesscomFetch(const QString &username, int maxGames);

    DatabaseLibrary* m_dbLibrary;
    QMenuBar* m_menuBar;
    QHash<QString, QThread*> m_saveThreads;
    QQueue<QPair<QString, int>> m_chesscomFetchQueue;
    bool m_chesscomFetchRunning = false;

private slots:
    void onAddGame();
    void onSettings();
    void onImportOnlineDatabase();
    void startNextChesscomFetch();
    void onPGNReady(const QString &combinedPGN, const QString &suggestedFilename);

signals:
    void PGNFetchError(const QString &errorMessage);
    void PGNReady(const QString &combinedPGN, const QString &suggestedFilename);

};


#endif // MAINWINDOW_H
