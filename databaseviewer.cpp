/*
Database Viewer
Widget that holds database info in a QTableView
History:
March 18, 2025 - Program Creation
*/

#include "databaseviewer.h"

#include "tabledelegate.h"
#include "helpers.h"
#include "databasefilter.h"
#include "chessgamewindow.h"
#include "chessposition.h"
#include "chesstabhost.h"
#include "pgngamedata.h"
#include "draggablecheckbox.h"

#include <fstream>
#include <vector>
#include <QResizeEvent>
#include <QFile>
#include <QMenu>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSettings>
#include <QLayoutItem>
#include <QTimer>
#include <QThread>
#include <QRegularExpression>
#include <QValidator>
#include <QPushButton>
#include <QSplitter>
#include <QSpacerItem>
#include <QToolBar>
#include <QAction>
#include <QIcon>

// Initializes the DatabaseViewer
DatabaseViewer::DatabaseViewer(QString filePath, QWidget *parent)
    : QWidget(parent)
    , dbView(new QTableView(this))
    , host(new ChessTabHost)
    , m_filePath(filePath)
{
    setupUI();
    
    dbView->setItemDelegate(new TableDelegate(this));
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

    // set preview to a placeholder game (warms-up QML, stopping the window from blinking when a game is previewed)
    QSharedPointer<NotationMove> rootMove(new NotationMove("", *new ChessPosition));
    rootMove->m_position->setBoardData(convertFenToBoardData(rootMove->FEN));
    PGNGame game;
    ChessGameWindow *embed = new ChessGameWindow(this, game);
    embed->previewSetup();
    embed->setFocusPolicy(Qt::StrongFocus);

    // put embed inside gamePreview
    gamePreview->hide();
    QLayout* containerLayout = new QVBoxLayout(gamePreview);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(embed);
    gamePreview->setLayout(containerLayout);
    gamePreview->show();
    

    //load header settings    
    QSettings settings;
    settings.beginGroup("DBViewHeaders");
    QStringList allHeaders = settings.value("all").toStringList();
    mShownHeaders = settings.value("shown").toStringList();
    QVariant ratioVar = settings.value("ratios");
    settings.endGroup();

    for(const QString& header: allHeaders){
        dbModel->addHeader(header);
    }

    if(ratioVar.isValid()){
        QList<QVariant> ratioList = ratioVar.toList();
        for(const QVariant& v: ratioList){
            mRatios.append(v.toFloat());
        }
    }
    
    //in case not enough for some reason
    while (mRatios.size() < dbModel->columnCount()){
        mRatios.append(0.1);
    }

    if(mShownHeaders.isEmpty()){
        for(int i = 0; i < dbModel->columnCount(); i++){
            QString header = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            mShownHeaders << header;
        }
    }

    //hide hidden ones
    for(int i = 0; i < dbModel->columnCount(); i++){
        QString header = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        if(!mShownHeaders.contains(header)){
            dbView->setColumnWidth(i, 0);  
        }
    }
    
    // signals and slots
    connect(mFilterAction, &QAction::triggered, this, &DatabaseViewer::filter);
    connect(mAddGameAction, &QAction::triggered, this, &DatabaseViewer::addGame);
    connect(dbView, &QAbstractItemView::doubleClicked, this, &DatabaseViewer::onDoubleSelected);
    connect(dbView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &DatabaseViewer::onSingleSelected);
    connect(dbView, &QWidget::customContextMenuRequested, this, &DatabaseViewer::onContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, this, &DatabaseViewer::onHeaderContextMenu);

    //save columns ratios 300ms after editing
    mSaveTimer = new QTimer(this);
    mSaveTimer->setSingleShot(true);
    mSaveTimer->setInterval(300);
    connect(mSaveTimer, &QTimer::timeout, this, &DatabaseViewer::saveColumnRatios);
    connect(header, &QHeaderView::sectionResized, this, [this](){mSaveTimer->start();});
    connect(header, &QHeaderView::sectionResized, this, &DatabaseViewer::resizeSplitter);
}

void DatabaseViewer::setupUI()
{
    QVBoxLayout* verticalLayout = new QVBoxLayout(this);
    
    QToolBar* toolbar = new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setIconSize(QSize(24, 24));
    
    QAction* filterAction = new QAction(QIcon(":/resource/img/filter.png"), "Filter", this);
    QAction* addGameAction = new QAction(QIcon(":/resource/img/board-icon.png"), "Add Game", this);
    
    toolbar->addAction(filterAction);
    toolbar->addAction(addGameAction);
    
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    
    contentLayout = new QSplitter(Qt::Horizontal, this);
    
    gamePreview = new QWidget();
    gamePreview->setMaximumWidth(800);
    
    contentLayout->addWidget(dbView);
    contentLayout->addWidget(gamePreview);
    
    verticalLayout->addWidget(toolbar);
    verticalLayout->addWidget(contentLayout);
    
    setLayout(verticalLayout);
    
    mFilterAction = filterAction;
    mAddGameAction = addGameAction;

}

// Destructor
DatabaseViewer::~DatabaseViewer()
{
}

// Window resize event handler
void DatabaseViewer::resizeEvent(QResizeEvent *event)
{
    resizeTable();
}

void DatabaseViewer::showEvent(QShowEvent *event)
{
    // force render on show
    QWidget::showEvent(event);
    resizeTable();
}

// Custom table resizer
void DatabaseViewer::resizeTable(){
    float sum = 0.0f;
    for(int i = 0; i < dbModel->columnCount(); i++){
        if(dbView->columnWidth(i) > 0) {
            sum += mRatios[i];
            // qDebug() << mRatios[i] << sum;
        }
    }

    int totalWidth = dbView->viewport()->width();
    // qDebug() << totalWidth;
    for(int i = 0; i < dbModel->columnCount(); i++){
        if(dbView->columnWidth(i) > 0) dbView->setColumnWidth(i, totalWidth*mRatios[i]/sum);
        // qDebug() <<i << totalWidth << mRatios[i] << sum;
    }

    
}

void DatabaseViewer::resizeSplitter(){
    int totalColumnWidth = 0;
    for(int i = 0; i < dbModel->columnCount(); i++){
        if(dbView->columnWidth(i) > 0) { 
            totalColumnWidth += dbView->columnWidth(i);
        }
    }
    
    QList<int> sizes = contentLayout->sizes();
    int totalSplitterWidth = sizes[0] + sizes[1];
    contentLayout->setSizes({totalColumnWidth, totalSplitterWidth-totalColumnWidth});
}

void DatabaseViewer::saveColumnRatios(){
    int cols = dbModel->columnCount();
    int totalWidth = dbView->viewport()->width();
    QList<QVariant> ratios;

    while(mRatios.size() < cols) mRatios.append(0.1);

    for(int i = 0; i < cols; i++){
        if(dbView->columnWidth(i) != 0){        
            float ratio = float(dbView->columnWidth(i)) / float(totalWidth);
            ratios.append(QVariant(ratio));
            mRatios[i] = ratio;
        }
        else{
            ratios.append(QVariant(mRatios[i]));
        }
    }
    QSettings settings;
    settings.beginGroup("DBViewHeaders");
    settings.setValue("ratios", ratios);
    settings.endGroup();
}

// Handles custom filters
void DatabaseViewer::filter(){

    // init filter window
    DatabaseFilter filterWindow(this);

    // apply filters    
    if(filterWindow.exec() == QDialog::Accepted){
        auto filters = filterWindow.getNameFilters();

        proxyModel->setPlayerFilter(filters.whiteFirst, filters.whiteLast, filters.blackFirst, filters.blackLast, filters.ignoreColours);
        proxyModel->setRangeFilter("Elo", filters.eloMin, filters.eloMax);
    }
}

QString findTag(const QVector<QPair<QString,QString>>& hdr, const QString& tag, const QString& notFound = QStringLiteral("?"))
{
    for (auto &kv : hdr) {
        if (kv.first == tag) return kv.second;

        //custom ones
        if(tag == "bElo" && kv.first == "BlackElo") return kv.second;
        if(tag == "wElo" && kv.first == "WhiteElo") return kv.second;

    }
    return notFound;
}

void DatabaseViewer::addGame(){
    PGNGame game; 
    int row = dbModel->rowCount();
    game.dbIndex = row;
    game.isParsed = true;
    game.headerInfo.push_back({QString("#"), QString::number(row+1)});
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
    QVector<PGNGame> database;
    for (int i = 0; i < dbModel->rowCount(); ++i){
        database.append(dbModel->getGame(i));
    }
    emit saveRequested(m_filePath, database);
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
        dialog.resize(300, 400);
        QVBoxLayout* layout = new QVBoxLayout(&dialog);

        DraggableCheckBoxContainer* container = new DraggableCheckBoxContainer(&dialog);

        QStringList essentialHeaders = {"#", "White", "Black", "Result", "Event", "Date"};
        
        //readd in proper order
        for(int i = 0; i < dbModel->columnCount(); i++){
            QString colName = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
            DraggableCheckBox*  box = new DraggableCheckBox(colName, &dialog);
            box->setChecked(dbView->columnWidth(i) != 0);

            if(essentialHeaders.contains(colName)){
                box->setDeleteEnabled(false);
                box->setDragEnabled(false);
            }
            connect(box, &DraggableCheckBox::deleteRequested, [=](){
                if(!essentialHeaders.contains(colName)){
                    int headerIndex = dbModel->headerIndex(colName);
                    if(headerIndex >= 0){
                        dbModel->removeHeader(headerIndex);
                        container->removeCheckBox(box);  
                        
                        if(headerIndex < mRatios.size()){
                            mRatios.removeAt(headerIndex);
                        }
                    }
                }
            });
            container->addCheckBox(box);
        }

        layout->addWidget(container);

        //ui
        QHBoxLayout* addLayout = new QHBoxLayout();
        QLineEdit* addEdit= new QLineEdit(&dialog);
        QRegularExpression regex("[A-Za-z]{0,15}");  
        QValidator* validator = new QRegularExpressionValidator(regex, addEdit);
        addEdit->setValidator(validator);
        addEdit->setPlaceholderText("e.g ECO, WhiteTitle");

        QPushButton* addBtn = new QPushButton(tr("Add Header"), &dialog);
        addLayout->addWidget(addEdit);
        addLayout->addWidget(addBtn);
        layout->addLayout(addLayout);

        QPushButton* okBtn = new QPushButton(tr("OK"), &dialog);
        layout->addWidget(okBtn);


        connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
            
        connect(addBtn, &QPushButton::clicked, [&](){  
            QString newHeader = addEdit->text().trimmed();
            if(!newHeader.isEmpty() && dbModel->headerIndex(newHeader) == -1){
                dbModel->addHeader(newHeader);

                //insert the thing last
                DraggableCheckBox* box = new DraggableCheckBox(newHeader, &dialog);
                box->setChecked(true);
                container->addCheckBox(box);

                addEdit->clear();
                mRatios.append(0.1f);
            }
        });

        if(dialog.exec() == QDialog::Accepted){

            QVector<DraggableCheckBox*> boxes = container->getCheckBoxes();

            QStringList columnOrder;
            QStringList shownHeaders;

            for(DraggableCheckBox* box: boxes){
                columnOrder << box->text();
                if(box->isChecked()) shownHeaders << box->text();
            }

            //visibility
            for (int i = 0; i < dbModel->columnCount(); i++) {
                QString headerName = dbModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
                if(!shownHeaders.contains(headerName)) dbView->setColumnWidth(i, 0);
                else dbView->setColumnWidth(i, 1);
            }

            //order visually
            QHeaderView* header = dbView->horizontalHeader();
            for(int visualPos = 0; visualPos < columnOrder.size(); visualPos++){
                QString headerName = columnOrder[visualPos];
                int logicalIndex = dbModel->headerIndex(headerName);
                if(logicalIndex >= 0){
                    int currentVisualPos = header->visualIndex(logicalIndex);
                    if(currentVisualPos != visualPos){
                        header->moveSection(currentVisualPos, visualPos);
                    }
                }
            }

            resizeTable();



            //save the new header settings
            QSettings settings;
            settings.beginGroup("DBViewHeaders");
            settings.setValue("all", columnOrder);
            settings.setValue("shown", shownHeaders);

            // QList<QVariant> ratioVariants;
            // for(float ratio : mRatios) {
            //     ratioVariants.append(QVariant(ratio));
            // }
            // settings.setValue("ratios", ratioVariants);
            settings.endGroup();


            
            mShownHeaders = shownHeaders;

            // for(int i = 0; i < shownHeaders.length(); i++) qDebug() << shownHeaders.at(i);
        }
    }

}

// Handles game preview
void DatabaseViewer::onSingleSelected(const QModelIndex &proxyIndex, const QModelIndex &previous)
{
    if (!proxyIndex.isValid() || proxyIndex.row() < 0) return;

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

    gamePreview->hide();
    if (gamePreview->layout()) {
        clearPreview(gamePreview);
    }
    QLayout* containerLayout = new QVBoxLayout(gamePreview);

    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_embed);
    gamePreview->setLayout(containerLayout);
    gamePreview->show();
}

void DatabaseViewer::setWindowTitle(QString text)
{
    host->setWindowTitle(text);
}
