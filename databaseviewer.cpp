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

#include <fstream>
#include <vector>
#include <QResizeEvent>
#include <QFile>
#include <QMenu>

// Initializes the DatabaseViewer
DatabaseViewer::DatabaseViewer(QString filePath, QWidget *parent)
    : QWidget(parent)
    , dbView(new QTableView(this))
    , ui(new Ui::DatabaseViewer)
    , host(new ChessTabHost)
    , m_filePath(filePath)
{
    // connect to ui and initialization
    ui->setupUi(this);

    ui->ContentLayout->insertWidget(0, dbView);

    ui->gamePreview->setMaximumWidth(800);

    dbView->setItemDelegate(new TableDelegate(this));
    dbView->setStyleSheet(getStyle("styles/tablestyle.qss"));
    dbView->verticalHeader()->setVisible(false);
    dbView->setShowGrid(false);
    dbView->setMinimumWidth(500);

    dbView->setSelectionBehavior(QAbstractItemView::SelectRows);
    dbView->setSelectionMode(QAbstractItemView::SingleSelection);
    dbView->setContextMenuPolicy(Qt::CustomContextMenu);


    dbModel = new DatabaseViewerModel(this);
    proxyModel = new DatabaseFilterProxyModel(parent);
    proxyModel->setSourceModel(dbModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    dbView->setModel(proxyModel);
    dbView->setSortingEnabled(true);
    proxyModel->sort(0, Qt::AscendingOrder);

    // signals and slots
    connect(ui->FilterButton, &QPushButton::released, this, &DatabaseViewer::filter);
    connect(ui->AddGameButton, &QPushButton::released, this, &DatabaseViewer::addGame);
    connect(ui->ContentLayout, &QSplitter::splitterMoved, this, &DatabaseViewer::resizeTable);
    connect(dbView, &QAbstractItemView::doubleClicked, this, &DatabaseViewer::onDoubleSelected);
    connect(dbView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &DatabaseViewer::onSingleSelected);
    connect(dbView, &QWidget::customContextMenuRequested, this, &DatabaseViewer::onContextMenu);

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


QString findTag(const QVector<QPair<QString,QString>>& hdr, const QString& tag, const QString& notFound = QStringLiteral("?"))
{
    for (auto &kv : hdr) {
        if (kv.first == tag) return kv.second;
    }
    return notFound;
}

void DatabaseViewer::addGame(){
    PGNGame game; int row = dbModel->rowCount();
    game.dbIndex = row;
    game.headerInfo.push_back({QString("Number"), QString::number(row+1)});
    dbModel->insertRows(row, 1);
    dbModel->addGame(game);
    static const QMap<QString,int> tagToCol = {
        {"Number",   0 },
        {"White",    1 },
        {"WhiteElo", 2 },
        {"Black",    3 },
        {"BlackElo", 4 },
        {"Result",   5 },
        {"Moves",    6 },
        {"Event",    7 },
        {"Date",     8 }
    };

    for (auto it = tagToCol.constBegin(); it != tagToCol.constEnd(); ++it) {
        const QString &tag = it.key();
        int column = it.value();
        QString value = findTag(game.headerInfo, tag, "");
        QModelIndex idx = dbModel->index(row, column);
        dbModel->setData(idx, value);
    }

    QModelIndex sourceIndex = dbModel->index(row, 0);
    QModelIndex proxyIndex = proxyModel->mapFromSource(sourceIndex);
    onDoubleSelected(proxyIndex);
}

// Adds game to database given PGN
void DatabaseViewer::importPGN()
{
    std::ifstream file(m_filePath.toStdString());
    if(file.fail()) return;

    // parse PGN and get headers
    StreamParser parser(file);

    std::vector<PGNGame> database = parser.parseDatabase();
    static const QMap<QString,int> tagToCol = {
        {"Number",   0 },
        {"White",    1 },
        {"WhiteElo", 2 },
        {"Black",    3 },
        {"BlackElo", 4 },
        {"Result",   5 },
        {"Moves",    6 },
        {"Event",    7 },
        {"Date",     8 }
    };

    // iterate through parsed pgn
    for(auto &game: database){
        if(game.headerInfo.size() > 0){
            // add to model
            int row = dbModel->rowCount();
            game.dbIndex = row;
            dbModel->insertRow(row);
            dbModel->addGame(game);
            for (auto it = tagToCol.constBegin(); it != tagToCol.constEnd(); ++it) {
                const QString &tag = it.key();
                int column = it.value();
                QString value = findTag(game.headerInfo, tag, "");
                QModelIndex idx = dbModel->index(row, column);
                dbModel->setData(idx, value);
            }
            dbModel->setData(dbModel->index(row, 0), row+1);
        } else {
            qDebug() << "Error: no game found!";
        }
    }
}

void DatabaseViewer::exportPGN()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Cannot write PGN"), tr("Unable to open file \"%1\" for writing.").arg(m_filePath));
        return;
    }

    QTextStream out(&file);
    for (int i = 0; i < dbModel->rowCount(); i++){
        PGNGame &dbGame = dbModel->getGame(i);
        QString PGNtext = dbGame.serializePGN();
        out << PGNtext << "\n\n";
    }

    file.close();
}

void DatabaseViewer::onPGNGameUpdated(PGNGame &game)
{
    if (game.dbIndex < 0 || game.dbIndex >= dbModel->rowCount()) {
        qDebug() << "Error: invalid dbIndex";
        return;
    }

    PGNGame &dbGame = dbModel->getGame(game.dbIndex);
    dbGame.bodyText = game.bodyText;
    dbGame.headerInfo = game.headerInfo;
    dbGame.rootMove = game.rootMove;

    if (m_embed){
        NotationViewer* notationViewer = m_embed->getNotationViewer();
        if (notationViewer->m_game.dbIndex == game.dbIndex){
            notationViewer->m_game.copyFrom(game);
            notationViewer->setRootMove(game.rootMove);
        }
    }

    for (int i = 0; i < game.headerInfo.size(); ++i) {
        int col = DATA_ORDER[i];
        if (col < 0) continue;
        const auto &kv = game.headerInfo[i];
        QModelIndex idx = dbModel->index(game.dbIndex, col);
        dbModel->setData(idx, kv.second, Qt::EditRole);
    }

    QModelIndex top = dbModel->index(game.dbIndex, 0);
    QModelIndex bot = dbModel->index(game.dbIndex, dbModel->columnCount() - 1);
    emit dbModel->dataChanged(top, bot);

    exportPGN();
}

// Handle game opened in table
void DatabaseViewer::onDoubleSelected(const QModelIndex &proxyIndex) {
    dbView->setFocus();
    if (!proxyIndex.isValid())
        return;

    // init game window requirements
    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    int row = sourceIndex.row();
    PGNGame &dbGame = dbModel->getGame(row);
    PGNGame game;
    // copy game to allow user to make temporary changes
    game.copyFrom(dbGame);
    QString event = findTag(game.headerInfo, QStringLiteral("Event"));
    QString white = findTag(game.headerInfo, QStringLiteral("White"));
    QString black = findTag(game.headerInfo, QStringLiteral("Black"));
    QString title = QString("%1,  \"%2\" vs \"%3\"").arg(event, white, black);

    if(!host->tabExists(title)){
        // create new tab for game
        ChessGameWindow *gameWindow = new ChessGameWindow(this, game);
        connect(gameWindow, &ChessGameWindow::PGNGameUpdated, this, &DatabaseViewer::onPGNGameUpdated);
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

void DatabaseViewer::onContextMenu(const QPoint &pos)
{
    QModelIndex proxyIndex = dbView->indexAt(pos);
    if (!proxyIndex.isValid()) return;

    QMenu menu(this);
    QAction *del = menu.addAction(tr("Delete Game"));
    QAction *act = menu.exec(dbView->viewport()->mapToGlobal(pos));
    if (act == del) {
        QModelIndex srcIdx = proxyModel->mapToSource(proxyIndex);
        int row = srcIdx.row();
        if (dbModel->removeGame(row, QModelIndex())) {
            // update dbView selection
            proxyModel->invalidate();
            dbView->clearSelection();

            for (int i = row; i < dbModel->rowCount(); i++){
                PGNGame &dbGame = dbModel->getGame(i);
                dbGame.dbIndex--;
                for (auto &[tag, value]: dbGame.headerInfo){
                    if (tag == "Number"){
                        value = QString::number(value.toInt()+1);
                    }
                }
                QModelIndex idx = dbModel->index(i, 0);
                dbModel->setData(idx, i+1);
            }
        }

        exportPGN();
    }
}

// Handles game preview
void DatabaseViewer::onSingleSelected(const QModelIndex &proxyIndex, const QModelIndex &previous)
{
    if (!proxyIndex.isValid())
        return;

    // get the game information of the selected row
    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    int row = sourceIndex.row();
    PGNGame &dbGame = dbModel->getGame(row);
    if (!dbGame.isParsed){
        parseBodyText(dbGame.bodyText, dbGame.rootMove);
        dbGame.isParsed = true;
    }

    // copy game to allow user to make temporary changes
    PGNGame game;
    game.copyFrom(dbGame);

    // build the notation tree from the game and construct a ChessGameWindow preview
    m_embed = new ChessGameWindow(this, game);
    m_embed->previewSetup();
    m_embed->setFocusPolicy(Qt::StrongFocus);

    // put embed inside gamePreview
    ui->gamePreview->hide();
    if (ui->gamePreview->layout()) {
        clearPreview(ui->gamePreview); // clear old preview
    }
    QLayout* containerLayout = new QVBoxLayout(ui->gamePreview);

    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_embed);
    ui->gamePreview->setLayout(containerLayout);
    ui->gamePreview->show();
}

void DatabaseViewer::setWindowTitle(QString text)
{
    host->setWindowTitle(text);
}
