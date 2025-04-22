#include "chessposition.h"

#include <QRegularExpression>
#include <QDebug>

ChessPosition::ChessPosition(QObject *parent)
    : QObject(parent)
{
    m_boardData = convertFenToBoardData("");
    plyCount = 0;
    m_sideToMove = 'w';

    m_castling.whiteKing = true;
    m_castling.whiteQueen = true;
    m_castling.blackKing = true;
    m_castling.blackQueen = true;

    m_enPassantTarget.clear();
    m_halfmoveClock = 0;
    m_fullmoveNumber = 1;
}

QVector<QVector<QString>> ChessPosition::boardData() const
{
    return m_boardData;
}

void ChessPosition::copyFrom(const ChessPosition &other) {
    setBoardData(other.m_boardData);
    m_sideToMove = other.m_sideToMove;
    m_castling = other.m_castling;
    m_enPassantTarget = other.m_enPassantTarget;
    m_halfmoveClock = other.m_halfmoveClock;
    m_fullmoveNumber = other.m_fullmoveNumber;
    plyCount = other.plyCount;
}

bool ChessPosition::validateMove(int oldRow, int oldCol, int newRow, int newCol) const {
    if (oldRow < 0 || oldRow >= 8 || oldCol < 0 || oldCol >= 8 ||
        newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8 ||
        (oldRow == newRow && oldCol == newCol))
    {
        return false;
    }

    const QString &fromSq = m_boardData[oldRow][oldCol];
    if (fromSq.isEmpty())
        return false;

    QChar color = fromSq[0];       // 'w' or 'b'
    QChar piece = fromSq[1];       // 'P','R','N','B','Q','K'

    // Ensure moving the right color
    if (color.toLatin1() != m_sideToMove)
        return false;

    const QString &dstSq = m_boardData[newRow][newCol];
    bool capture = !dstSq.isEmpty();
    if (capture && dstSq[0] == color)
        return false;

    int dr = newRow - oldRow;
    int dc = newCol - oldCol;
    int adr = qAbs(dr), adc = qAbs(dc);

    if (piece == 'K' && oldRow == (color=='w'?7:0) && dr == 0 && adc == 2) {
        bool kingSide = (dc == 2);
        if (!(kingSide ? canCastleKingside(color) : canCastleQueenside(color))) return false;
        // Path clear
        int step = kingSide ? 1 : -1;
        int rookCol = kingSide ? 7 : 0;
        for (int c = oldCol + step; c != rookCol; c += step) {
            if (m_boardData[oldRow][c] != "") {
                return false;
            }
        }

        // // Not in check and no traverse through check
        // if (inCheck(color)) return false;
        // // Check intermediate and destination
        // for (int i = 1; i <= 2; ++i) {
        //     int c = oldCol + step * i;
        //     if (squareAttacked(oldRow, c, (color=='w'?'b':'w'))) return false;
        // }
        return true;
    }

    switch (piece.toLatin1()) {
        case 'P': {
            int dir = (color == 'w' ? -1 : 1);
            // Single
            if (dc == 0 && dr == dir && dstSq.isEmpty())
                return true;
            // Double from start
            if (dc == 0 && dr == 2*dir && oldRow == (color=='w'?6:1) && dstSq.isEmpty() && m_boardData[oldRow+dir][oldCol].isEmpty())
                return true;
            // Capture
            if (adr == 1 && dr == dir && (capture || m_enPassantTarget == QString(QChar('a'+newCol))+QString::number(8-newRow)))
                return true;
            return false;
        }
        case 'R': {
            if ((dr == 0 && adc > 0) || (dc == 0 && adr > 0)) {
                int stepR = (dr == 0 ? 0 : dr/adr);
                int stepC = (dc == 0 ? 0 : dc/adc);
                int r = oldRow + stepR, c = oldCol + stepC;
                while (r != newRow || c != newCol) {
                    if (!m_boardData[r][c].isEmpty()) return false;
                    r += stepR; c += stepC;
                }
                return true;
            }
            return false;
        }
        case 'N': {
            return ( (adr==2 && adc==1) || (adr==1 && adc==2) );
        }
        case 'B': {
            if (adr == adc && adr > 0) {
                int stepR = dr/adr, stepC = dc/adc;
                int r = oldRow + stepR, c = oldCol + stepC;
                while (r != newRow) {
                    if (!m_boardData[r][c].isEmpty()) return false;
                    r += stepR; c += stepC;
                }
                return true;
            }
            return false;
        }
        case 'Q': {
            if ((adr == adc && adr > 0) || (dr == 0 && adc > 0) || (dc == 0 && adr > 0)) {
                int stepR = (dr==0?0:dr/adr);
                int stepC = (dc==0?0:dc/adc);
                int r = oldRow + stepR, c = oldCol + stepC;
                while (r != newRow || c != newCol) {
                    if (!m_boardData[r][c].isEmpty()) return false;
                    r += stepR; c += stepC;
                }
                return true;
            }
            return false;
        }
        case 'K': {
            // Normal king move: one square any direction
            if (adr <= 1 && adc <= 1)
                return true;
            return false;
        }
        default:
            return false;
    }
}

void ChessPosition::release(int oldRow, int oldCol, int newRow, int newCol)
{
    if (!validateMove(oldRow, oldCol, newRow, newCol)){
        return;
    }

    ChessPosition* newPos = new ChessPosition;
    newPos->m_boardData = m_boardData;
    newPos->plyCount = plyCount+1;
    newPos->m_boardData[newRow][newCol] = m_boardData[oldRow][oldCol];
    newPos->m_boardData[oldRow][oldCol] = "";  // Clear the old position

    QString moveText = (plyCount % 2 == 0 ? QString::number(1+(plyCount/2)) + "." : "");
    if (newPos->m_boardData[newRow][newCol][1] != 'P'){
        moveText += QString("%3%1%2").arg(QChar('a' + newCol)).arg(8 - newRow).arg(newPos->m_boardData[newRow][newCol][1]);
    } else {
        moveText += QString("%1%2").arg(QChar('a' + newCol)).arg(8 - newRow);
    }

    QSharedPointer<NotationMove> newMove(new NotationMove(moveText, *this));
    newMove->m_position = newPos;

    // Notify QML
    emit moveMade(newMove);
    emit boardDataChanged();
}

void ChessPosition::setBoardData(const QVector<QVector<QString>> &data)
{
    if (m_boardData != data) {
        m_boardData = data;
        emit boardDataChanged();
    }
}

QVector<QVector<QString>> convertFenToBoardData(const QString &fen)
{
    QString effectiveFen = fen.trimmed().isEmpty()
    ? "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    : fen;

    QVector<QVector<QString>> boardData;
    QStringList parts = effectiveFen.split(" ");
    if (parts.isEmpty())
        return boardData;

    QString position = parts[0];
    QStringList rows = position.split("/");
    if (rows.size() != 8)
        return boardData;  // Ensure it's a valid board representation

    for (const QString &row : rows) {
        QVector<QString> rowData;
        for (int i = 0; i < row.length(); ++i) {
            QChar ch = row[i];
            if (ch.isDigit()) {
                int emptyCount = ch.digitValue();
                for (int j = 0; j < emptyCount; ++j)
                    rowData.append("");
            } else {
                QString piece;
                if (ch.isUpper())
                    piece = "w" + QString(ch);
                else
                    piece = "b" + QString(ch).toUpper();
                rowData.append(piece);
            }
        }
        // Ensure that every row has exactly 8 columns
        while (rowData.size() < 8)
            rowData.append("");
        boardData.append(rowData);
    }

    return boardData;
}

QString ChessPosition::positionToFEN() const {
    QStringList rowStr;
    for (auto &row : m_boardData) {
        QString s;
        int empties = 0;
        for (QString sq : row) {
            if (sq.isEmpty()) { empties++; }
            else {
                if (empties) { s += QString::number(empties); empties = 0; }
                QChar p = sq[1];
                s += (sq[0] == 'w' ? p : p.toLower());
            }
        }
        if (empties) s += QString::number(empties);
        rowStr.append(s);
    }
    QString boardPart = rowStr.join('/');
    QString sidePart = (m_sideToMove == 'w' ? "w" : "b");
    QString cr;
    if (m_castling.whiteKing)  cr += 'K';
    if (m_castling.whiteQueen) cr += 'Q';
    if (m_castling.blackKing)  cr += 'k';
    if (m_castling.blackQueen) cr += 'q';
    if (cr.isEmpty()) cr = "-";
    QString ep = m_enPassantTarget;
    QString hm = QString::number(m_halfmoveClock);
    QString fm = QString::number(m_fullmoveNumber);
    return QString("%1 %2 %3 %4 %5 %6").arg(boardPart, sidePart, cr, ep, hm, fm);
}

bool ChessPosition::makeMove(QString san) {
    // Handle castling
    if (san == "O-O" || san == "O-O-O") {
        bool kingSide = (san == "O-O");
        QChar color = m_sideToMove;
        int row = (color=='w'?7:0);
        int oldKC = 4;
        int newKC = kingSide?6:2;
        int oldRC = kingSide?7:0;
        int newRC = kingSide?5:3;
        // Validate
        if (!validateMove(row, oldKC, row, newKC)) return false;

        applyMove(row, oldKC, row, newKC, '\0');
        applyMove(row, oldRC, row, newRC, '\0');
        if (color=='w') { m_castling.whiteKing=m_castling.whiteQueen=false; }
        else { m_castling.blackKing=m_castling.blackQueen=false; }
        m_halfmoveClock++;
        if (m_sideToMove=='b') m_fullmoveNumber++;
        m_sideToMove = (m_sideToMove=='w'?'b':'w');
        plyCount++;
        emit boardDataChanged();
        return true;
    }
    QRegularExpression re(R"(^([NBRQK]?)([a-h]?)([1-8]?)(x?)([a-h][1-8])(=?[QNRB]?)([+#]?)$)");
    auto m = re.match(san);
    if (!m.hasMatch()) return false;
    QChar piece = m.captured(1).isEmpty() ? 'P' : m.captured(1)[0];
    QString disFile  = m.captured(2);
    QString disRank  = m.captured(3);
    bool capture    = !m.captured(4).isEmpty();
    QString dst      = m.captured(5);
    QString promoStr = m.captured(6);
    QChar promo      = promoStr.startsWith('=') ? promoStr[1] : '\0';

    // Find all origins for this piece that can move to dst
    auto origins = findPieceOrigins(piece, dst, disFile+disRank);
    for (auto &o : origins) {
        int sr = o.first, sc = o.second;
        int dr = 8 - dst[1].digitValue(), dc = dst[0].unicode() - 'a';
        if (isLegalPseudo(sr, sc, dr, dc, promo)) {
            if (!applyMove(sr, sc, dr, dc, promo)) continue;
            m_halfmoveClock = (piece=='P' || capture)?0:m_halfmoveClock+1;
            if (m_sideToMove == 'b') m_fullmoveNumber++;
            m_sideToMove = (m_sideToMove=='w'?'b':'w');
            plyCount++;
            // Emit the NotationMove for UI
            auto moveObj = QSharedPointer<NotationMove>::create(san, *this);
            // emit moveMade(moveObj);
            emit boardDataChanged();
            return true;
        }
    }
    return false;
}

bool ChessPosition::applyMove(int sr, int sc, int dr, int dc, QChar promotion) {
    QString from = m_boardData[sr][sc];
    if (from.isEmpty()) return false;
    // Update castling rights if king or rook moves
    if (from[1]=='K') {
        if (from[0]=='w') { m_castling.whiteKing=m_castling.whiteQueen=false; }
        else { m_castling.blackKing=m_castling.blackQueen=false; }
    } else if (from[1]=='R') {
        if (sr==7 && sc==0) m_castling.whiteQueen=false;
        if (sr==7 && sc==7) m_castling.whiteKing=false;
        if (sr==0 && sc==0) m_castling.blackQueen=false;
        if (sr==0 && sc==7) m_castling.blackKing=false;
    }
    // Capture affects opponent rook rights
    QString target = m_boardData[dr][dc];
    if (!target.isEmpty() && target[1]=='R') {
        if (dr==7 && dc==0) m_castling.whiteQueen=false;
        if (dr==7 && dc==7) m_castling.whiteKing=false;
        if (dr==0 && dc==0) m_castling.blackQueen=false;
        if (dr==0 && dc==7) m_castling.blackKing=false;
    }
    // Move
    m_boardData[dr][dc] = from;
    m_boardData[sr][sc] = "";
    // Pawn double move: set en passant target
    if (from[1]=='P' && qAbs(dr-sr)==2) {
        int epRow = (sr+dr)/2;
        m_enPassantTarget = QString(QChar('a'+sc)) + QString::number(8-epRow);
    } else {
        m_enPassantTarget.clear();
    }
    // Promotion
    if (promotion!=QChar('\0') && from[1]=='P' && (dr==0||dr==7)) {
        m_boardData[dr][dc] = QString(from[0]) + promotion;
    }
    return true;
}

bool ChessPosition::isLegalPseudo(int sr, int sc, int dr, int dc, QChar promo) const {
    if (!validateMove(sr, sc, dr, dc)) return false;
    return true;
}

bool ChessPosition::inCheck(QChar side) const { return false; }
bool ChessPosition::canCastleKingside(QChar side) const { return true; }
bool ChessPosition::canCastleQueenside(QChar side) const { return true; }

QVector<QPair<int,int>> ChessPosition::findPieceOrigins(QChar piece, const QString &dest, const QString &disamb) const {
    QVector<QPair<int,int>> vec;
    int tgtRow = dest[1].digitValue();
    int tgtCol = dest[0].unicode() - 'a';
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            QString sq = m_boardData[r][c];
            if (sq.isEmpty()) continue;
            if (sq[1] == piece && sq[0] == m_sideToMove) {
                // Check file/rank disambiguation
                if (!disamb.isEmpty()) {
                    if (disamb[0].isLetter() && c != disamb[0].unicode()-'a') continue;
                    if (disamb[0].isDigit() && r != disamb[0].digitValue()) continue;
                }
                vec.append(qMakePair(r,c));
            }
        }
    }
    return vec;
}


void buildNotationTree(const QSharedPointer<VariationNode> varNode, QSharedPointer<NotationMove> parentMove)
{
    int plyCount = varNode->plyCount;
    int variationIdx = 0;
    QString comment;

    for (int i = 0; i < plyCount; ++i) {
        QString token = varNode->moves[i];

        if (token.startsWith('{')) {
            // strip leading '{'
            token.remove(0, 1);
            while (!token.endsWith('}')) {
                comment += token + " ";
                i++;
                if (i >= plyCount) break;
                token = varNode->moves[i];
            }
            // strip trailing '}'
            if (token.endsWith('}')) {
                token.chop(1);
                comment += token;
            }
            continue;
        }

        while (variationIdx < varNode->variations.size() && varNode->variations[variationIdx].first == i)
        {
            qDebug() << "entering variation!!" << parentMove->moveText << parentMove->m_previousMove->moveText;
            auto subNode = varNode->variations[variationIdx].second;
            buildNotationTree(subNode,  parentMove->m_previousMove);
            variationIdx++;
        }

        qDebug() << "Attempting SAN:" << token;

        ChessPosition *clonePos = new ChessPosition;
        clonePos->copyFrom(*parentMove->m_position);

        if (!clonePos->makeMove(token)) {
            // parentMove->commentAfter += token;
            qDebug() << "Illegal move skipped:" << token;
            delete clonePos;
            continue;
        }

        auto childMove = QSharedPointer<NotationMove>::create(token, *clonePos);
        linkMoves(parentMove, childMove);
        qDebug() << parentMove->moveText << childMove->moveText;

        if (!comment.isEmpty()) {
            parentMove->commentAfter = comment;
            comment.clear();
        }

        parentMove = childMove;
    }

    while (variationIdx < varNode->variations.size()) {
        auto subNode = varNode->variations[variationIdx].second;
        buildNotationTree(subNode, parentMove);
        ++variationIdx;
    }
}
