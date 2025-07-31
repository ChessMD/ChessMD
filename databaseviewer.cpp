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
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QSettings>
#include <QLayoutItem>

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
    // dbView->setStyleSheet(getStyle(":/resource/styles/tablestyle.qss"));
    dbView->verticalHeader()->setVisible(false);
    dbView->setShowGrid(false);
    dbView->setMinimumWidth(500);

    dbView->setSelectionBehavior(QAbstractItemView::SelectRows);
    dbView->setSelectionMode(QAbstractItemView::SingleSelection);
    dbView->setContextMenuPolicy(Qt::CustomContextMenu);

    QHeaderView* header = dbView->horizontalHeader();
    header->setContextMenuPolicy(Qt::CustomContextMenu);

    dbModel = new DatabaseViewerModel(this);
    proxyModel = new DatabaseFilterProxyModel(parent);
    proxyModel->setSourceModel(dbModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    dbView->setModel(proxyModel);
    dbView->setSortingEnabled(true);
    proxyModel->sort(0, Qt::AscendingOrder);

    {
    //load header settings    
    QSettings settings;
    settings.beginGroup("DBViewHeaders");
    QStringList allHeaders = settings.value("all").toStringList();
    QStringList shownHeaders = settings.value("shown").toStringList();
    settings.endGroup();

    for(const QString& header: allHeaders){
        dbModel->addHeader(header);
    }

    if(!shownHeaders.isEmpty()){
        for(int i = 0; i < dbModel->columnCount(); i++){
            QString header = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            if(!shownHeaders.contains(header)) dbView->setColumnWidth(i, 0);
        }
    }
    
    }

    // signals and slots
    connect(ui->FilterButton, &QPushButton::released, this, &DatabaseViewer::filter);
    connect(ui->AddGameButton, &QPushButton::released, this, &DatabaseViewer::addGame);
    connect(ui->ContentLayout, &QSplitter::splitterMoved, this, &DatabaseViewer::resizeTable);
    connect(dbView, &QAbstractItemView::doubleClicked, this, &DatabaseViewer::onDoubleSelected);
    connect(dbView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &DatabaseViewer::onSingleSelected);
    connect(dbView, &QWidget::customContextMenuRequested, this, &DatabaseViewer::onContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, this, &DatabaseViewer::onHeaderContextMenu);
    connect(header, &QHeaderView::sectionResized, this, &DatabaseViewer::saveColumnRatios);

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
    // qDebug() << ratios.size();
    // for(auto i: ratios) qDebug() << i;
    // if(ratios.size() != dbModel->columnCount()) qDebug() << "ratios no match";

    float sum = 0.0f;
    for(int i = 0; i < dbModel->columnCount(); i++){
        if(dbView->columnWidth(i) != 0) sum += mRatios[i];
    }

    int totalWidth = dbView->viewport()->width();
    for(int i = 0; i < dbModel->columnCount(); i++){
        if(dbView->columnWidth(i) != 0) dbView->setColumnWidth(i, totalWidth*mRatios[i]/sum);
    }
}

void DatabaseViewer::saveColumnRatios(){
    int cols = dbModel->columnCount();
    int totalWidth = dbView->viewport()->width();
    QVector<float> ratios;
    for(int i = 0; i < cols; i++){
        if(dbView->columnWidth(i) != 0){        
            float ratio = float(dbView->columnWidth(i)) / float(totalWidth);
            ratios.append(ratio);
        }
        else{
            ratios.append(mRatios[i]);
        }
    }
    QSettings settings;
    settings.beginGroup("DbViewHeaders");
    settings.setValue("ratios", QVariant::fromValue(ratios));
    settings.endGroup();

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
    PGNGame game; 
    int row = dbModel->rowCount();
    game.dbIndex = row;
    game.headerInfo.push_back({QString("Number"), QString::number(row+1)});
    dbModel->insertRows(row, 1);
    dbModel->addGame(game);
    

    for (int i = 0; i < dbModel->columnCount(); i++) {
        QString tag = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        QString value = findTag(game.headerInfo, tag, "");
        QModelIndex idx = dbModel->index(row, i);
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

    // iterate through parsed pgn
    for(auto &game: database){
        if(game.headerInfo.size() > 0){
            // add to model
            int row = dbModel->rowCount();
            game.dbIndex = row;
            dbModel->insertRow(row);
            dbModel->addGame(game);

            for (int i = 0; i < dbModel->columnCount(); i++) {
                QString tag = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
                QString value = findTag(game.headerInfo, tag, "");
                QModelIndex idx = dbModel->index(row, i);
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

    for (int i = 0; i < game.headerInfo.size(); i++) {
        const auto &kv = game.headerInfo[i];
        int col = dbModel->headerIndex(kv.first);
        if (col < 0) continue;
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

// Right click table header menu for updating shown headers
void DatabaseViewer::onHeaderContextMenu(const QPoint &pos){
    int col = dbView->horizontalHeader()->logicalIndexAt(pos);
    if(col < 0) return;

    QMenu menu(this);
    QAction* config = menu.addAction(tr("Configure Columns"));

    QAction* selected = menu.exec(dbView->horizontalHeader()->mapToGlobal(pos));
    if(selected == config){
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Configure Columns"));
        QVBoxLayout* layout = new QVBoxLayout(&dialog);

        QVector<QCheckBox*> boxes;

        //lambda to build the checkboxes in proper order
        auto rebuildBoxes = [&](){
            QLayoutItem* child;
            
            //delete everything
            while((child = layout -> takeAt(0)) != nullptr){
                if(child->widget()) delete child->widget();
                delete child;
            }
            boxes.clear();

            //readd in proepr order
            for(int i = 0; i < dbModel->columnCount(); i++){
                QString colName = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
                QCheckBox* box = new QCheckBox(colName, &dialog);
                box->setChecked(dbView->columnWidth(i) != 0);
                layout->addWidget(box);
                boxes.append(box);
            }

        };

        rebuildBoxes();

        //ui
        QHBoxLayout* addLayout = new QHBoxLayout();
        QLineEdit* addEdit = new QLineEdit(&dialog);
        QPushButton* addBtn = new QPushButton(tr("Add Header"), &dialog);
        addLayout->addWidget(addEdit);
        addLayout->addWidget(addBtn);
        layout->addLayout(addLayout);

        QPushButton* okBtn = new QPushButton(tr("OK"), &dialog);
        layout->addWidget(okBtn);
        connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        
        //lambad to add custom header
        connect(addBtn, &QPushButton::clicked, [&](){
            QString newHeader = addEdit->text().trimmed();
            if(!newHeader.isEmpty()){
                dbModel->addHeader(newHeader);
                addEdit->clear();
                rebuildBoxes();
            }

        });

        if(dialog.exec() == QDialog::Accepted){
            for (int i = 0; i < boxes.size(); i++) {
                if(!boxes[i]->isChecked()) dbView->setColumnWidth(i, 0);
                else dbView->setColumnWidth(i, 1);
            }
            resizeTable();

            //save the new header settings
            QSettings settings;
            settings.beginGroup("DBViewHeaders");
            
            QStringList allHeaders;
            QStringList shownHeaders;
            for(int i = 0; i < dbModel->columnCount(); i++){
                QString header = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
                allHeaders << header;
                if(dbView->columnWidth(i) != 0){
                    shownHeaders << header;
                }

            }
            settings.setValue("all", allHeaders);
            settings.setValue("shown", shownHeaders);
            settings.endGroup();
        }


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
