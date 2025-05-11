#include "databaseviewer.h"
#include "ui_databaseviewer.h"

#include "tabledelegate.h"
#include "helpers.h"
#include "databasefilter.h"
#include "chessgamewindow.h"
#include "chessposition.h"
#include "chesstabhost.h"


#include <fstream>
#include <vector>
#include <QResizeEvent>


DatabaseViewer::DatabaseViewer(QWidget *parent)
    : QWidget(parent)
    , dbView(new QTableView(this))
    , ui(new Ui::DatabaseViewer)
    , host(new ChessTabHost)
{
    ui->setupUi(this);
    connect(ui->FilterButton, &QPushButton::released, this, &DatabaseViewer::filter);
    connect(ui->ContentLayout, &QSplitter::splitterMoved, this, &DatabaseViewer::resizeTable);

    dbView->setSelectionBehavior(QAbstractItemView::SelectRows);
    dbView->setSelectionMode(QAbstractItemView::SingleSelection);

    dbModel = new DatabaseViewerModel(this);
    proxyModel = new DatabaseFilterProxyModel(parent);
    proxyModel->setSourceModel(dbModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);


    dbView->setModel(proxyModel);
    dbView->setSortingEnabled(true);

    // Signal when a row is double clicked and open a board
    connect(dbView, &QAbstractItemView::doubleClicked, this, &DatabaseViewer::onTableActivated);
    // Signal when row is selected but not double clicked
    connect(dbView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &DatabaseViewer::onTableSelected);

    ui->ContentLayout->insertWidget(0, dbView);

    ui->gamePreview->setMaximumWidth(800);

    dbView->setItemDelegate(new TableDelegate(this));
    dbView->setStyleSheet(getStyle("styles/tablestyle.qss"));
    dbView->verticalHeader()->setVisible(false);
    dbView->setShowGrid(false);


    // Build the notation tree just like in onTableActivated:
    QSharedPointer<NotationMove> rootMove(new NotationMove("", *new ChessPosition));
    rootMove->m_position->setBoardData(convertFenToBoardData(rootMove->FEN));

    // Create your ChessGameWindow
    ChessGameWindow *embed = new ChessGameWindow(this, rootMove);
    embed->previewSetup();

    // Clear out any previous preview
    QWidget* preview = ui->gamePreview;
    preview->hide();

    // Put embed inside gamePreview
    auto* containerLayout = new QVBoxLayout(preview);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(embed);
    preview->setLayout(containerLayout);
    preview->show();
}

DatabaseViewer::~DatabaseViewer()
{
    delete ui;
}

void DatabaseViewer::resizeEvent(QResizeEvent *event){
    resizeTable();
}

void DatabaseViewer::showEvent(QShowEvent *event) {
    //force render on show
    QWidget::showEvent(event);
    resizeTable();
}

void DatabaseViewer::resizeTable(){
    const float widths[9] = {0.1, 0.15, 0.1, 0.15, 0.1, 0.1, 0.05, 0.15, 0.1};
    QList window_width = this->ui->ContentLayout->sizes();

    for(int i = 0; i < 9; i++){
        dbView->setColumnWidth(i, window_width.front()*widths[i]);
    }
}


void DatabaseViewer::addGame(QString file_name)
{
    std::ifstream file(file_name.toStdString());
    if(file.fail()) return;

    // Parse PGN and get headers
    StreamParser parser(file);
    std::vector<PGNGameData> database = parser.parseDatabase();

    for(auto &game: database){
        if(game.headerInfo.size() > 0){
            int row = dbModel->rowCount();

            dbModel->insertRow(row);
            dbModel->addGame(game);
            for(int h = 0; h < game.headerInfo.size(); h++){
                if(DATA_ORDER[h] > -1){
                    QModelIndex index = dbModel->index(row, DATA_ORDER[h]);
                    dbModel->setData(index, game.headerInfo[h].second);
                }
            }
        } else {
            qDebug() << "Error: no game found!";
        }
    }
}

void DatabaseViewer::filter(){

    DatabaseFilter filterWindow(this);

    if(filterWindow.exec() == QDialog::Accepted){
        auto filters = filterWindow.getNameFilters();
        proxyModel->setTextFilter("Black", QString("^(?=.*%1)(?=.*%2).*").arg(filters.blackFirst, filters.blackLast));
        proxyModel->setTextFilter("White", QString("^(?=.*%1)(?=.*%2).*").arg(filters.whiteFirst, filters.whiteLast));
        proxyModel->setRangeFilter("Elo", filters.eloMin, filters.eloMax);
    }
}

void DatabaseViewer::onTableActivated(const QModelIndex &proxyIndex) {
    if (!proxyIndex.isValid())
        return;



    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    int row = sourceIndex.row();
    const PGNGameData& game = dbModel->getGame(row);
    QString title = QString("%1,  \"%2\" vs \"%3\"").arg(game.headerInfo[6].second, game.headerInfo[4].second, game.headerInfo[5].second);


    if(!host->tabExists(title)){
        QSharedPointer<NotationMove> rootMove(new NotationMove("", *new ChessPosition));
        rootMove->m_position->setBoardData( convertFenToBoardData(rootMove->FEN));
        buildNotationTree(game.getRootVariation(), rootMove);

        ChessGameWindow *gameWin = new ChessGameWindow(this, rootMove);
        gameWin->engineSetup();
        gameWin->toolbarSetup();


        host->addNewTab(gameWin, title);
    }
    else{
        host->activateTabByLabel(title);
    }


    //set focus to new window
    //source: https://stackoverflow.com/questions/6087887/bring-window-to-front-raise-show-activatewindow-don-t-work
    host->setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive | Qt::WindowMaximized);
    host->raise();
    host->activateWindow(); // for Windows
    host->show();
}

void DatabaseViewer::onTableSelected(const QModelIndex &proxyIndex, const QModelIndex &previous)
{
    if (!proxyIndex.isValid())
        return;

    // Map to your source model
    QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
    int row = sourceIndex.row();
    const PGNGameData& game = dbModel->getGame(row);

    // Build the notation tree just like in onTableActivated:
    QSharedPointer<NotationMove> rootMove(new NotationMove("", *new ChessPosition));
    rootMove->m_position->setBoardData(convertFenToBoardData(rootMove->FEN));
    buildNotationTree(game.getRootVariation(), rootMove);

    // Create your ChessGameWindow
    ChessGameWindow *embed = new ChessGameWindow(this, rootMove);
    embed->previewSetup();

    // Clear out any previous preview
    QWidget* preview = ui->gamePreview;
    preview->hide();
    QLayout* oldLayout = preview->layout();
    if (oldLayout) {
        // delete old widgets/layout
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete oldLayout;
    }

    // Put embed inside gamePreview
    auto* containerLayout = new QVBoxLayout(preview);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(embed);
    preview->setLayout(containerLayout);
    preview->show();
}

void DatabaseViewer::setWindowTitle(QString text)
{
    host->setWindowTitle(text);
}
