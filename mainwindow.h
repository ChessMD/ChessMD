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

    DatabaseLibrary* m_dbLibrary;
    QMenuBar* m_menuBar;
    QHash<QString, QThread*> m_saveThreads;


private slots:
    void onAddGame();
    void onSettings();


signals:


};


#endif // MAINWINDOW_H
