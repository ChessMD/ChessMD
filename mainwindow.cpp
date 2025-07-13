#include <QFileDialog>
#include <QOperatingSystemVersion>

#include "chesstabhost.h"
#include "databaselibrary.h"
#include "chessqsettings.h"
#include "mainwindow.h"

MainWindow::MainWindow()
{
    /*
    m_tabWidget = new ChessTabHost(this);

    if (size > 0) {
        for (int i = 0; i < size; i++) {
            DatabaseLibrary *dbLibrary = new DatabaseLibrary;
            m_tabWidget->addNewTab(dbLibrary, tabNameList[i]);
        }
        m_tabWidget->setActiveTab(0);
    } else
        m_tabWidget->addNewTab(new DatabaseLibrary, "");

    m_tabWidget->setWindowState(Qt::WindowMaximized);
    */

    m_dbLibrary = new DatabaseLibrary(this);

    setStatusBar(new QStatusBar);

    setCentralWidget(m_dbLibrary);
    createMenus();

    setMinimumSize(800,600);
}


void MainWindow::createMenus() {

    m_menuBar = menuBar();

    QMenu *addNewGame = new QMenu("&Databases");
    QAction* addGameAct = new QAction(tr("&Add New Database"), this);

    addNewGame->addAction(addGameAct);

    connect(addGameAct, &QAction::triggered, this, &MainWindow::onAddGame);

    QMenu *settings = new QMenu("&Settings");
    QAction* selectEngineFile = new QAction(tr("&Select Engine File"), this);
    settings->addAction(selectEngineFile);

    connect(selectEngineFile, &QAction::triggered, this, &MainWindow::onSelectEngineFile);


    m_menuBar->addMenu(addNewGame);
    m_menuBar->addMenu(settings);

    setMenuBar(m_menuBar);
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


void MainWindow::setStatusBarText(const QString &text)
{
    statusBar()->showMessage(text);
}
