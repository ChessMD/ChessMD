#ifndef GAMEHOST_H
#define GAMEHOST_H

#include <QObject>
#include <QWidget>
#include <QTabBar>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "ChessGameWindow.h"

class gamehost : public QObject
{
    Q_OBJECT
public:
    explicit gamehost(QObject *parent = nullptr);
    void addNewTab();

private slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);

private:
    QTabBar* tabBar;
    QStackedWidget* stack;

signals:
};

#endif // GAMEHOST_H
