/*
Chess Tab Host
Reusable widget that allows for embeds within a custom tab and user event bar.
History:
March 18, 2025 - Program Creation
*/

#include "chesstabhost.h"
#include "chessgamewindow.h"
#include "databaseviewer.h"
#include "databaselibrary.h"

#include <QPushButton>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QPainter>
#include <QRect>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#include <windowsx.h>
#endif

//initializes custom tab bar
CustomTabBar::CustomTabBar(int defaultWidth, QWidget* parent)
    : QTabBar(parent)
    , defaultWidth(defaultWidth)
{
    setUsesScrollButtons(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); 
    setTabsClosable(false);
    setMovable(true);
    setExpanding(false);
    setDocumentMode(true);

    QTabBar::setDrawBase(false);


    //fix these colours lol
    setStyleSheet(
        "QTabBar {"
        "    border: none;"
        "    margin: 0px;"           
        "    padding: 0px;"  
        "}"
        "QTabBar::tab {"
        "    background-color: #adaeb0;" //hcc
        "    border-top-left-radius: 8px;"
        "    border-top-right-radius: 8px;"
        "    margin-right: 2px;"
        "    margin-top: 20px;"
        "    min-height: 30px;"
        "    color: #5f6368;" //hcc
        "}"
        "QTabBar::tab:selected {"
        "    background-color: palette(window);"  
        "    color: #202124;" //hcc
        "    border: 1px solid #343536;" //hcc
        "    border-bottom: none;"        
        "}"
        "QTabBar::tab:hover:!selected {"
        "    background-color: #d2d4d5;" //hcc
        "}"
    );
}

void CustomTabBar::addCloseButton(int index) {
    // container to position close button
    QWidget* container = new QWidget(this);
    container->setFixedSize(30, 30); 
    
    QPushButton* closeButton = new QPushButton(container);
    closeButton->setIcon(QIcon(":/resource/img/close.png"));
    closeButton->setIconSize(QSize(12, 12));
    closeButton->setFixedSize(12, 12);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;" //hcc
        "}"
        "QPushButton:pressed {"
        "    background-color: #d0d0d0;" //hcc
        "}"
    );
    
    closeButton->move(9, 20);
    
    // find current index when clicked
    connect(closeButton, &QPushButton::clicked, [this, closeButton]() {
        for (int i = 0; i < count(); ++i) {
            QWidget* tabButton = this->tabButton(i, QTabBar::RightSide);
            if (tabButton && tabButton->findChild<QPushButton*>() == closeButton) {
                emit tabCloseRequested(i);
                break;
            }
        }
    });
    
    setTabButton(index, QTabBar::RightSide, container);
}

void CustomTabBar::removeCloseButton(int index) {
    // QWidget* button = tabButton(index, QTabBar::RightSide);
    // if (button) {
    //     setTabButton(index, QTabBar::RightSide, nullptr);
    //     button->deleteLater();
    // }
}

void CustomTabBar::tabInserted(int index) {
    QTabBar::tabInserted(index);
    addCloseButton(index);
}

void CustomTabBar::tabRemoved(int index) {
    QTabBar::tabRemoved(index);
    removeCloseButton(index);
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

//if not windows (windwos uses defualt logic)
#ifndef Q_OS_WIN
void CustomTabBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int tabIndex = tabAt(event->pos());
        
        if (tabIndex < 0) {
            dragStartPos = event->pos();
            isDraggingWindow = false; 
            event->accept();
            return;
        }
    }
    
    dragStartPos = QPoint(-1, -1);
    isDraggingWindow = false;
    
    QTabBar::mousePressEvent(event);
}

void CustomTabBar::mouseMoveEvent(QMouseEvent* event) {
    if (!isDraggingWindow && dragStartPos.x() >= 0) {
        QPoint diff = event->pos() - dragStartPos;
        //15 px jut like title bar
        if(diff.manhattanLength() >= 15) { 
            if(window()->isMaximized()) {
                QPoint globalClickPos = mapToGlobal(dragStartPos);
                window()->showNormal();
                
                QRect restoredGeometry = window()->geometry();
                dragStartPos = QPoint(globalClickPos.x() - restoredGeometry.x(), globalClickPos.y() - restoredGeometry.y());
                
                window()->move(globalClickPos.x() - dragStartPos.x(), globalClickPos.y() - dragStartPos.y());
            } else {
                dragStartPos = event->pos();
            }
            isDraggingWindow = true;
        }
    }
    
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
    dragStartPos = QPoint(-1, -1); 
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

#endif

//initalizes container for custom tab bar
CustomTitleBar::CustomTitleBar(QWidget* parent)
    : QWidget(parent)
{
    // Set properties
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMaximumHeight(50);
    setMinimumHeight(50);  

    
    QHBoxLayout* horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);  
    horizontalLayout->setSpacing(0);   

    horizontalLayout->addSpacing(10);

    tabBar = new CustomTabBar(300, this);
    horizontalLayout->addWidget(tabBar);
    
    minimizeButton = new QPushButton(this);
    maximizeButton = new QPushButton(this);
    closeButton = new QPushButton(this);

    minimizeButton->setIcon(QIcon(":/resource/img/minimize.png"));
    maximizeButton->setIcon(QIcon(":/resource/img/maximize.png"));
    closeButton->setIcon(QIcon(":/resource/img/close.png"));

    minimizeButton->setIconSize(QSize(16, 16));
    maximizeButton->setIconSize(QSize(16, 16));
    closeButton->setIconSize(QSize(16, 16));

    QString buttonStyle = 
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    min-width: 46px;"           
        "    max-width: 46px;"
        "    min-height: 50px;"          
        "    max-height: 50px;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;" //hcc
        "}"
        "QPushButton:pressed {"
        "    background-color: #d0d0d0;" //hcc
        "}";

    //red styling for close
    QString closeButtonStyle = 
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    min-width: 46px;"
        "    max-width: 46px;"
        "    min-height: 50px;"
        "    max-height: 50px;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e81123;" //hcc
        "    color: white;" //hcc
        "}"
        "QPushButton:pressed {"
        "    background-color: #c50e1f;" //hcc
        "}";

    minimizeButton->setStyleSheet(buttonStyle);
    maximizeButton->setStyleSheet(buttonStyle);
    closeButton->setStyleSheet(closeButtonStyle);

    minimizeButton->setFixedSize(46, 50);
    maximizeButton->setFixedSize(46, 50);
    closeButton->setFixedSize(46, 50);

    horizontalLayout->addWidget(minimizeButton);
    horizontalLayout->addWidget(maximizeButton);
    horizontalLayout->addWidget(closeButton);

    connect(minimizeButton, &QPushButton::clicked, this, &CustomTitleBar::MinimizeWindow);
    connect(maximizeButton, &QPushButton::clicked, this, &CustomTitleBar::MaximizeWindow);
    connect(closeButton, &QPushButton::clicked, this, &CustomTitleBar::CloseWindow);

    isMoving = false;
}

CustomTitleBar::~CustomTitleBar(){
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

//if not windows
#ifndef Q_OS_WIN
//handles window adjustment
void CustomTitleBar::mousePressEvent(QMouseEvent* event){
    clickPos = event->pos();
    isMoving = false; 
}

void CustomTitleBar::mouseMoveEvent(QMouseEvent* event){
    if(!isMoving) {
        QPoint diff = event->pos() - clickPos;
        //15px threshhold to start moving
        if(diff.manhattanLength() >= 15) { 
            startDrag(clickPos);  
        }
    }
    
    if(isMoving){
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
#endif

void CustomTitleBar::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    
    // QPainter painter(this);
    
    // painter.setPen(QPen(QColor("#000000"), 1)); //hcc
    // painter.drawLine(0, height() - 1, width(), height() - 1);
}

void CustomTitleBar::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
}




//initializes tab host window
ChessTabHost::ChessTabHost(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(1024,768);
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);

    #ifdef Q_OS_WIN
    setAttribute(Qt::WA_NativeWindow);
    #endif
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);


    CustomTitleBar* titleBar = new CustomTitleBar(this);

    tabBar = titleBar->tabBar;
    connect(tabBar, &QTabBar::currentChanged, this, &ChessTabHost::onTabChanged);
    connect(tabBar, &QTabBar::tabCloseRequested, this, &ChessTabHost::onTabCloseRequested);
    connect(tabBar, &QTabBar::tabMoved, this, &ChessTabHost::onTabMoved);

    // addTabButton = titleBar->addTabButton;
    // connect(addTabButton, &QToolButton::clicked, this, &ChessTabHost::onAddTabClicked);

    stack = new QStackedWidget(this);

    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(stack);
}

#ifdef Q_OS_WIN
bool ChessTabHost::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        
        if (msg->message == WM_NCHITTEST) {
            QPoint cursorPos = mapFromGlobal(QCursor::pos());
            
            // check if cursor is in the title bar area (top 50px)
            if (cursorPos.y() >= 0 && cursorPos.y() <= 50) {
                // get the CustomTitleBar widget to find the tab bar area
                CustomTitleBar* titleBar = findChild<CustomTitleBar*>();
                if (titleBar && titleBar->tabBar) {
                    // convert cursor position to tab bar coordinates
                    QPoint tabBarPos = titleBar->tabBar->mapFromGlobal(QCursor::pos());
                    
                    // check if cursor is over a tab
                    if (titleBar->tabBar->rect().contains(tabBarPos)) {
                        int tabIndex = titleBar->tabBar->tabAt(tabBarPos);
                        if (tabIndex >= 0) {
                            return false;
                        }
                    }
                }
                
                // check if not over window control buttons (right edge buttons are 138px wide: 46*3)
                if (cursorPos.x() < width() - 138) {
                    *result = HTCAPTION; 
                    return true;
                }
            }
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}
#endif

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

//handles new tab depending on embed type **rn not allowed
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
    //not exist rn
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
