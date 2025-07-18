#include <QFileDialog>
#include <QOperatingSystemVersion>

#include "chesstabhost.h"
#include "databaselibrary.h"
#include "chessqsettings.h"

#include "mainwindow.h"
#include "settingsdialog.h"

#include <QToolBar>
#include <QAction>

MainWindow::MainWindow()
{

    m_dbLibrary = new DatabaseLibrary(this);

    setCentralWidget(m_dbLibrary);
    setupSidebar();
    setMinimumSize(800,600);
}


void MainWindow::setupSidebar() {
    QToolBar* sidebar = new QToolBar(this);
    sidebar->setOrientation(Qt::Vertical);
    sidebar->setMovable(false);
    sidebar->setFloatable(false);
    sidebar->setIconSize(QSize(32, 32));
    sidebar->setFixedWidth(48);
    
    sidebar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);


    //center icons
    sidebar->setStyleSheet(R"(
        QToolBar {
            padding: 0px;
            margin: 0px;
        }
        QToolButton {
            padding: 8px;
        }
    )");

    QWidget* topSpacer = new QWidget(this);
    topSpacer->setFixedHeight(5);  
    sidebar->addWidget(topSpacer);

    // engine 
    QMenu* engineMenu = new QMenu(this);
    QAction* selectEngineFile = new QAction(tr("Select Engine File"), this);
    engineMenu->addAction(selectEngineFile);
    connect(selectEngineFile, &QAction::triggered, this, &MainWindow::onSelectEngineFile);
    QToolButton* engineButton = new QToolButton(this);
    engineButton->setIcon(QIcon(":/resource/img/engine.png"));
    engineButton->setToolTip(tr("Engine"));
    engineButton->setMenu(engineMenu);
    engineButton->setPopupMode(QToolButton::InstantPopup);
    engineButton->setIconSize(QSize(32, 32));
    engineButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    sidebar->addWidget(engineButton);

    //spacer to put settings at bottom
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    spacer->setMinimumHeight(10); 
    sidebar->addWidget(spacer);

    // settings 
    QMenu* settingsMenu = new QMenu(this);
    QAction* settingsAct = new QAction(tr("Settings"), this);
    settingsMenu->addAction(settingsAct);
    connect(settingsAct, &QAction::triggered, this, &MainWindow::onSettings);
    QToolButton* settingsButton = new QToolButton(this);
    settingsButton->setIcon(QIcon(":/resource/img/settings.png"));
    settingsButton->setToolTip(tr("Settings"));
    settingsButton->setMenu(settingsMenu);
    settingsButton->setPopupMode(QToolButton::InstantPopup);
    settingsButton->setIconSize(QSize(32, 32));
    settingsButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    sidebar->addWidget(settingsButton);

    // bottom spacer
    QWidget* bottomSpacer = new QWidget(this);
    bottomSpacer->setFixedHeight(5);
    sidebar->addWidget(bottomSpacer);

    // add sidebar to the left
    addToolBar(Qt::LeftToolBarArea, sidebar);
}

void MainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    setMinimumSize(0,0);
}

void MainWindow::onAddGame()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Select a chess PGN file", "", "PGN files (*.pgn)");

    m_dbLibrary->AddNewGame(file_name);
}


void MainWindow::onSelectEngineFile()
{
    QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();

    QString file_name;

    if (osVersion.type() == QOperatingSystemVersion::Windows)
        file_name =  QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), QString(), tr("(*.exe)"));
    else
        file_name =  QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), QString(), tr("(*)"));

    ChessQSettings * m_settings = new ChessQSettings();

    m_settings->setEngineFile(file_name);
    m_settings->saveSettings();
}

void MainWindow::onSettings(){
    SettingsDialog dlg(this);
    dlg.exec();
}
