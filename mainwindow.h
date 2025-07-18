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

    void setStatusBarText(const QString &text);

protected:
    void showEvent(QShowEvent *ev) override;

private:

    void setupSidebar();

    DatabaseLibrary *m_dbLibrary;
    QMenuBar * m_menuBar;

private slots:

    void onAddGame();
    void onSettings();


signals:


};


#endif // MAINWINDOW_H
