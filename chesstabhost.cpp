/*
March 18, 2025: File Creation
*/

#include "chesstabhost.h"
#include "ui_customtitlebar.h"
#include "chessgamewindow.h"
#include "databaseviewer.h"
#include "databaselibrary.h"


#include <qDebug>
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>




CustomTabBar::CustomTabBar(int defaultWidth, QWidget* parent)
    : QTabBar(parent)
    , defaultWidth(defaultWidth)
{
    setWindowFlags(Qt::FramelessWindowHint);

    setUsesScrollButtons(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setTabsClosable(true);
    setMovable(true);
    setExpanding(false);
}

QSize CustomTabBar::tabSizeHint(int index) const {
    QSize defaultSize = QTabBar::tabSizeHint(index);

    int w = width();
    int n = count();

    int tabTotal = n * defaultWidth;

    int finalWidth;
    if(tabTotal <= w) finalWidth = defaultWidth;
    else finalWidth = w/n;

    defaultSize.setWidth(finalWidth);

    return defaultSize;
}

CustomTitleBar::CustomTitleBar(QWidget* parent)
    : QWidget(parent)
{

    tabBar = new CustomTabBar(300, this);

    addTabButton = new QToolButton(this);
    addTabButton->setText("+");
    addTabButton->setStyleSheet("font-size:18px; background-color: coral; padding: 0px 0px 0px 0px;");
    addTabButton->setToolTip("Create New Holder");

    QHBoxLayout *topBarLayout = new QHBoxLayout(this);

    topBarLayout->addWidget(tabBar);
    topBarLayout->addWidget(addTabButton);

    isMoving = false;

}

CustomTitleBar::~CustomTitleBar(){
}

void CustomTitleBar::MinimizeWindow(){
    window()->showMinimized();
}

void CustomTitleBar::MaximizeWindow(){
    if(!window()->isMaximized()) window()->showMaximized();
    else window()->showNormal();
}

void CustomTitleBar::CloseWindow(){
    window()->close();
}

void CustomTitleBar::mousePressEvent(QMouseEvent* event){
    clickPos= event->pos();
    isMoving= true;
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent* event){
    if(isMoving){
        //calculate difference between the press position and the new Mouse position
        QPoint diff = event->pos() - clickPos;
        window()->move(window()->pos() + diff);
    }
}

void CustomTitleBar::mouseReleaseEvent(QMouseEvent* event){
    isMoving= false;
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent* event){
    MaximizeWindow();
}






ChessTabHost::ChessTabHost(QWidget* parent)
    : QWidget(parent)
{

    m_parent = parent;

    setWindowFlags(Qt::CustomizeWindowHint);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);


    CustomTitleBar* titleBar = new CustomTitleBar(this);

    tabBar = titleBar->tabBar;
    connect(tabBar, &QTabBar::currentChanged, this, &ChessTabHost::onTabChanged);
    connect(tabBar, &QTabBar::tabCloseRequested, this, &ChessTabHost::onTabCloseRequested);
    connect(tabBar, &QTabBar::tabMoved, this, &ChessTabHost::onTabMoved);

    addTabButton = titleBar->addTabButton;
    connect(addTabButton, &QToolButton::clicked, this, &ChessTabHost::onAddTabClicked);

    stack = new QStackedWidget(this);

    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(stack);
}

int ChessTabHost::rowCount(){
    return tabBar->count();
}

void ChessTabHost::addNewTab(QWidget* embed, QString title) {


    QString tabTitle;

    //Could've used derived classes... todo(?)
    if(DatabaseLibrary* dbLibrary = qobject_cast<DatabaseLibrary*>(embed)){
        connect(dbLibrary, &DatabaseLibrary::fileDoubleClicked, this, &ChessTabHost::onTabReplaced);
        tabTitle = title;
        if (tabTitle.isEmpty()) {
            tabTitle = QString("Game Holder %1").arg(tabBar->count() + 1);
            if (isTabTitleExist(tabTitle)) {
                for (int i = 0; i < tabBar->count(); i++) {
                    tabTitle = QString("Game Holder %1").arg(i + 1);
                    if (isTabTitleExist(tabTitle) == false)
                        break;
                }
            }
        }
        QSqlDatabase db = QSqlDatabase::database();

        QSqlQuery query(db);
        query.exec(R"(
            CREATE TABLE IF NOT EXISTS databases (
                id    INTEGER PRIMARY KEY AUTOINCREMENT,
                event TEXT,
                site TEXT,
                date TEXT,
                round TEXT,
                white TEXT,
                black TEXT,
                result TEXT,
                whiteElo TEXT,
                blackElo TEXT
            )
        )");

    }
    else if(ChessGameWindow* gameWindow = qobject_cast<ChessGameWindow*>(embed)){
        tabTitle = QString("Game %1").arg(tabBar->count() + 1);
    }
    else if (DatabaseViewer* dbViewer =  qobject_cast<DatabaseViewer*>(embed)){
        tabTitle = QString("Database %1").arg(tabBar->count() + 1);
    }

    QWidget* container = new QWidget;
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addWidget(embed);
    container->setLayout(containerLayout);

    int index = stack->addWidget(container);
    tabBar->addTab(tabTitle);
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

    if(tabBar->count() == 0){
     //   this->close();
        return;
    }

    // Update the current index after removal.
    int currentIndex = tabBar->currentIndex();
    stack->setCurrentIndex(currentIndex);
}

void ChessTabHost::onAddTabClicked() {
    addNewTab(new DatabaseLibrary, "");
}

void ChessTabHost::onTabMoved(int from, int to) {
    QWidget* widget = stack->widget(from);
    stack->removeWidget(widget);
    stack->insertWidget(to, widget);
}

void ChessTabHost::onTabReplaced(const QString &fileIdentifier)
{

    ((ChessMainWindow *) m_parent)->setStatusBarText("Loading ...");
    QApplication::processEvents(); // force the event loop to process all pending events, including the update to the status bar.

    int closeIndex = tabBar->currentIndex();

    DatabaseViewer * dbViewer = new DatabaseViewer;

    dbViewer->addGame(fileIdentifier);

    addNewTab(dbViewer, fileIdentifier);
    onTabCloseRequested(closeIndex);

    ((ChessMainWindow *) m_parent)->setStatusBarText("");
    QApplication::processEvents(); // force the event loop to process all pending events, including the update to the status bar.

}

void ChessTabHost::setActiveTab(int index)
{
    tabBar->setCurrentIndex(index);
}

bool ChessTabHost::isTabTitleExist(QString title)
{
    for (int i = 0; i < tabBar->count(); i++) {
        if (tabBar->tabText(i) == title)
            return true;
    }

    return false;
}

