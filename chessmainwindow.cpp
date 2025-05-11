#include <QFileDialog>

#include "chesstabhost.h"
#include "databaselibrary.h"
#include "chessqsettings.h"
#include "chesstabhost.h"
#include "chessmainwindow.h"

#include <QSqlDatabase>
#include <QSqlQuery>


ChessMainWindow::ChessMainWindow()
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

    QSqlDatabase db = QSqlDatabase::database();

    QSqlQuery query(db);
    query.exec(R"(
            CREATE TABLE IF NOT EXISTS databases (
                id    INTEGER PRIMARY KEY AUTOINCREMENT,
                Event TEXT,
                Site TEXT,
                Date TEXT,
                Round TEXT,
                White TEXT,
                Black TEXT,
                Result TEXT,
                WhiteElo TEXT,
                BlackElo TEXT,
                ECO TEXT,
                PlyCount TEXT,
                SourceVersionDate TEXT
            )
        )");
}


void ChessMainWindow::createMenus() {

    m_menuBar = menuBar();

    QMenu *addNewGame = new QMenu("&Databases");
    QAction* addGameAct = new QAction(tr("&Add New Database"), this);

    addNewGame->addAction(addGameAct);

    connect(addGameAct, &QAction::triggered, this, &ChessMainWindow::onAddGame);

    QMenu *settings = new QMenu("&Settings");
    QAction* selectEngineFile = new QAction(tr("&Select Engine File"), this);
    settings->addAction(selectEngineFile);

    connect(selectEngineFile, &QAction::triggered, this, &ChessMainWindow::onSelectEngineFile);


    m_menuBar->addMenu(addNewGame);
    m_menuBar->addMenu(settings);

    setMenuBar(m_menuBar);
}

void ChessMainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    setMinimumSize(0,0);
}

void ChessMainWindow::onAddGame()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Select a chess PGN file", "", "PGN files (*.pgn)");

    m_dbLibrary->AddNewGame(file_name);
}


void ChessMainWindow::onSelectEngineFile()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Select a chess engine file");

    ChessQSettings * m_settings = new ChessQSettings();

    m_settings->setEngineFile(file_name);
    m_settings->saveSettings();
}

void ChessMainWindow::setStatusBarText(const QString &text)
{
    statusBar()->showMessage(text);
}
