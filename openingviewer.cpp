#include "openingviewer.h"
#include "pgngamedata.h"

#include <cstring>
#include <QFile>
#include <QDataStream>
#include <QtGlobal>
#include <QHeaderView>
#include <QPainterPath>
#include <QApplication>

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

void ResultBarDelegate::paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant v = index.data(Qt::UserRole);
    if (!v.isValid() || !v.canConvert<QVariantList>()) {
        QStyledItemDelegate::paint(p, option, index);
        return;
    }
    QVariantList list = v.toList();
    if (list.size() < 3) {
        QStyledItemDelegate::paint(p, option, index);
        return;
    }
    const double whitePct = list[0].toDouble();
    const double drawPct  = list[1].toDouble();
    const double blackPct = list[2].toDouble();

    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    QRect cellRect = opt.rect;
    if (opt.state & QStyle::State_Selected) {
        p->save();
        p->fillRect(cellRect, opt.palette.highlight());
        p->restore();
    }

    QRect inner = cellRect.adjusted(4, 4, -4, -4);
    if (inner.width() <= 0 || inner.height() <= 0) {
        QStyledItemDelegate::paint(p, option, index);
        return;
    }

    p->save();

    int totalW = inner.width();
    int wWhite = qBound(0, int(totalW * whitePct / 100.0), totalW);
    int wDraw  = qBound(0, int(totalW * drawPct  / 100.0), totalW - wWhite);
    int wBlack = totalW - wWhite - wDraw;
    if (wBlack < 0) wBlack = 0;

    int x = inner.left();
    QRect segWhite(x, inner.top(), wWhite, inner.height()); x += wWhite;
    QRect segDraw (x, inner.top(), wDraw,  inner.height()); x += wDraw;
    QRect segBlack(x, inner.top(), wBlack, inner.height());

    const QColor colWhite = QColor(255,255,255);
    const QColor colDraw  = QColor(200,200,200);
    const QColor colBlack = QColor(90,90,90);
    const int radius = qMin(6, inner.height() / 2);

    // draw rounded segments with correct corner handling
    auto drawSegmentRounded = [&](QPainter *p, const QRect &seg, const QColor &c, bool leftRounded, bool rightRounded){
        if (seg.width() <= 0) return;
        p->setPen(Qt::NoPen);
        if (leftRounded || rightRounded) {
            QPainterPath path;
            qreal x1 = seg.left(), x2 = seg.right() + 1.0;
            qreal y1 = seg.top(),  y2 = seg.bottom() + 1.0;
            qreal rad = radius;
            rad = qMin(rad, qMin((double)seg.width()/2.0, (double)seg.height()/2.0));
            path.moveTo(x1 + (leftRounded ? rad : 0), y1);
            path.lineTo(x2 - (rightRounded ? rad : 0), y1);
            if (rightRounded) path.quadTo(x2, y1, x2, y1 + rad);
            path.lineTo(x2, y2 - (rightRounded ? rad : 0));
            if (rightRounded) path.quadTo(x2, y2, x2 - rad, y2);
            path.lineTo(x1 + (leftRounded ? rad : 0), y2);
            if (leftRounded) path.quadTo(x1, y2, x1, y2 - rad);
            path.lineTo(x1, y1 + (leftRounded ? rad : 0));
            if (leftRounded) path.quadTo(x1, y1, x1 + rad, y1);
            p->fillPath(path, c);
        } else {
            p->fillRect(seg, c);
        }
    };

    bool hasWhite = segWhite.width() > 0;
    bool hasDraw  = segDraw.width()  > 0;
    bool hasBlack = segBlack.width() > 0;

    if (hasWhite && !hasDraw && !hasBlack) {
        drawSegmentRounded(p, segWhite, colWhite, true, true);
    } else if (!hasWhite && hasDraw && !hasBlack) {
        drawSegmentRounded(p, segDraw, colDraw, true, true);
    } else if (!hasWhite && !hasDraw && hasBlack) {
        drawSegmentRounded(p, segBlack, colBlack, true, true);
    } else {
        if (hasWhite) drawSegmentRounded(p, segWhite, colWhite, true, false);
        if (hasDraw)  drawSegmentRounded(p, segDraw,  colDraw,  false, false);
        if (hasBlack) drawSegmentRounded(p, segBlack, colBlack, false, true);
        if (!hasWhite && hasDraw && !hasBlack)
            drawSegmentRounded(p, segDraw, colDraw, true, true);
    }

    QFont f = opt.font;
    f.setPointSizeF(f.pointSizeF() - 0.2);
    p->setFont(f);
    QFontMetrics fm(f);
    const int pad = 6;

    auto textColorFor = [&](const QColor &segColor) -> QColor {
        auto luminance = [](const QColor &c) {
            return 0.299*c.red() + 0.587*c.green() + 0.114*c.blue();
        };
        return (luminance(segColor) < 128.0) ? QColor(255,255,255) : QColor(0,0,0);
    };

    if (segWhite.width() > 0) {
        QString t = QString("%1%").arg(int(qRound(whitePct)));
        int tw = fm.horizontalAdvance(t);
        if (tw + pad*2 <= segWhite.width()) {
            QColor tc = textColorFor(colWhite);
            p->setPen(tc);
            int tx = segWhite.left() + pad;
            int ty = segWhite.top() + (segWhite.height() + fm.ascent() - fm.descent())/2;
            p->drawText(tx, ty, t);
        }
    }
    if (segDraw.width() > 0) {
        QString t = QString("%1%").arg(int(qRound(drawPct)));
        int tw = fm.horizontalAdvance(t);
        if (tw + pad*2 <= segDraw.width()) {
            QColor tc = textColorFor(colDraw);
            p->setPen(tc);
            int tx = segDraw.left() + (segDraw.width() - tw) / 2;
            int ty = segDraw.top() + (segDraw.height() + fm.ascent() - fm.descent())/2;
            p->drawText(tx, ty, t);
        }
    }
    if (segBlack.width() > 0) {
        QString t = QString("%1%").arg(int(qRound(blackPct)));
        int tw = fm.horizontalAdvance(t);
        if (tw + pad*2 <= segBlack.width()) {
            QColor tc = textColorFor(colBlack);
            p->setPen(tc);
            int tx = segBlack.left() + segBlack.width() - pad - tw;
            int ty = segBlack.top() + (segBlack.height() + fm.ascent() - fm.descent())/2;
            p->drawText(tx, ty, t);
        }
    }

    p->restore();
}

OpeningViewer::OpeningViewer(QWidget *parent)
    : QWidget{parent}
{
    // load opening book
    mOpeningBookLoaded = mOpeningInfo.deserialize("./opening/openings.bin");

    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(8, 6, 8, 6);
    mainLayout->setSpacing(12);

    // moves list side
    QVBoxLayout* listsLayout = new QVBoxLayout();
    listsLayout->setContentsMargins(0, 0, 0, 0);
    listsLayout->setSpacing(4);

    mPositionLabel = new QLabel("Loading Position...");
    mPositionLabel->setStyleSheet("font-weight: bold; font-size: 13px;");

    mStatsLabel = new QLabel("No position data");
    mStatsLabel->setStyleSheet("font-size: 12px;");

    QWidget* leftHeader = new QWidget(this);
    QVBoxLayout* leftHeaderLayout = new QVBoxLayout(leftHeader);
    leftHeaderLayout->setContentsMargins(0, 0, 0, 0);
    leftHeaderLayout->setSpacing(2);
    leftHeaderLayout->addWidget(mPositionLabel);
    leftHeaderLayout->addWidget(mStatsLabel);
    leftHeader->setLayout(leftHeaderLayout);

    mMovesList = new QTableWidget();
    mMovesList->setColumnCount(3);
    mMovesList->setHorizontalHeaderLabels(QStringList() << "Move" << "Games" << "Win %");
    mMovesList->setAlternatingRowColors(false);
    mMovesList->setShowGrid(false);
    mMovesList->viewport()->setAttribute(Qt::WA_Hover, true);
    mMovesList->setSortingEnabled(true);
    mMovesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    mMovesList->setSelectionMode(QAbstractItemView::SingleSelection);
    mMovesList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMovesList->setMouseTracking(true);
    mMovesList->sortByColumn(1, Qt::DescendingOrder);
    mMovesList->setMinimumHeight(150);
    mMovesList->setItemDelegateForColumn(2, new ResultBarDelegate(this));
    mMovesList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    mMovesList->setFocusPolicy(Qt::NoFocus);
    mMovesList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mMovesList->verticalHeader()->setDefaultSectionSize(10);
    mMovesList->verticalHeader()->setVisible(false);
    mMovesList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    mMovesList->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    mMovesList->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    mMovesList->setColumnWidth(0, 60);
    mMovesList->setColumnWidth(1, 70);
    mMovesList->setCursor(Qt::PointingHandCursor);

    listsLayout->addWidget(leftHeader);
    listsLayout->addWidget(mMovesList);

    QVBoxLayout* gamesLayout = new QVBoxLayout();
    gamesLayout->setContentsMargins(0, 0, 0, 0);
    gamesLayout->setSpacing(2);

    // games list side
    mGamesLabel = new QLabel(tr("Games"));
    mGamesLabel->setStyleSheet("font-weight: bold; font-size: 12px;");

    QWidget* rightHeader = new QWidget(this);
    QVBoxLayout* rightHeaderLayout = new QVBoxLayout(rightHeader);
    rightHeaderLayout->setContentsMargins(0, 0, 0, 0);
    rightHeaderLayout->setSpacing(2);
    rightHeaderLayout->addWidget(mGamesLabel);
    rightHeader->setLayout(rightHeaderLayout);

    mGamesList = new QTableWidget();
    mGamesList->setColumnCount(7);
    mGamesList->setHorizontalHeaderLabels(QStringList() << "White" << "WhiteElo" << "Black" << "BlackElo" << "Result" << "Date" << "Event");
    mGamesList->setAlternatingRowColors(false);
    mGamesList->setShowGrid(false);
    mGamesList->setSelectionBehavior(QAbstractItemView::SelectRows);
    mGamesList->setSelectionMode(QAbstractItemView::SingleSelection);
    mGamesList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mGamesList->setMouseTracking(true);
    mGamesList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    mGamesList->horizontalHeader()->setStretchLastSection(true);
    mGamesList->verticalHeader()->setVisible(false);
    mGamesList->setMinimumHeight(150);
    mGamesList->setMinimumWidth(400);
    mGamesList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mGamesList->verticalHeader()->setDefaultSectionSize(10);
    mGamesList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mGamesList->setFocusPolicy(Qt::NoFocus);
    mGamesList->setCursor(Qt::PointingHandCursor);

    gamesLayout->addWidget(rightHeader);
    gamesLayout->addWidget(mGamesList);

    const int headerHeight = qMax(leftHeader->sizeHint().height(), rightHeader->sizeHint().height());
    leftHeader->setFixedHeight(headerHeight);
    rightHeader->setFixedHeight(headerHeight);
    mainLayout->addLayout(listsLayout);
    mainLayout->addLayout(gamesLayout);
    mainLayout->setStretch(0, 1);
    mainLayout->setStretch(1, 1);
    mPositionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mStatsLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mGamesLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    gamesLayout->setAlignment(mGamesLabel, Qt::AlignLeft | Qt::AlignTop);
    listsLayout->setAlignment(mPositionLabel, Qt::AlignLeft | Qt::AlignTop);

    setLayout(mainLayout);

    // styles
    QString styleSheet = R"(
        QTableWidget {
            background-color: palette(alternate-base);
            border: 1px solid palette(mid);
            border-radius: 3px;
        }
        QTableWidget::item:selected {
            background-color: palette(highlight);
        }
        QTableWidget::item:focus {
            outline: none;
            border: none;
        }
        QTableWidget::item:hover:!selected {
            background-color: transparent;
            border: none;
        }
        QTableWidget::item:selected:focus {
            background-color: palette(highlight);
            border: none;
            outline: none;
        }
    )";

    mMovesList->setStyleSheet(styleSheet);
    mGamesList->setStyleSheet(styleSheet);

    connect(mMovesList, &QTableWidget::itemClicked, this, &OpeningViewer::onNextMoveSelected);
    connect(mGamesList, &QTableWidget::itemDoubleClicked, this, &OpeningViewer::onGameSelected);
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
    mMovesList->setRowCount(0);
    mMovesList->setSortingEnabled(false);

    QString nextNumPrefix;
    int nextMoveNum = (position->getPlyCount())/2 + 1;
    if (position->m_sideToMove == 'w') {
        nextNumPrefix = QString::number(nextMoveNum) + ".";
    } else {
        nextNumPrefix = QString::number(nextMoveNum) + "...";
    }

    auto legalMoves = position->generateLegalMoves();
    for (const auto [sr, sc, dr, dc, promo]: std::as_const(legalMoves)){
        ChessPosition tempPos;
        tempPos.copyFrom(*position);
        tempPos.applyMove(sr, sc, dr, dc, QChar(promo));
        auto [newWin, _] = mOpeningInfo.getWinrate(tempPos.computeZobrist());
        int total = newWin.whiteWin + newWin.blackWin + newWin.draw;
        if (total){
            float whitePct = newWin.whiteWin * 100 / total, blackPct = newWin.blackWin * 100 / total, drawPct = newWin.draw * 100 / total;
            addMoveToList(QString(nextNumPrefix+position->lanToSan(sr, sc, dr, dc, QChar(promo))), total, whitePct, drawPct, blackPct, {sr, sc, dr, dc, promo});
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
        mMovesList->setRowCount(0);
        mGamesList->setRowCount(0);
        mGamesLabel->setText("Games: 0 of 0 shown");
        mStatsLabel->setText(tr("0 Games"));
    }
    mMovesList->setSortingEnabled(true);
    mMovesList->viewport()->update();
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

    qint64 fileSize = f.size();

    for (quint32 gid : ids) {
        PGNGame game;

        if (gid >= static_cast<quint32>(mHeaderOffsets.size())) {
            qDebug() << "Bad game id!" << gid;
            out.append(game);
            continue;
        }

        quint64 off = mHeaderOffsets[gid];
        if (off >= static_cast<quint64>(fileSize)) {
            qWarning() << "Header offset out of range:" << off << "file size:" << fileSize;
            out.append(game);
            continue;
        }

        if (!f.seek(static_cast<qint64>(off))) {
            qWarning() << "Failed to seek header file to offset" << off;
            out.append(game);
            continue;
        }

        // Create QDataStream AFTER the seek so the stream reads from the correct location.
        QDataStream in(&f);
        in.setVersion(QDataStream::Qt_6_5);

        QString white, whiteElo, black, blackElo, event, date, result;
        in >> white >> whiteElo >> black >> blackElo >> event >> date >> result;
        in >> game.bodyText;

        // Very small defensive sanity: if we got nothing, log the offset & stream status
        if (white.isEmpty() && black.isEmpty() && game.bodyText.isEmpty()) {
            qWarning() << "Empty header read at offset" << off << "for gid" << gid << "stream status:" << in.status();
            // read a few raw bytes around the offset for debugging:
            const qint64 dbgN = qMin<qint64>(256, fileSize - static_cast<qint64>(off));
            if (dbgN > 0) {
                f.seek(static_cast<qint64>(off));
                QByteArray dbg = f.read(dbgN);
                qDebug() << "raw bytes:" << dbg.toHex().left(200);
            }
        }

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
        QTableWidgetItem* whiteEloItem = new QTableWidgetItem(whiteElo.toInt());
        whiteEloItem->setData(Qt::DisplayRole, whiteElo.toInt());
        QTableWidgetItem* blackEloItem = new QTableWidgetItem(blackElo.toInt());
        blackEloItem->setData(Qt::DisplayRole, blackElo.toInt());
        int row = mGamesList->rowCount();
        mGamesList->insertRow(row);
        mGamesList->setItem(row, 0, new QTableWidgetItem(white));
        mGamesList->setItem(row, 1, whiteEloItem);
        mGamesList->setItem(row, 2, new QTableWidgetItem(black));
        mGamesList->setItem(row, 3, blackEloItem);
        mGamesList->setItem(row, 4, new QTableWidgetItem(result));
        mGamesList->setItem(row, 5, new QTableWidgetItem(date));
        mGamesList->setItem(row, 6, new QTableWidgetItem(event));
        mGamesList->item(row, 0)->setData(Qt::UserRole, gameIDs[i]);
    }

    mGamesList->setSortingEnabled(true);
    mGamesLabel->setText(tr("Games: %1 of %2 shown").arg(qMin(gameIDs.size(), MAX_GAMES_TO_SHOW)).arg(winrate.whiteWin + winrate.blackWin + winrate.draw));
}

// helper
void OpeningViewer::addMoveToList(const QString& move, int games, float whitePct, float drawPct, float blackPct, SimpleMove moveData)
{
    int row = mMovesList->rowCount();
    mMovesList->insertRow(row);

    QTableWidgetItem* moveItem = new QTableWidgetItem(move);
    moveItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    moveItem->setData(Qt::UserRole, QVariant::fromValue(moveData));
    mMovesList->setItem(row, 0, moveItem);

    QTableWidgetItem* gamesItem = new QTableWidgetItem(games);
    gamesItem->setData(Qt::DisplayRole, QVariant(static_cast<qint64>(games))); // numeric sort key
    gamesItem->setData(Qt::UserRole, games);
    gamesItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    mMovesList->setItem(row, 1, gamesItem);

    // Percent column: also store list for the delegate and a numeric display role for sorting
    QString percentText = QString("%1% / %2% / %3%").arg(whitePct, 0, 'f', 1).arg(drawPct, 0, 'f', 1).arg(blackPct, 0, 'f', 1);
    QTableWidgetItem* pctItem = new QTableWidgetItem(percentText);
    pctItem->setData(Qt::DisplayRole, QVariant(static_cast<double>(whitePct))); // primary sort key = whitePct
    QVariantList triple;
    triple << QVariant::fromValue(static_cast<double>(whitePct)) << QVariant::fromValue(static_cast<double>(drawPct)) << QVariant::fromValue(static_cast<double>(blackPct));
    pctItem->setData(Qt::UserRole, triple); // used by ResultBarDelegate
    pctItem->setToolTip(percentText);
    pctItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    mMovesList->setItem(row, 2, pctItem);

    // keep row height consistent when adding many rows (if desired)
    mMovesList->setRowHeight(row, mMovesList->verticalHeader()->defaultSectionSize());
}

void OpeningViewer::onNextMoveSelected(QTableWidgetItem* item)
{
    if (!item) return;
    int row = item->row();
    if (row < 0) return;
    QTableWidgetItem* first = mMovesList->item(row, 0);
    if (!first) return;
    QVariant v = first->data(Qt::UserRole);
    if (!v.isValid()) return;
    SimpleMove moveData = v.value<SimpleMove>();
    emit moveClicked(moveData);
}

void OpeningViewer::onGameSelected(QTableWidgetItem* item)
{
    // emit gameSelected(gameId);
}
