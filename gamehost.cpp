#include "gamehost.h"

#include <QVBoxLayout>
#include <QTabBar>
#include <QStackedWidget>

gamehost::gamehost(QObject *parent)
    : QObject{parent}
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create a movable, closable tab bar
    tabBar = new QTabBar(this);
    tabBar->setMovable(true);
    tabBar->setTabsClosable(true);
    connect(tabBar, &QTabBar::currentChanged, this, &gamehost::onTabChanged);
    connect(tabBar, &QTabBar::tabCloseRequested, this, &gamehost::onTabCloseRequested);

    // Create a stacked widget to hold the tab contents
    stack = new QStackedWidget(this);

    layout->addWidget(tabBar);
    layout->addWidget(stack);

    // Start with one tab
    addNewTab();
}

void gamehost::addNewTab() {
    // Create a new chess game window
    gamewindow* gameWindow = new gamewindow;

    // Since QMainWindow is a QWidget, we wrap it inside a container widget.
    QWidget* container = new QWidget;
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(gameWindow);
    container->setLayout(containerLayout);

    // Add the container to the stack and a corresponding tab to the tab bar.
    int index = stack->addWidget(container);
    QString tabTitle = QString("Game %1").arg(tabBar->count() + 1);
    tabBar->addTab(tabTitle);
    tabBar->setCurrentIndex(index);
}

void gamehost::onTabChanged(int index) {
    stack->setCurrentIndex(index);
}

void gamehost::onTabCloseRequested(int index) {
    QWidget* widget = stack->widget(index);
    stack->removeWidget(widget);
    widget->deleteLater();
    tabBar->removeTab(index);
}
