#ifndef CHESSMAINWINDOW_H
#define CHESSMAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>

class DatabaseLibrary;


class ChessMainWindow  : public QMainWindow
{
    Q_OBJECT
public:
    ChessMainWindow();

    void setStatusBarText(const QString &text);

protected:
    void showEvent(QShowEvent *ev);

private:

    void createMenus();

    DatabaseLibrary *m_dbLibrary;
    QMenuBar * m_menuBar;


private slots:

    void onAddGame();
    void onSelectEngineFile();


signals:


};


#endif // CHESSMAINWINDOW_H
