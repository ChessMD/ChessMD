#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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


protected:
    void showEvent(QShowEvent *ev);

private:

    void setupSidebar();

    DatabaseLibrary *m_dbLibrary;
    QMenuBar * m_menuBar;


private slots:

    void onAddGame();
    void onSelectEngineFile();
    void onSettings();


signals:


};


#endif // MAINWINDOW_H
