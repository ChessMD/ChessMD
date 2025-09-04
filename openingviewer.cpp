#include "openingviewer.h"
#include "pgngamedata.h"

#include <cstring>  
#include <QFile>
#include <QDataStream>
#include <QtGlobal>
#include <QHeaderView>

const int MAX_GAMES_TO_SHOW = 1000;
const int MAX_OPENING_DEPTH = 70; // counted in half-moves

bool OpeningInfo::serialize(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_5); // set version for compatibility

    // write arrays
    out << gameIDs;
    out << zobristPositions;
    out << insertedCount;
    out << whiteWin;
    out << blackWin;
    out << draw;

    return out.status() == QDataStream::Ok;
}

bool OpeningInfo::deserialize(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_5);

    in >> gameIDs;
    in >> zobristPositions;
    in >> insertedCount;
    in >> whiteWin;
    in >> blackWin;
    in >> draw;

    return in.status() == QDataStream::Ok;
}

OpeningViewer::OpeningViewer(QWidget *parent)
    : QWidget{parent}
{   
    // load opening book
    mOpeningBookLoaded = mOpeningInfo.deserialize("./opening/openings.bin");
    mOpeningInfo.prefixSum.push_back(0);
    for (int i = 0; i < mOpeningInfo.zobristPositions.size(); i++){
        mOpeningInfo.prefixSum.push_back(mOpeningInfo.prefixSum.back() + mOpeningInfo.insertedCount[i]);
    }

    QHBoxLayout* mainLayout = new QHBoxLayout();

    // moves list side
    QVBoxLayout* listsLayout = new QVBoxLayout();

    mPositionLabel = new QLabel("Loading Position...");
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
    
    connect(mMovesList, &QTreeWidget::itemDoubleClicked, this, &OpeningViewer::onNextMoveSelected);
    connect(mGamesList, &QTableWidget::cellDoubleClicked, this, &OpeningViewer::onGameSelected);
}

void OpeningViewer::onMoveSelected(QSharedPointer<NotationMove>& move)
{
    if (!move->m_zobristHash) move->m_zobristHash = move->m_position->computeZobrist();
    updatePosition(move->m_zobristHash, move->m_position, move->moveText);
}

void OpeningViewer::updatePosition(const quint64 zobrist, QSharedPointer<ChessPosition> position, const QString moveText)
{
    if (!mOpeningInfo.zobristPositions.size()){
        return;
    }

    auto [winrate, openingIndex] = getWinrate(zobrist);
    int total = winrate.whiteWin + winrate.blackWin + winrate.draw;

    mStatsLabel->setText(tr("%1 Games").arg(total));
    mMovesList->clear();

    auto legalMoves = position->generateLegalMoves();
    for (const auto [sr, sc, dr, dc, promo]: std::as_const(legalMoves)){
        ChessPosition tempPos;
        tempPos.copyFrom(*position);
        tempPos.applyMove(sr, sc, dr, dc, QChar(promo));
        auto [newWin, _] = getWinrate(tempPos.computeZobrist());
        int total = newWin.whiteWin + newWin.blackWin + newWin.draw;
        if (total){
            float whitePct = newWin.whiteWin * 100 / total, blackPct = newWin.blackWin * 100 / total, drawPct = newWin.draw * 100 / total;
            addMoveToList(position->lanToSan(sr, sc, dr, dc, QChar(promo)), total, whitePct, drawPct, blackPct);
        }
    }

    QString numPrefix;
    int moveNum = (position->getPlyCount()-1)/2 + 1;
    if (position->m_sideToMove == 'b') {
        numPrefix = QString::number(moveNum) + ".";
    } else {
        numPrefix = QString::number(moveNum) + "...";
    }
    mPositionLabel->setText((moveText.isEmpty() ? "Starting Position" : "Position after " + numPrefix + moveText));
    if (total){
        updateGamesList(openingIndex);
    } else {
        mMovesList->clear();
        mGamesList->setRowCount(0);
        mGamesLabel->setText("Games: 0 of 0 shown");
        mStatsLabel->setText(tr("0 Games"));
    }
}

QPair<PositionWinrate, int> OpeningViewer::getWinrate(const quint64 zobrist)
{
    PositionWinrate winrate = {0, 0, 0};
    int low = 0, high = mOpeningInfo.zobristPositions.size()-1;
    while (low < high){
        int mid = (low + high)/2;
        if (mOpeningInfo.zobristPositions[mid] < zobrist){
            low = mid+1;
        } else {
            high = mid;
        }
    }
    if (zobrist == mOpeningInfo.zobristPositions[low]) {
        winrate.whiteWin += mOpeningInfo.whiteWin[low];
        winrate.blackWin += mOpeningInfo.blackWin[low];
        winrate.draw += mOpeningInfo.draw[low];
    }
    // qDebug() << low << zobrist << mOpeningInfo.zobristPositions[low];
    return {winrate, low};
}

bool OpeningViewer::ensureHeaderOffsetsLoaded(const QString &path)
{
    if (mHeaderOffsetsLoaded) return true;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open headers file for offsets:" << path;
        return false;
    }
    QDataStream in(&f);
    quint32 gameCount;
    in >> gameCount;

    mHeaderOffsets.resize(gameCount);
    f.seek(4);
    for (quint32 i = 0; i < gameCount; ++i) {
        quint64 off;
        in >> off;
        mHeaderOffsets[i] = off;
    }
    f.close();
    mHeaderOffsetsLoaded = true;
    return true;
}

QVector<PGNGame> OpeningViewer::loadGameHeadersBatch(const QString &path, const QVector<quint32> &ids)
{
    QVector<PGNGame> out;
    out.reserve(ids.size());

    if (!ensureHeaderOffsetsLoaded(path)) return out;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "cannot open headers file:" << path;
        return out;
    }
    QDataStream in(&f);

    for (quint32 gid : ids) {
        if (gid >= static_cast<quint32>(mHeaderOffsets.size())) {
            qDebug() << "Bad game id!" << gid;
            out.append(PGNGame());
            continue;
        }
        quint64 off = mHeaderOffsets[gid];
        f.seek(off);
        PGNGame game;
        QString white, whiteElo, black, blackElo, event, date, result;
        in >> white >> whiteElo >> black >> blackElo >> event >> date >> result;
        if (!white.isEmpty()) game.headerInfo.push_back(qMakePair(QString("White"), white));
        if (!whiteElo.isEmpty()) game.headerInfo.push_back(qMakePair(QString("WhiteElo"), whiteElo));
        if (!black.isEmpty()) game.headerInfo.push_back(qMakePair(QString("Black"), black));
        if (!blackElo.isEmpty()) game.headerInfo.push_back(qMakePair(QString("BlackElo"), blackElo));
        if (!event.isEmpty()) game.headerInfo.push_back(qMakePair(QString("Event"), event));
        if (!date.isEmpty()) game.headerInfo.push_back(qMakePair(QString("Date"), date));
        if (!result.isEmpty()) {
            game.headerInfo.push_back(qMakePair(QString("Result"), result));
            game.result = result;
        }
        in >> game.bodyText;
        game.isParsed = false;
        out.append(std::move(game));
    }

    f.close();
    return out;
}

void OpeningViewer::updateGamesList(const int openingIndex)
{
    if (openingIndex+1 >= mOpeningInfo.prefixSum.size()){
        qDebug() << "Failed to update opening game list: openingIndex out of range!";
        return;
    }

    QVector<quint32> gameIDs;
    int startInd = mOpeningInfo.prefixSum[openingIndex], endInd = mOpeningInfo.prefixSum[openingIndex+1];
    gameIDs.reserve(qMin(endInd - startInd, MAX_GAMES_TO_SHOW));
    for (int i = startInd; i < endInd && i - startInd < MAX_GAMES_TO_SHOW; i++){
        if (i >= mOpeningInfo.gameIDs.size()){
            qDebug() << "Failed to add game(s) to opening game list: prefixSum out of range!";
            break;
        }
        gameIDs.push_back(mOpeningInfo.gameIDs[i]);
    }

    QVector<PGNGame> games = loadGameHeadersBatch("./opening/openings.headers", gameIDs);

    mGamesList->setRowCount(0);
    mGamesList->setSortingEnabled(false); // no sorting while loading
    for (int i = 0; i < gameIDs.size(); i++) {
        const PGNGame &game = games[i];
        QString white, whiteElo, black, blackElo, result, date, event;
        for (const auto& header : std::as_const(game.headerInfo)) {
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
        mGamesList->item(row, 0)->setData(Qt::UserRole, gameIDs[i]);
    }

    mGamesList->setSortingEnabled(true);
    mGamesLabel->setText(tr("Games: %1 of %2 shown").arg(qMin(gameIDs.size(), MAX_GAMES_TO_SHOW)).arg(mOpeningInfo.whiteWin[openingIndex] + mOpeningInfo.blackWin[openingIndex] + mOpeningInfo.draw[openingIndex]));
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

void OpeningViewer::onNextMoveSelected(QTreeWidgetItem* item, int column)
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
