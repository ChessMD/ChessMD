#include "openingviewer.h"
#include "pgngamedata.h"

#include <cstring>  
#include <QFile>
#include <QDataStream>
#include <QtGlobal>
#include <QHeaderView>

const int MAX_GAMES_TO_SHOW = 1000;
const int MAX_OPENING_DEPTH = 70; // counted in half-moves
const quint64 MAGIC = 0x4F50454E424B3131ULL;
const quint32 VERSION = 1;

bool OpeningInfo::serialize(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "serialize: cannot open" << path;
        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_5);

    // write magic & version as raw to be easy to parse in mmap
    file.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
    file.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));

    // write N (count)
    quint64 N = static_cast<quint64>(zobristPositions.size());
    file.write(reinterpret_cast<const char*>(&N), sizeof(N));

    // write raw zobrist array
    if (N) {
        const quint64* ptr = reinterpret_cast<const quint64*>(zobristPositions.constData());
        qint64 bytes = static_cast<qint64>(N * sizeof(quint64));
        qint64 written = file.write(reinterpret_cast<const char*>(ptr), bytes);
        if (written != bytes) { file.close(); return false; }
    }

    // write PositionInfo entries
    for (int i = 0; i < N; ++i) {
        PositionInfo pi;
        pi.insertedCount = (i < insertedCount.size()) ? static_cast<quint32>(insertedCount[i]) : 0;
        pi.whiteWin = (i < whiteWin.size()) ? static_cast<quint32>(whiteWin[i]) : 0;
        pi.blackWin = (i < blackWin.size()) ? static_cast<quint32>(blackWin[i]) : 0;
        pi.draw = (i < draw.size()) ? static_cast<quint32>(draw[i]) : 0;
        pi.startIndex = (i < startIndex.size()) ? static_cast<quint32>(startIndex[i]) : 0;
        file.write(reinterpret_cast<const char*>(&pi), sizeof(pi));
    }

    // write gameIDs as raw uint32 array
    if (!gameIDs.isEmpty()) {
        const quint32* gptr = reinterpret_cast<const quint32*>(gameIDs.constData());
        qint64 bytes = static_cast<qint64>(gameIDs.size() * sizeof(quint32));
        qint64 written = file.write(reinterpret_cast<const char*>(gptr), bytes);
        if (written != bytes) {
            file.close();
            return false;
        }
    }

    file.flush();
    file.close();
    return true;
}

bool OpeningInfo::deserialize(const QString& path) {
    unmapDataFile(); // close previous map if any

    m_dataFilePath = path;
    m_mappedFile.setFileName(path);
    if (!m_mappedFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Deserialize: cannot open file for mapping:" << path;
        return false;
    }

    qint64 totalSize = m_mappedFile.size();
    if (totalSize < static_cast<qint64>(sizeof(quint64) + sizeof(quint32) + sizeof(quint64))) {
        qDebug() << "Deserialize: file too small";
        m_mappedFile.close();
        return false;
    }

    const uchar* base = m_mappedFile.map(0, totalSize);
    if (!base) {
        qDebug() << "Deserialize: mmap failed";
        m_mappedBase = nullptr;
        m_mappedSize = 0;
        m_mappedFile.close();
        return false;
    }

    // parse header
    const uchar* p = base;
    const quint64 magic = *reinterpret_cast<const quint64*>(p); p += sizeof(quint64);
    const quint32 version = *reinterpret_cast<const quint32*>(p); p += sizeof(quint32);
    quint64 N = *reinterpret_cast<const quint64*>(p); p += sizeof(quint64);

    if (magic != MAGIC) {
        qDebug() << "Deserialize: Bad magic:" << QString::number(magic, 16) << "expected:" << QString::number(MAGIC, 16);
        return false;
    }
    if (version != VERSION) {
        qDebug() << "Deserialize: Unsupported version:" << version;
        return false;
    }

    // bounds check
    quint64 expectedMin = sizeof(quint64) + sizeof(quint32) + sizeof(quint64) + N * sizeof(quint64) + N * sizeof(PositionInfo);
    if (static_cast<quint64>(totalSize) < expectedMin) {
        qDebug() << "Deserialize: File too small for header + arrays";
        m_mappedFile.unmap(const_cast<uchar*>(base));
        m_mappedBase = nullptr; m_mappedSize = 0; m_mappedFile.close();
        return false;
    }

    // set pointers
    m_mappedBase = base;
    m_mappedSize = totalSize;

    m_nPositions = static_cast<int>(N);
    // zobrist base points to the next location
    m_zobristBase = reinterpret_cast<const quint64*>(p);
    // position info base after zobrist array:
    p += N * sizeof(quint64);
    m_positionInfoStart = static_cast<quint64>(p - base); // offset into mapped base
    m_gameIdsDataStart = m_positionInfoStart + N * sizeof(PositionInfo);

    zobristPositions.clear();
    insertedCount.clear();
    whiteWin.clear();
    blackWin.clear();
    draw.clear();
    startIndex.clear();

    return true;
}

void OpeningInfo::unmapDataFile()  {
    if (!m_mappedBase) return;
    if (m_mappedFile.isOpen()) {
        m_mappedFile.unmap(const_cast<uchar*>(m_mappedBase));
        m_mappedBase = nullptr;
        m_mappedSize = 0;
        m_mappedFile.close();
    } else {
        m_mappedBase = nullptr;
        m_mappedSize = 0;
    }
}

bool OpeningInfo::mapDataFile() {
    if (m_mappedBase) return true;
    if (m_dataFilePath.isEmpty()) return false;
    m_mappedFile.setFileName(m_dataFilePath);
    if (!m_mappedFile.open(QIODevice::ReadOnly)) return false;
    qint64 size = m_mappedFile.size();
    if (size <= 0) return false;
    const uchar* base = m_mappedFile.map(0, size);
    if (!base) {
        m_mappedFile.close();
        return false;
    }
    m_mappedBase = base;
    m_mappedSize = size;
    return true;
}

QPair<PositionWinrate, int> OpeningInfo::getWinrate(const quint64 zobrist)
{
    PositionWinrate winrate = {0, 0, 0};
    if (!m_mappedBase || m_nPositions == 0 || !m_zobristBase) return {winrate, 0};
    const quint64* begin = m_zobristBase;
    const quint64* end = m_zobristBase + m_nPositions;
    const quint64* it = std::lower_bound(begin, end, zobrist);
    if (it == end || *it != zobrist) return {winrate, 0};
    int index = static_cast<int>(it - begin);
    quint64 positionOffset = m_positionInfoStart + static_cast<quint64>(index) * sizeof(OpeningInfo::PositionInfo);
    if (positionOffset + sizeof(OpeningInfo::PositionInfo) <= static_cast<quint64>(m_mappedSize)) {
        const OpeningInfo::PositionInfo* pi = reinterpret_cast<const OpeningInfo::PositionInfo*>(m_mappedBase + positionOffset);
        winrate.whiteWin = static_cast<int>(pi->whiteWin);
        winrate.blackWin = static_cast<int>(pi->blackWin);
        winrate.draw = static_cast<int>(pi->draw);
    }
    return {winrate, index};
}

QVector<quint32> OpeningInfo::readGameIDs(int openingIndex) {
    QVector<quint32> out;
    if (openingIndex < 0) return out;
    if (m_dataFilePath.isEmpty()) return out;

    bool mapped = mapDataFile();
    quint64 posOffset = m_positionInfoStart + static_cast<quint64>(openingIndex) * sizeof(PositionInfo);

    PositionInfo pi;
    if (mapped && m_mappedBase && posOffset + sizeof(PositionInfo) <= static_cast<quint64>(m_mappedSize)) {
        // read from mapped memory
        const PositionInfo* posPtr = reinterpret_cast<const PositionInfo*>(m_mappedBase + posOffset);
        pi = *posPtr; // copies the struct (native endian)
    } else {
        // fallback: open file and read position entry
        QFile f(m_dataFilePath);
        if (!f.open(QIODevice::ReadOnly)) {
            qDebug() << "OpeningInfo::readGameIDs: cannot open" << m_dataFilePath;
            return out;
        }
        if (!f.seek(static_cast<qint64>(posOffset))) {
            qDebug() << "OpeningInfo::readGameIDs: failed to seek to position offset" << posOffset;
            f.close();
            return out;
        }
        QByteArray mb = f.read(sizeof(PositionInfo));
        f.close();
        if (mb.size() != sizeof(PositionInfo)) {
            qDebug() << "OpeningInfo::readGameIDs: short position read";
            return out;
        }
        memcpy(&pi, mb.constData(), sizeof(PositionInfo));
    }

    quint64 startIndex = pi.startIndex;
    quint32 totalCount = pi.insertedCount;
    if (totalCount == 0) return out;
    quint32 toRead = qMin<quint32>(totalCount, static_cast<quint32>(MAX_GAMES_TO_SHOW));
    quint64 byteOffset = m_gameIdsDataStart + startIndex * sizeof(quint32);
    quint64 bytes = static_cast<quint64>(toRead) * sizeof(quint32);

    if (mapped && m_mappedBase && byteOffset + bytes <= static_cast<quint64>(m_mappedSize)) {
        // read directly from mapped gameIDs area
        const quint32* idsPtr = reinterpret_cast<const quint32*>(m_mappedBase + byteOffset);
        out.resize(toRead);
        memcpy(out.data(), idsPtr, bytes);
        return out;
    }

    // fallback to QFile read
    QFile f2(m_dataFilePath);
    if (!f2.open(QIODevice::ReadOnly)) {
        qDebug() << "OpeningInfo::readGameIDs: cannot open data file (fallback)" << m_dataFilePath;
        return out;
    }
    if (!f2.seek(static_cast<qint64>(byteOffset))) {
        qDebug() << "OpeningInfo::readGameIDs: seek failed (fallback) to" << byteOffset;
        f2.close();
        return out;
    }
    QByteArray buf = f2.read(static_cast<qint64>(bytes));
    f2.close();
    if (buf.size() != static_cast<int>(bytes)) {
        qDebug() << "OpeningInfo::readGameIDs: short read (fallback) got" << buf.size() << "expected" << bytes;
        return out;
    }
    out.resize(toRead);
    memcpy(out.data(), buf.constData(), bytes);
    return out;
}

OpeningViewer::OpeningViewer(QWidget *parent)
    : QWidget{parent}
{   
    // load opening book
    mOpeningBookLoaded = mOpeningInfo.deserialize("./opening/openings.bin");

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
    auto [winrate, openingIndex] = mOpeningInfo.getWinrate(zobrist);
    int total = winrate.whiteWin + winrate.blackWin + winrate.draw;

    mStatsLabel->setText(tr("%1 Games").arg(total));
    mMovesList->clear();

    auto legalMoves = position->generateLegalMoves();
    for (const auto [sr, sc, dr, dc, promo]: std::as_const(legalMoves)){
        ChessPosition tempPos;
        tempPos.copyFrom(*position);
        tempPos.applyMove(sr, sc, dr, dc, QChar(promo));
        auto [newWin, _] = mOpeningInfo.getWinrate(tempPos.computeZobrist());
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
        updateGamesList(openingIndex, winrate);
    } else {
        mMovesList->clear();
        mGamesList->setRowCount(0);
        mGamesLabel->setText("Games: 0 of 0 shown");
        mStatsLabel->setText(tr("0 Games"));
    }
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
        qDebug() << "cannot open headers file:" << path;
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

void OpeningViewer::updateGamesList(const int openingIndex, const PositionWinrate winrate)
{
    if (winrate.whiteWin + winrate.blackWin + winrate.draw == 0){
        mGamesList->setRowCount(0);
        mGamesLabel->setText(tr("Games: 0 of 0 shown"));
        return;
    }

    QVector<quint32> gameIDs = mOpeningInfo.readGameIDs(openingIndex);
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
    mGamesLabel->setText(tr("Games: %1 of %2 shown").arg(qMin(gameIDs.size(), MAX_GAMES_TO_SHOW)).arg(winrate.whiteWin + winrate.blackWin + winrate.draw));
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
