#include "openingviewer.h"
#include "pgngamedata.h"

#include <algorithm>   
#include <cstring>  
#include <QFile>
#include <QDataStream>
#include <QtGlobal>
#include <QHeaderView>

OpeningTree::OpeningTree()
    : mRoot(new BuildNode),
      mMappedBase(nullptr),
      mCurOffset(0)
{

}

OpeningTree::~OpeningTree() {
    if (mMappedBase) mFile.unmap(reinterpret_cast<uchar*>(const_cast<char*>(mMappedBase)));
    deleteSubtree(mRoot);
}

void OpeningTree::insertGame(const QVector<quint16>& moves, int gameID, GameResult result){
    BuildNode* node = mRoot;
    node->gamesReached++;

    if (result == WHITE_WIN) node->whiteWins++;
    else if (result == DRAW) node->draws++;

    for (int i = 0; i < moves.size(); i++){
        quint16 m = moves[i];
        // find move in trie
        auto it = std::find_if(node->children.begin(), node->children.end(), [=](auto &pr){return pr.first ==m;});
        if(it == node->children.end()){
            // not alreaqy exist
            auto* child = new BuildNode;
            child->gamesReached = 1;

            if (result == WHITE_WIN) child->whiteWins = 1;
            else if (result == DRAW) child->draws = 1;

            node->children.append(qMakePair(m, child));
            node = child;
        }
        else{
            // increment gamesreached if already there
            it->second->gamesReached++;

            if (result == WHITE_WIN) it->second->whiteWins++;
            else if (result == DRAW) it->second->draws++;
            
            node = it->second;
        }

        if(i == moves.size() - 1){
            node->gameIds.append(gameID);
        }
    }
}

// serialize
bool OpeningTree::serialize(const QString& path){
    assignOffsets();
    QFile f(path);

    if (!f.open(QIODevice::WriteOnly)) return false;
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);

    // write in bfs order
    for(auto* node: mBfsOrder){
        quint64 off = mOffsets[node];
        f.seek(off);
        out << node->gamesReached;
        out << node->whiteWins;
        out << node->draws;
        quint8 cc = node->children.size();
        out << cc;
        for(auto& pr: node->children){
            quint16 mv = pr.first;
            BuildNode* ch = pr.second;
            quint32 cnt = ch->gamesReached;
            quint32 ww = ch->whiteWins;
            quint32 dd = ch->draws;
            quint64 chOff = mOffsets[ch];
            out << mv << cnt << ww << dd << chOff;
        }
        quint8 gameIdCount = node->gameIds.size();
        out << gameIdCount;
        for(int id : node->gameIds) {
            out << id;
        }
    }
    
    return true;
}

// load from serialized
bool OpeningTree::load(const QString& path) {
    mFile.setFileName(path);
    if (!mFile.open(QIODevice::ReadOnly)) return false;
    mMappedSize = quint64(mFile.size());
    uchar* ptr = mFile.map(0, mMappedSize);
    if (!ptr) return false;
    mMappedBase = reinterpret_cast<const char*>(ptr);

    return true;
}

// navigate

void OpeningTree::reset(){
    mCurOffset = 0;
}

bool OpeningTree::play(quint16 moveCode){
    NodeView nv = readNode(mCurOffset);
    for(int i = 0; i < nv.childCount; i++){
        if (nv.children[i].moveCode == moveCode){
            mCurOffset = nv.children[i].offset;
            return 1;
        }
    }

    // no games
    return 0;
}
 
quint32 OpeningTree::gamesReached() const {
    return readNode(mCurOffset).gamesReached;
}


QVector<Continuation> OpeningTree::continuations() const {
    NodeView nv = readNode(mCurOffset);
    QVector<Continuation> out;
    out.reserve(nv.childCount);
    for (int i = 0; i < nv.childCount; i++){
        quint32 count = nv.children[i].count;
        quint32 whiteWins = nv.children[i].whiteWins;
        quint32 draws = nv.children[i].draws;
        quint32 blackWins = count - whiteWins - draws;  
        
        float whitePct = (count > 0) ? (float)whiteWins / count * 100.0f : 0.0f;
        float drawPct = (count > 0) ? (float)draws / count * 100.0f : 0.0f;
        float blackPct = (count > 0) ? (float)blackWins / count * 100.0f : 0.0f;
            
        out.append({nv.children[i].moveCode,  count, whitePct, drawPct, blackPct});
    }
    return out;
}

QVector<int> OpeningTree::getIds() const {
    QVector<int> result;
    collectGameIds(mCurOffset, result);
    return result;
}

void OpeningTree::collectGameIds(quint64 nodeOffset, QVector<int>& ids) const {
    NodeView nv = readNode(nodeOffset);
    
    const char* p = reinterpret_cast<const char*>(nv.children) + (nv.childCount * sizeof(ChildEntry));
    quint8 gameIdCount;
    std::memcpy(&gameIdCount, p, 1); p += 1;
    
    for (int i = 0; i < gameIdCount; i++) {
        int gameId;
        std::memcpy(&gameId, p, 4); p += 4;
        if (!ids.contains(gameId)) {
            ids.append(gameId);
        }
    }
    
    // recursively check
    for (int i = 0; i < nv.childCount; i++) {
        collectGameIds(nv.children[i].offset, ids);
    }
}

void OpeningTree::deleteSubtree(BuildNode* n) {
    // delete node recursively
    for(auto& pr: n->children) deleteSubtree(pr.second);
    delete n;
}

void OpeningTree::assignOffsets(){
    mBfsOrder.clear();
    mOffsets.clear();
    quint64 nextOff = 0;
    mBfsOrder.append(mRoot);
    for(int i = 0; i < mBfsOrder.size(); i++){
        BuildNode* n = mBfsOrder[i];
        mOffsets[n] = nextOff;
        quint64 size = 4 + 4 + 4 + 1 + quint64(n->children.size()) * (2+4+4+4+8) + 1 + quint64(n->gameIds.size()) * 4;
        nextOff += size;
        for(auto& pr: n -> children) mBfsOrder.append(pr.second);
    }
}

OpeningTree::NodeView OpeningTree::readNode(quint64 off) const {
    const char* p = static_cast<const char*>(mMappedBase) + off;
    NodeView nv;
    std::memcpy(&nv.gamesReached, p, 4);  p += 4;
    std::memcpy(&nv.whiteWins, p, 4);   p += 4;
    std::memcpy(&nv.draws, p, 4);       p += 4;
    std::memcpy(&nv.childCount, p, 1);  p += 1;
    nv.children = reinterpret_cast<const ChildEntry*>(p);

    return nv;
}


OpeningViewer::OpeningViewer(QWidget *parent)
    : QWidget{parent}
{   
    // load opening book
    mOpeningBookLoaded = mTree.load("./opening/openings.bin");

    mTree.mOpeningInfo.deserialize("./opening/serialtest.bin");
    for (int i = 0; i < mTree.mOpeningInfo.zobristPositions.size(); i++){
        int prevInd = (i == 0 ? 0 : mTree.mOpeningInfo.prefixSum.back());
        int gameCount = mTree.mOpeningInfo.whiteWin[i] + mTree.mOpeningInfo.blackWin[i] + mTree.mOpeningInfo.draw[i];
        mTree.mOpeningInfo.prefixSum.push_back(prevInd + gameCount);
    }

    //
    // ui
    //

    QHBoxLayout* mainLayout = new QHBoxLayout();

    // moves list side
    QVBoxLayout* listsLayout = new QVBoxLayout();

    mPositionLabel = new QLabel("Starting Position");
    mPositionLabel->setStyleSheet("font-weight: bold; font-size: 13px;");

    mStatsLabel = new QLabel("No position data");
    mStatsLabel->setStyleSheet("font-size: 12px;"); 

    mMovesList = new QTreeWidget();
    mMovesList->setHeaderLabels(QStringList() << "Move" << "Games" << "Win %");
    mMovesList->setRootIsDecorated(false);
    mMovesList->setAlternatingRowColors(true);
    mMovesList->setSortingEnabled(true);
    mMovesList->sortByColumn(1, Qt::DescendingOrder);
    mMovesList->setMinimumHeight(150);

    listsLayout->addWidget(mPositionLabel);
    listsLayout->addWidget(mStatsLabel);
    listsLayout->addWidget(mMovesList);
    mainLayout->addLayout(listsLayout);
    
    
    // games list side
    mGamesLabel = new QLabel(tr("Games"));
    mGamesLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    
    mGamesList = new QTableWidget();
    mGamesList->setColumnCount(7);  
    mGamesList->setHorizontalHeaderLabels(QStringList() << "White" << "WhiteElo" << "Black" << "BlackElo" << "Result" << "Date" << "Event");
    mGamesList->setAlternatingRowColors(true);
    mGamesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    mGamesList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mGamesList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    mGamesList->horizontalHeader()->setStretchLastSection(true);
    mGamesList->setMinimumHeight(150);
    mGamesList->setMinimumWidth(400);
    
    QVBoxLayout* gamesLayout = new QVBoxLayout();
    gamesLayout->addWidget(mGamesLabel);
    gamesLayout->addWidget(mGamesList);
    mainLayout->addLayout(gamesLayout); 
    
    setLayout(mainLayout);
    
    
    // styles
    QString styleSheet = R"(
        QTreeWidget, QTableWidget {
            border: 1px solid palette(mid);
            border-radius: 3px;
        }
        QTreeWidget::item, QTableWidget::item {
            height: 22px;
        }
        QTreeWidget::item:hover, QTableWidget::item:hover {
            background: palette(highlight);
            color: palette(highlighted-text);
            opacity: 0.5;
        }
        QTreeWidget::item:selected, QTableWidget::item:selected {
            background: palette(highlight);
            color: palette(highlighted-text);
        }
    )";
    
    mMovesList->setStyleSheet(styleSheet);
    mGamesList->setStyleSheet(styleSheet);
    
    connect(mMovesList, &QTreeWidget::itemDoubleClicked, this, &OpeningViewer::onMoveSelected);
    connect(mGamesList, &QTableWidget::cellDoubleClicked, this, &OpeningViewer::onGameSelected);
}

void OpeningViewer::updatePosition(const quint64 zobrist, QSharedPointer<ChessPosition> position)
{
    auto winrate = getWinrate(zobrist);
    int total = winrate.whiteWin + winrate.blackWin + winrate.draw;
    if (total == 0){
        mMovesList->clear();
        mGamesList->setRowCount(0);
        mGamesLabel->setText("Games: 0 of 0 shown");
        mStatsLabel->setText(tr("0 Games"));
        return;
    }

    mStatsLabel->setText(tr("%1 Games").arg(total));
    mMovesList->clear();

    auto legalMoves = position->generateLegalMoves();
    // legalMoves.clear();
    for (const auto [sr, sc, dr, dc, promo]: std::as_const(legalMoves)){
        ChessPosition tempPos;
        tempPos.copyFrom(*position);
        tempPos.applyMove(sr, sc, dr, dc, QChar(promo));
        auto newWin = getWinrate(tempPos.computeZobrist());
        int total = newWin.whiteWin + newWin.blackWin + newWin.draw;
        if (total){
            float whitePct = newWin.whiteWin / total, blackPct = newWin.blackWin / total, drawPct = newWin.blackWin / total;
            addMoveToList(position->lanToSan(sr, sc, dr, dc, QChar(promo)), total, whitePct, drawPct, blackPct);
        }
    }

    updateGamesList();
}

PositionWinrate OpeningViewer::getWinrate(const quint64 zobrist)
{
    PositionWinrate winrate = {0, 0, 0};
    int low = 0, high = mTree.mOpeningInfo.zobristPositions.size()-1;
    while (low < high){
        int mid = (low + high)/2;
        if (mTree.mOpeningInfo.zobristPositions[mid] < zobrist){
            low = mid+1;
        } else {
            high = mid;
        }
    }
    if (zobrist == mTree.mOpeningInfo.zobristPositions[low]) {
        winrate.whiteWin += mTree.mOpeningInfo.whiteWin[low];
        winrate.blackWin += mTree.mOpeningInfo.blackWin[low];
        winrate.draw += mTree.mOpeningInfo.draw[low];
    }
    return winrate;
}

// void OpeningViewer::updatePosition(const QVector<QString>& uciMoves)
// {
//     if (!mOpeningBookLoaded) {
//         mPositionLabel->setText(tr("No opening database loaded"));
//         mStatsLabel->setText(tr(""));
//         mMovesList->clear();
//         mGamesList->clear();
//         return;
//     }

//     mTree.reset();
//     for (const QString& uci: uciMoves){
//         quint16 code = encodeMove(uci);
//         if(!mTree.play(code)){
//             // no games
//             mMovesList->clear();
//             mGamesList->setRowCount(0);
//             mGamesLabel->setText("Games: 0 of 0 shown");
//             mStatsLabel->setText(tr("0 Games"));
//             return;
//         }
//     }

//     quint32 total = mTree.gamesReached();
//     mStatsLabel->setText(tr("%1 Games").arg(total));

//     mMovesList->clear();
//     for(auto cont: mTree.continuations()) {
//         QString uci = decodeMove(cont.moveCode);
//         addMoveToList(uci, cont.count, cont.whitePct, cont.drawPct, cont.blackPct);
//     }
    
//     updateGamesList();
// }

void OpeningViewer::updateGamesList()
{
    mGamesList->setRowCount(0);  
    
    QVector<int> gameIds = mTree.getIds();
    
    const int MAX_GAMES_TO_SHOW = 1000;
    mGamesLabel->setText(tr("Games: %1 of %2 shown").arg(qMin(gameIds.size(), MAX_GAMES_TO_SHOW)).arg(gameIds.size()));
    
    // no sorting while loading
    mGamesList->setSortingEnabled(false);
    
    // add games
    for (int i = 0; i < qMin(gameIds.size(), MAX_GAMES_TO_SHOW); i++) {
        PGNGame game = PGNGame::loadGameHeader("./opening/openings.headers", gameIds[i]);
        
        QString white, whiteElo, black, blackElo, result, date, event;
        for (const auto& header : game.headerInfo) {
            if (header.first == "White") white = header.second;
            else if (header.first == "WhiteElo") whiteElo = header.second;
            else if (header.first == "Black") black = header.second;
            else if (header.first == "BlackElo") blackElo = header.second;
            else if (header.first == "Date") date = header.second;
            else if (header.first == "Event") event = header.second;
        }
        result = game.result;
        
        int row = mGamesList->rowCount();
        mGamesList->insertRow(row);
        
        mGamesList->setItem(row, 0, new QTableWidgetItem(white));
        mGamesList->setItem(row, 1, new QTableWidgetItem(whiteElo));
        mGamesList->setItem(row, 2, new QTableWidgetItem(black));
        mGamesList->setItem(row, 3, new QTableWidgetItem(blackElo));
        mGamesList->setItem(row, 4, new QTableWidgetItem(result));
        mGamesList->setItem(row, 5, new QTableWidgetItem(date));
        mGamesList->setItem(row, 6, new QTableWidgetItem(event));
        
        mGamesList->item(row, 0)->setData(Qt::UserRole, gameIds[i]);
    }
    
    mGamesList->setSortingEnabled(true);
    
}

// helper
void OpeningViewer::addMoveToList(const QString& move, int games, float whitePct, float drawPct, float blackPct)
{
    MoveListItem* item = new MoveListItem(mMovesList);
    
    item->setText(0, move);
    item->setText(1, QString::number(games));

    // colour bars
    QString percentText = QString("%1% / %2% / %3%").arg(whitePct, 0, 'f', 1).arg(drawPct, 0, 'f', 1).arg(blackPct, 0, 'f', 1);

    item->setText(2, percentText);

    
    // data for sorting
    item->setData(0, Qt::UserRole, move);
    item->setData(1, Qt::UserRole, games);
    item->setData(2, Qt::UserRole, whitePct);
}

void OpeningViewer::onMoveSelected(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    
    QString move = item->data(0, Qt::UserRole).toString();
    emit moveClicked(move);
}

void OpeningViewer::onGameSelected(int row, int column)
{
    if (row < 0 || row >= mGamesList->rowCount()) return;
    
    int gameId = mGamesList->item(row, 0)->data(Qt::UserRole).toInt();
    emit gameSelected(gameId);
}


// some stolen conversion methods
quint16 OpeningViewer::encodeMove(const QString& uci)
{
    if (uci.size() < 4) return 0;
    int fileFrom = uci[0].toLatin1() - 'a';
    int rankFrom = uci[1].digitValue() - 1;
    int fileTo   = uci[2].toLatin1() - 'a';
    int rankTo   = uci[3].digitValue() - 1;

    quint16 from = quint16(rankFrom * 8 + fileFrom);
    quint16 to   = quint16(rankTo   * 8 + fileTo);
    return (from << 6) | to;
}

QString OpeningViewer::decodeMove(quint16 code)
{
    quint16 from = code >> 6;
    quint16 to   = code &  0x3F;
    int fileFrom = from % 8;
    int rankFrom = from / 8;
    int fileTo   = to   % 8;
    int rankTo   = to   / 8;

    auto sq = [](int f, int r){
        return QString(QChar('a' + f)) + QString::number(r + 1);
    };
    return sq(fileFrom, rankFrom) + sq(fileTo, rankTo);
}
