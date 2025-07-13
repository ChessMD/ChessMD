/*
Database Viewer
Widget that holds database info in a QTableView
History:
March 18, 2025 - Program Creation
*/

#include "databaseviewer.h"
#include "ui_databaseviewer.h"

#include "tabledelegate.h"
#include "helpers.h"
#include "databasefilter.h"
#include "chessgamewindow.h"
#include "chessposition.h"
#include "chesstabhost.h"
#include "pgngamedata.h"
#include "helpers.h"

#include <fstream>
#include <vector>
#include <QResizeEvent>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>

// Initializes the DatabaseViewer
DatabaseViewer::DatabaseViewer(QWidget *parent)
    : QWidget(parent)
    , dbView(new QTableView(this))
    , ui(new Ui::DatabaseViewer)
    , host(new ChessTabHost)
{
    // connect to ui and initialization
    ui->setupUi(this);

    ui->ContentLayout->insertWidget(0, dbView);

    ui->gamePreview->setMaximumWidth(800);

    dbView->setItemDelegate(new TableDelegate(this));
    dbView->setStyleSheet(getStyle("styles/tablestyle.qss"));
    dbView->verticalHeader()->setVisible(false);
    dbView->setShowGrid(false);

    dbView->setSelectionBehavior(QAbstractItemView::SelectRows);
    dbView->setSelectionMode(QAbstractItemView::SingleSelection);

    dbModel = new DatabaseViewerModel(this);
    proxyModel = new DatabaseFilterProxyModel(parent);
    proxyModel->setSourceModel(dbModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    dbView->setModel(proxyModel);
    dbView->setSortingEnabled(true);

    // signals and slots
    connect(ui->FilterButton, &QPushButton::released, this, &DatabaseViewer::filter);
    connect(ui->ContentLayout, &QSplitter::splitterMoved, this, &DatabaseViewer::resizeTable);
    connect(dbView, &QAbstractItemView::doubleClicked, this, &DatabaseViewer::onDoubleSelected);
    connect(dbView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &DatabaseViewer::onSingleSelected);

    // set preview to a placeholder game (warms-up QML, stopping the window from blinking when a game is previewed)
    QSharedPointer<NotationMove> rootMove(new NotationMove("", *new ChessPosition));
    rootMove->m_position->setBoardData(convertFenToBoardData(rootMove->FEN));
    PGNGame game;
    ChessGameWindow *embed = new ChessGameWindow(this, game);
    embed->previewSetup();
    embed->setFocusPolicy(Qt::StrongFocus);

    // put embed inside gamePreview
    ui->gamePreview->hide();
    QLayout* containerLayout = new QVBoxLayout(ui->gamePreview);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(embed);
    ui->gamePreview->setLayout(containerLayout);
    ui->gamePreview->show();
}

// Destructor
DatabaseViewer::~DatabaseViewer()
{
    delete ui;
}

// Window resize event handler
void DatabaseViewer::resizeEvent(QResizeEvent *event){
    resizeTable();
}

void DatabaseViewer::showEvent(QShowEvent *event) {
    // force render on show
    QWidget::showEvent(event);
    resizeTable();
}

// Custom table resizer
void DatabaseViewer::resizeTable(){
    const float widths[9] = {0.1, 0.15, 0.1, 0.15, 0.1, 0.1, 0.05, 0.15, 0.1};
    QList window_width = this->ui->ContentLayout->sizes();

    for(int i = 0; i < 9; i++){
        dbView->setColumnWidth(i, window_width.front()*widths[i]);
    }
}

QString DatabaseViewer::getDatabasePath(const QString &pgnFilePath){
    return getDatabasePathForPGN(pgnFilePath);
}


// Adds game to database given PGN
void DatabaseViewer::addGame(QString file_name)
{
    // init sql database
    m_databasePath = getDatabasePath(file_name);
    m_connectionName = QFileInfo(file_name).absoluteFilePath();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        qWarning() << "Failed to open database:" << m_databasePath;
        return;
    }


    



    std::ifstream file(file_name.toStdString());
    if(file.fail()) return;

    // parse PGN and get headers
    StreamParser parser(file);
    std::vector<PGNGame> database = parser.parseDatabase();

    

    // requirements for SQL db
    const QSet<QString> requiredKeys = {"Event","Site","Date","Round","White","Black","Result", "WhiteElo", "BlackElo", "ECO"};

    //table
    QSqlQuery q(db);
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS games (
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
            SourceVersionDate TEXT,
            PGNBody TEXT
        )
    )");
    q.exec("BEGIN TRANSACTION");


    // iterate through parsed pgn
    for(auto &game: database){

        if(game.headerInfo.size() > 0){
            // add to model
            int row = dbModel->rowCount();
            dbModel->insertRow(row);
            dbModel->addGame(game);

            // sql table values
            QStringList cols, params;

            //add pgnbody since not part of headers
            cols << "PGNBody";
            QString escapedPGN = game.bodyText;
            escapedPGN.replace("'", "''");
            params << QString("'%1'").arg(escapedPGN);


            for(int h = 0; h < game.headerInfo.size(); h++){

                // handle required SQL db elements
                if(requiredKeys.contains(game.headerInfo[h].first)){
                    cols << game.headerInfo[h].first;
                    QString tS = game.headerInfo[h].second;
                    tS.replace("'", "''");
                    params << QString("'%1'").arg(tS);
                }

                // add to dbModel
                if(DATA_ORDER[h] > -1){
                    QModelIndex index = dbModel->index(row, DATA_ORDER[h]);
                    dbModel->setData(index, game.headerInfo[h].second);
                }
            }

            // insert into SQL database + error handling
            QString sql = QStringLiteral("INSERT INTO games(%1) VALUES(%2)").arg(cols.join(", ")).arg(params.join(", "));
            if (!q.exec(sql)) {
                qWarning() << "Insert failed:" << q.lastError().text();
                return;
            }


        } else {
            qDebug() << "Error: no game found!";
        }
    }

    q.exec("COMMIT");
    

    dbModel->setConnectionName(m_connectionName);
}

void DatabaseViewer::loadExistingDatabase(QString file_name){
    if (file_name.isEmpty()) return;

    m_databasePath = getDatabasePath(file_name);
    m_connectionName = QFileInfo(file_name).absoluteFilePath();

    if (!QFile::exists(m_databasePath)) {
        qWarning() << "Database file does not exist:" << m_databasePath;
        return;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(m_databasePath);
    
    if (!db.open()) {
        qWarning() << "Failed to open existing database:" << m_databasePath;
        return;
    }

    // Set connection name for the model and load data
    dbModel->setConnectionName(m_connectionName);
    dbModel->loadExistingDatabase();
    
    

}

// Handles custom filters
void DatabaseViewer::filter(){

    // init filter window
    DatabaseFilter filterWindow(this);

    // apply filters
    if(filterWindow.exec() == QDialog::Accepted){
        auto filters = filterWindow.getNameFilters();
        proxyModel->setTextFilter("Black", QString("^(?=.*%1)(?=.*%2).*").arg(filters.blackFirst, filters.blackLast));
        proxyModel->setTextFilter("White", QString("^(?=.*%1)(?=.*%2).*").arg(filters.whiteFirst, filters.whiteLast));
        proxyModel->setRangeFilter("Elo", filters.eloMin, filters.eloMax);
    }
}

// Handle game opened in table
void DatabaseViewer::onDoubleSelected(const QModelIndex &proxyIndex) {
    dbView->setFocus();
    if (!proxyIndex.isValid())
        return;

    // init game window requirements
    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    int row = sourceIndex.row();
    const PGNGame& game = dbModel->getGame(row);
    QString title = QString("%1,  \"%2\" vs \"%3\"").arg(game.headerInfo[6].second, game.headerInfo[4].second, game.headerInfo[5].second);

    if(!host->tabExists(title)){
        // create new tab for game
        ChessGameWindow *gameWindow = new ChessGameWindow(this, game);
        gameWindow->mainSetup();

        host->addNewTab(gameWindow, title);
    } else {
        // open existing tab
        host->activateTabByLabel(title);
    }

    // set focus to new window
    // source: https://stackoverflow.com/questions/6087887/bring-window-to-front-raise-show-activatewindow-don-t-work
    host->setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive | Qt::WindowMaximized);
    host->raise();
    host->activateWindow(); // for Windows
    host->show();

}

// Clear existing layouts inside preview
void clearPreview(QWidget* container) {
    QLayout* oldLayout = container->layout();
    if (!oldLayout) return;

    QLayoutItem* item = nullptr;
    while ((item = oldLayout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) {
            oldLayout->removeWidget(w);
            w->deleteLater();
        }
        // if there was a nested layout, clear that too
        if (auto childLayout = item->layout()) {
            delete childLayout;
        }
        delete item;
    }
    delete oldLayout;
}

// Handles game preview
void DatabaseViewer::onSingleSelected(const QModelIndex &proxyIndex, const QModelIndex &previous)
{
    if (!proxyIndex.isValid())
        return;

    // get the game information of the selected row
    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    int row = sourceIndex.row();
    const PGNGame& game = dbModel->getGame(row);

    // build the notation tree from the game and construct a ChessGameWindow preview
    ChessGameWindow *embed = new ChessGameWindow(this, game);
    embed->previewSetup();
    embed->setFocusPolicy(Qt::StrongFocus);

    // put embed inside gamePreview
    ui->gamePreview->hide();
    if (ui->gamePreview->layout()) {
        clearPreview(ui->gamePreview); // clear old preview
    }
    QLayout* containerLayout = new QVBoxLayout(ui->gamePreview);

    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(embed);
    ui->gamePreview->setLayout(containerLayout);
    ui->gamePreview->show();
}

void DatabaseViewer::setWindowTitle(QString text)
{
    host->setWindowTitle(text);
}
