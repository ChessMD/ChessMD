#include "chesstabhost.h"
#include "ui_customtitlebar.h"
#include "chessgamewindow.h"
#include "databaseviewer.h"
#include "databaselibrary.h"

#include <qDebug>
#include <QPushButton>
#include <QMouseEvent>


CustomTabBar::CustomTabBar(int defaultWidth, QWidget* parent)
    : QTabBar(parent)
    , defaultWidth(defaultWidth)
{
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

    int tabTotal = n*defaultWidth;

    int finalWidth;
    if(tabTotal <= w) finalWidth = defaultWidth;
    else finalWidth = w/n;

    defaultSize.setWidth(finalWidth);

    return defaultSize;
}

CustomTitleBar::CustomTitleBar(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CustomTitleBar)
{
    ui->setupUi(this);

    tabBar = new CustomTabBar(300, this);

    addTabButton = new QToolButton(this);
    addTabButton->setText("+");
    addTabButton->setToolTip("New Tab");

    ui->topBarLayout->addWidget(tabBar);
    ui->topBarLayout->addWidget(addTabButton);


    connect(ui->minimizeButton, &QPushButton::clicked, this, &CustomTitleBar::MinimizeWindow);
    connect(ui->maximizeButton, &QPushButton::clicked, this, &CustomTitleBar::MaximizeWindow);
    connect(ui->closeButton, &QPushButton::clicked, this, &CustomTitleBar::CloseWindow);
    isMoving = false;

}

CustomTitleBar::~CustomTitleBar(){
    delete ui;
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


    if(DatabaseLibrary* dbLibrary = qobject_cast<DatabaseLibrary*>(embed)){
        connect(dbLibrary, &DatabaseLibrary::fileDoubleClicked, this, &ChessTabHost::onTabReplaced);
        tabTitle = QString("New Tab");
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

    if(tabBar->count() == 0){
        this->close();
        return;
    }

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
    int closeIndex = tabBar->currentIndex();
    addNewTab(new DatabaseViewer, fileIdentifier);
    onTabCloseRequested(closeIndex);
}




