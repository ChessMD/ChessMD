/*
Chess Tab Host
Reusable widget that allows for embeds within a custom tab and user event bar.
History:
March 18, 2025 - Program Creation
*/

#include "chesstabhost.h"
#include "ui_customtitlebar.h"
#include "chessgamewindow.h"
#include "databaseviewer.h"
#include "databaselibrary.h"

#include <QDebug>
#include <QPushButton>
#include <QMouseEvent>
#include <QCloseEvent>

//initializes custom tab bar
CustomTabBar::CustomTabBar(int defaultWidth, QWidget* parent)
    : QTabBar(parent)
    , defaultWidth(defaultWidth)
{
    setUsesScrollButtons(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); 
    setTabsClosable(true);
    setMovable(true);
    setExpanding(false);
    setDocumentMode(true);

    setStyleSheet("QTabBar { background-color: #f1f3f4; border: none; }");
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

void CustomTabBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int tabIndex = tabAt(event->pos());
        
        if (tabIndex < 0) {
            if(window()->isMaximized()) {
                QPoint globalClickPos = mapToGlobal(event->pos());
                window()->showNormal();
                
                QRect restoredGeometry = window()->geometry();
                dragStartPos = QPoint(globalClickPos.x() - restoredGeometry.x(), globalClickPos.y() - restoredGeometry.y());
                
                window()->move(globalClickPos.x() - dragStartPos.x(), globalClickPos.y() - dragStartPos.y());
            } else {
                dragStartPos = event->pos();
            }
            
            isDraggingWindow = true;
            event->accept();
            return;
        }
        
    }
    QTabBar::mousePressEvent(event);
}

void CustomTabBar::mouseMoveEvent(QMouseEvent* event) {
    if (isDraggingWindow) {
        QPoint diff = event->pos() - dragStartPos;
        window()->move(window()->pos() + diff);
        event->accept();
        return;
    }
    QTabBar::mouseMoveEvent(event);
}

void CustomTabBar::mouseReleaseEvent(QMouseEvent* event) {
    isDraggingWindow = false;
    QTabBar::mouseReleaseEvent(event);
}

void CustomTabBar::mouseDoubleClickEvent(QMouseEvent* event) {
    int tabIndex = tabAt(event->pos());
    
    if (tabIndex < 0) {
        if (CustomTitleBar* titleBar = qobject_cast<CustomTitleBar*>(parent())) {
            titleBar->MaximizeWindow();
        }
        event->accept();
        return;
    }
    
    QTabBar::mouseDoubleClickEvent(event);
}

//initalizes container for custom tab bar
CustomTitleBar::CustomTitleBar(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CustomTitleBar)
{
    ui->setupUi(this);

    tabBar = new CustomTabBar(300, this);

    addTabButton = new QToolButton(this);
    addTabButton->setText("+");
    addTabButton->setToolTip("New Tab");
    addTabButton->hide();

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

//handle minimize, maxmimize, and close buttons
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

void CustomTitleBar::startDrag(QPoint localPos) {
    if(window()->isMaximized()) {
        QPoint globalClickPos = mapToGlobal(localPos);
        window()->showNormal();
        
        QRect restoredGeometry = window()->geometry();
        clickPos = QPoint(globalClickPos.x() - restoredGeometry.x(),  globalClickPos.y() - restoredGeometry.y());
        
        window()->move(globalClickPos.x() - clickPos.x(), globalClickPos.y() - clickPos.y());
    } else {
        clickPos = localPos;
    }
    isMoving = true;
}

//handles window adjustment
void CustomTitleBar::mousePressEvent(QMouseEvent* event){
    startDrag(event->pos());
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent* event){
    if(isMoving){
        //calculate difference between the press position and the new Mouse position
        QPoint diff = event->pos() - clickPos;
        window()->move(window()->pos() + diff);
    }
}

void CustomTitleBar::mouseReleaseEvent(QMouseEvent* event){
    isMoving = false;
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent* event){
    MaximizeWindow();
}




//initializes tab host window
ChessTabHost::ChessTabHost(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(1024,768);
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
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


void ChessTabHost::closeEvent(QCloseEvent *event)
{
    if (tabBar->count()){
        // request to close the current tab
        int count = tabBar->count();
        onTabCloseRequested(tabBar->currentIndex());
        if (count == tabBar->count()){
            event->ignore();
            return;
        }

        // continue request to closing tabs
        while ((count = tabBar->count()) > 0) {
            onTabCloseRequested(0);
            if (count == tabBar->count()) {
                event->ignore();
                return;
            }
        }
    }
    QWidget::closeEvent(event);
}

int ChessTabHost::rowCount(){
    return tabBar->count();
}

//handles new tab depending on embed type
void ChessTabHost::addNewTab(QWidget* embed, QString title) {


    QString tabTitle;

    //Could've used derived classes... todo(?)
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


//Tab Event Handling
void ChessTabHost::onTabChanged(int index) {
    stack->setCurrentIndex(index);
}

void ChessTabHost::onTabCloseRequested(int index) {
    if (index < 0 || index >= tabBar->count()){
        qDebug() << "onTabCloseRequested: index out of range";
        return;
    }

    QWidget* widget = stack->widget(index);

    QWidget* embed = nullptr;
    if (auto* lay = widget->layout()){
        if (auto* item = lay->itemAt(0))
            embed = item->widget();
    }

    // save dialogue
    if (embed){
        if (auto* gameWin = qobject_cast<ChessGameWindow*>(embed)) {
            gameWin->close();
            if (gameWin->isVisible()) {
                return;
            }
        }
    }

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
    // addNewTab(new DatabaseLibrary, "New Tab");
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
    addNewTab(new DatabaseViewer(fileIdentifier), fileIdentifier);
    onTabCloseRequested(closeIndex);
}

bool ChessTabHost::tabExists(QString label){
    for (int i = 0; i < tabBar->count(); i++) {
        if (tabBar->tabText(i) == label)
            return true;
    }

    return false;
}


void ChessTabHost::activateTabByLabel(QString label)
{
    for (int i = 0; i < tabBar->count(); i++) {
        if (tabBar->tabText(i) == label) {
            tabBar->setCurrentIndex(i);
            break;
        }
    }
}
