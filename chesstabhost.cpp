#include "chesstabhost.h"
#include "chessgamewindow.h"
#include "databaseviewer.h"
#include "databaselibrary.h"

#include <qDebug>

ChessTabHost::ChessTabHost(QWidget* parent)
    : QWidget(parent)
{

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* topBar = new QWidget(this);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->setSpacing(0);

    tabBar = new QTabBar(this);
    tabBar->setMovable(true);
    tabBar->setTabsClosable(true);
    tabBar->setExpanding(true);
    tabBar->setStyleSheet("QTabBar::tab { max-width: 150px; }");
    connect(tabBar, &QTabBar::currentChanged, this, &ChessTabHost::onTabChanged);
    connect(tabBar, &QTabBar::tabCloseRequested, this, &ChessTabHost::onTabCloseRequested);
    connect(tabBar, &QTabBar::tabMoved, this, &ChessTabHost::onTabMoved);

    addTabButton = new QToolButton(topBar);
    addTabButton->setText("+");
    addTabButton->setToolTip("New Tab");
    connect(addTabButton, &QToolButton::clicked, this, &ChessTabHost::onAddTabClicked);

    topBarLayout->addWidget(tabBar);
    topBarLayout->addWidget(addTabButton);
    topBarLayout->setStretch(0, 1); // let the tab bar take available space

    stack = new QStackedWidget(this);

    mainLayout->addWidget(topBar);
    mainLayout->addWidget(stack);

    addNewTab(new DatabaseLibrary, "New Tab");

}

void ChessTabHost::addNewTab(QWidget* embed, QString title) {


    QString tabTitle;


    if(DatabaseLibrary* dbLibrary = qobject_cast<DatabaseLibrary*>(embed)){
        connect(dbLibrary, &DatabaseLibrary::fileDoubleClicked, this, &ChessTabHost::onTabReplaced);
        tabTitle = QString("New Tab");
    }
    else if(ChessGameWindow* gameWindow = qobject_cast<ChessGameWindow*>(embed)){
        tabTitle = QString("Game %1").arg(tabBar->count() + 1);
    }
    else if (DatabaseViewer* dbViewer =  qobject_cast<DatabaseViewer*>(embed)){
        tabTitle = QString("Database %1").arg(tabBar->count() + 1);
        connect(dbViewer, &DatabaseViewer::gameActivated, this, &ChessTabHost::onGameActivated);
    }

    QWidget* container = new QWidget;
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addWidget(embed);
    container->setLayout(containerLayout);



    int index = stack->addWidget(container);
    tabBar->addTab(title);
    tabBar->setCurrentIndex(index);
}

void ChessTabHost::onTabChanged(int index) {
    stack->setCurrentIndex(index);
}

void ChessTabHost::onTabCloseRequested(int index) {
    QWidget* widget = stack->widget(index);
    stack->removeWidget(widget);
    widget->deleteLater();
    tabBar->removeTab(index);
    // Update the current index after removal.
    int currentIndex = tabBar->currentIndex();
    stack->setCurrentIndex(currentIndex);
}

void ChessTabHost::onAddTabClicked() {
    addNewTab(new DatabaseLibrary, "New Tab");
    // addNewTab(new ChessGameWindow, "Board UI");
}

void ChessTabHost::onTabMoved(int from, int to) {
    QWidget* widget = stack->widget(from);
    stack->removeWidget(widget);
    stack->insertWidget(to, widget);
}

void ChessTabHost::onTabReplaced(const QString &fileIdentifier)
{
    onTabCloseRequested(tabBar->currentIndex());
    addNewTab(new DatabaseViewer, fileIdentifier);
}

void ChessTabHost::onGameActivated() {
    auto *gameWin = new ChessGameWindow;
    addNewTab(gameWin, QString("Game %1").arg(tabBar->count()+1));
}
