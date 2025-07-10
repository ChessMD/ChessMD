/*
March 20, 2025: File Creation
*/

#include "chessposition.h"

#include <QRegularExpression>
#include <QDebug>

ChessPosition::ChessPosition(QObject *parent)
    : QObject(parent)
{
    m_boardData = convertFenToBoardData("");
    m_plyCount = 0;
    m_sideToMove = 'w';

    m_castling.whiteKing = true;
    m_castling.whiteQueen = true;
    m_castling.blackKing = true;
    m_castling.blackQueen = true;

    m_enPassantTarget = '-';
    m_halfmoveClock = 0;
    m_fullmoveNumber = 1;
}

QVector<QVector<QString>> ChessPosition::boardData() const
{
    return m_boardData;
}

void ChessPosition::copyFrom(const ChessPosition &other)
{
    m_boardData = other.m_boardData;
    m_sideToMove = other.m_sideToMove;
    m_castling.whiteKing = other.m_castling.whiteKing;
    m_castling.blackKing = other.m_castling.blackKing;
    m_castling.whiteQueen = other.m_castling.whiteQueen;
    m_castling.blackQueen = other.m_castling.blackQueen;
    m_enPassantTarget = other.m_enPassantTarget;
    m_halfmoveClock = other.m_halfmoveClock;
    m_fullmoveNumber = other.m_fullmoveNumber;
    m_plyCount = other.m_plyCount;
    m_lastMove = other.m_lastMove;
    m_evalScore = other.m_evalScore;
}

bool ChessPosition::validateMove(int oldRow, int oldCol, int newRow, int newCol) const
{
    if (oldRow < 0 || oldRow >= 8 || oldCol < 0 || oldCol >= 8 ||
        newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8 ||
        (oldRow == newRow && oldCol == newCol))
    {
        return false;
    }

    const QString &fromSq = m_boardData[oldRow][oldCol];
    if (fromSq.isEmpty())
        return false;

    QChar color = fromSq[0];
    QChar piece = fromSq[1];

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
        for (int i = 0; i <= 2; i++){
            if (squareAttacked(oldRow, oldCol + step*i, (color=='w'?'b':'w'))) {
                return false;
            }
        }

        return true;
    }

    ChessPosition* temp = new ChessPosition;
    temp->copyFrom(*this);
    temp->applyMove(oldRow, oldCol, newRow, newCol, '\0');
    if (temp->inCheck(color))
        return false;

    switch (piece.toLatin1()) {
    case 'P': {
        int dir = (color == 'w' ? -1 : 1);
        // single
        if (dc == 0 && dr == dir && dstSq.isEmpty())
            return true;
        // double from start
        if (dc == 0 && dr == 2*dir && oldRow == (color=='w'?6:1) && dstSq.isEmpty() && m_boardData[oldRow+dir][oldCol].isEmpty())
            return true;
        // capture
        if (adc == 1 && adr == 1 && dr == dir && (capture || m_enPassantTarget == QString(QChar('a'+newCol))+QString::number(8-newRow)))
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

bool ChessPosition::squareAttacked(int row, int col, QChar attacker) const
{
    // Pawn attacks
    int dir = (attacker == 'w' ? 1 : -1);
    for (int dc : {-1, 1}) {
        int r = row + dir;
        int c = col + dc;
        if (r>=0 && r<8 && c>=0 && c<8 && m_boardData[r][c] == QString(attacker) + 'P')
            return true;
    }
    // Knight attacks
    static const QVector<QPair<int,int>> knightMoves = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
    for (auto &m : knightMoves) {
        int r = row + m.first, c = col + m.second;
        if (r>=0 && r<8 && c>=0 && c<8 && m_boardData[r][c] == QString(attacker) + 'N')
            return true;
    }
    // Sliding pieces & king
    static const QVector<QPair<int,int>> dirs = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto &d : dirs) {
        int r = row + d.first, c = col + d.second;
        int dist = 1;
        while (r>=0 && r<8 && c>=0 && c<8) {
            QString sq = m_boardData[r][c];
            if (!sq.isEmpty()) {
                if (sq[0] == attacker) {
                    char p = sq[1].toLatin1();
                    // adjacent king
                    if (dist==1 && p=='K') return true;
                    // rook/queen
                    if ((d.first==0||d.second==0) && (p=='R'||p=='Q')) return true;
                    // bishop/queen
                    if ((d.first!=0&&d.second!=0) && (p=='B'||p=='Q')) return true;
                }
                break;
            }
            r += d.first; c += d.second; dist++;
        }
    }
    return false;
}

void ChessPosition::buildUserMove(int sr, int sc, int dr, int dc, QChar promo)
{
    ChessPosition* newPos = new ChessPosition;
    newPos->copyFrom(*this);
    newPos->applyMove(sr, sc, dr, dc, promo);

    QString moveText;
    if (newPos->m_boardData[dr][dc][1] != 'P'){
        moveText = QString("%1").arg(newPos->m_boardData[dr][dc][1]);
    }
    moveText += QString("%1%2").arg(QChar('a' + dc)).arg(8 - dr);
    if (promo != '\0') {
        moveText += "=" + QString(promo);
    }

    QSharedPointer<NotationMove> newMove(new NotationMove(moveText, *newPos));

    emit moveMade(newMove);
    emit boardDataChanged();
}

void ChessPosition::release(int oldRow, int oldCol, int newRow, int newCol)
{
    if (!validateMove(oldRow, oldCol, newRow, newCol)){
        return;
    }

    if (m_boardData[oldRow][oldCol][1] == 'P' && (newRow == 0 || newRow == 7)){
        emit requestPromotion(oldRow, oldCol, newRow, newCol);
    } else {
        buildUserMove(oldRow, oldCol, newRow, newCol, '\0');
    }
}

void ChessPosition::promote(int sr, int sc, int dr, int dc, QChar promo)
{
    buildUserMove(sr, sc, dr, dc, promo);
}

void ChessPosition::setBoardData(const QVector<QVector<QString>> &data)
{
    if (m_boardData != data) {
        m_boardData = data;
        emit boardDataChanged();
    }
}

bool ChessPosition::tryMakeMove(QString san) {
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
        if (color=='w') { m_castling.whiteKing=m_castling.whiteQueen=false; }
        else { m_castling.blackKing=m_castling.blackQueen=false; }
        m_halfmoveClock++;
        if (m_sideToMove=='b') m_fullmoveNumber++;
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
        if (validateMove(sr, sc, dr, dc)) {
            applyMove(sr, sc, dr, dc, promo);
            m_halfmoveClock = (piece=='P' || capture)?0:m_halfmoveClock+1;
            if (m_sideToMove == 'b') m_fullmoveNumber++;
            auto moveObj = QSharedPointer<NotationMove>::create(san, *this);
            return true;
        }
    }
    return false;
}

void ChessPosition::applyMove(int sr, int sc, int dr, int dc, QChar promotion) {
    QString from = m_boardData[sr][sc];
    if (from.size() < 2) {
        qWarning() << "Invalid ‘from’ string, aborting move.";
        return;
    }

    // Update castling rights if king or rook moves
    if (from[1]=='K') {
        if (from[0]=='w') { m_castling.whiteKing=m_castling.whiteQueen=false; }
        else { m_castling.blackKing=m_castling.blackQueen=false; }
        // Rook relocation during castling
        if (sc-dc == 2){
            m_boardData[sr][sc-1] = m_boardData[sr][0];
            m_boardData[sr][0] = "";
        }
        if (sc-dc == -2){
            m_boardData[sr][sc+1] = m_boardData[sr][7];
            m_boardData[sr][7] = "";
        }
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

    // En passant capture
    if (from[1]=='P' && qAbs(sc-dc)==1 && m_boardData[dr][dc].isEmpty()) {
        QString dest = QString(QChar('a'+dc)) + QString::number(8 - dr);
        if (m_enPassantTarget == dest) {
            m_boardData[sr][dc] = "";
        }
    }

    // Perform move
    m_boardData[dr][dc] = from;
    m_boardData[sr][sc] = "";

    // Pawn double move: set en passant target
    if (from[1]=='P' && qAbs(dr-sr)==2) {
        int epRow = (sr+dr)/2;
        m_enPassantTarget = QString(QChar('a'+sc)) + QString::number(8-epRow);
    } else {
        m_enPassantTarget = '-';
    }
    // Promotion
    if (promotion!=QChar('\0') && from[1]=='P' && (dr==0||dr==7)) {
        m_boardData[dr][dc] = QString(from[0]) + promotion.toUpper();
    }
    m_sideToMove = (m_sideToMove=='w'?'b':'w');
    m_plyCount++;
    m_lastMove  = ((sr * 8 + sc) << 8) | (dr * 8 + dc);
}

bool ChessPosition::inCheck(QChar side) const
{
    // Find king
    for (int r=0; r<8; r++) {
        for (int c=0; c<8; c++) {
            if (m_boardData[r][c] == QString(side) + 'K') {
                return squareAttacked(r, c, (side=='w'?'b':'w'));
            }
        }
    }
    return false;
}

bool ChessPosition::canCastleKingside(QChar side) const
{
    return (side == 'w' ? m_castling.whiteKing : m_castling.blackKing);
}
bool ChessPosition::canCastleQueenside(QChar side) const
{
    return (side == 'w' ? m_castling.whiteQueen : m_castling.blackQueen);
}

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
        token.remove(QRegularExpression(R"(^\d+\.+)"));

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
            // qDebug() << comment;
            if (!comment.isEmpty()) {
                parentMove->commentAfter = comment;
                comment.clear();
            }
            continue;
        }

        while (variationIdx < varNode->variations.size() && varNode->variations[variationIdx].first == i)
        {
            auto subNode = varNode->variations[variationIdx].second;
            buildNotationTree(subNode,  parentMove->m_previousMove);
            variationIdx++;
        }

        ChessPosition *clonePos = new ChessPosition;
        clonePos->copyFrom(*parentMove->m_position);

        if (!clonePos->tryMakeMove(token)) {
            parentMove->commentAfter += token;
            // qDebug() << "Illegal move skipped:" << token;
            delete clonePos;
            continue;
        }

        auto childMove = QSharedPointer<NotationMove>::create(token, *clonePos);
        linkMoves(parentMove, childMove);

        parentMove = childMove;
    }

    while (variationIdx < varNode->variations.size()) {
        auto subNode = varNode->variations[variationIdx].second;
        buildNotationTree(subNode, parentMove);
        ++variationIdx;
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

QString ChessPosition::lanToSan(int sr, int sc, int dr, int dc, QChar promo) const
{
    // 1) Piece identity
    const QString fromSq = m_boardData[sr][sc];
    if (fromSq.size() < 2) {
        qWarning() << "Invalid ‘from’ string, aborting move.";
        return "";
    }
    QChar piece = fromSq[1];  // 'P','N','B','R','Q','K'
    bool isPawn = (piece == 'P');

    // 2) Castling?
    if (piece == 'K' && qAbs(dc - sc) == 2) {
        return (dc > sc ? "O-O" : "O-O-O");
    }

    // 3) Detect capture (including en passant)
    bool isCapture = !m_boardData[dr][dc].isEmpty() || (isPawn && qAbs(dc - sc)==1 && m_enPassantTarget == QString(QChar('a'+dc)) + QString::number(8-dr));

    // 4) Find all same‐type pieces that *could* move to (dr,dc)
    QVector<QPair<int,int>> candidates;
    QChar mySide = m_sideToMove;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const QString &sq = m_boardData[r][c];
            if (sq.size()==2 && sq[0]==mySide && sq[1]==piece) {
                if (!(r==sr && c==sc) && validateMove(r,c,dr,dc)) {
                    candidates.append({r,c});
                }
            }
        }
    }
    // always include the actual mover
    candidates.append({sr,sc});

    // 5) Disambiguator if more than one candidate (and not pawn)
    QString disamb;
    if (!isPawn && candidates.size() > 1) {
        bool needFile=false, needRank=false;
        for (auto &o : candidates) {
            if (o.second != sc) needFile = true;
            if (o.first  != sr) needRank = true;
        }
        if (needFile) disamb += QChar('a' + sc);
        if (needRank) disamb += QChar('1' + (7 - sr));
    }

    // 6) Build SAN prefix
    QString san;
    if (!isPawn) {
        san += piece;    // N/B/R/Q
        san += disamb;
    } else if (isCapture) {
        // pawn captures include file
        san += QChar('a' + sc);
    }

    // 7) Capture marker and destination
    if (isCapture) san += 'x';
    san += QChar('a' + dc);
    san += QString::number(8 - dr);

    // 8) Promotion?
    if (promo != QChar('\0')) {
        san += '=';
        san += promo.toUpper();
    }

    return san;
}


QSharedPointer<NotationMove> parseEngineLine(const QString& line, QSharedPointer<NotationMove> startMove)
{
    QSharedPointer<NotationMove> newMove, tempMove = startMove, rootMove;
    QString token;
    int root = 1;
    for (int i = 0; i < line.length(); i++){
        if (line[i] != ' '){
            token += line[i];
        }
        if (line[i] == ' ' || i+1 == line.length()){
            if (token.size() < 4 || token.size() > 5)
                continue;
            int sc = token.at(0).toLatin1() - 'a', sr = 8 - token.at(1).digitValue();
            int dc = token.at(2).toLatin1() - 'a', dr = 8 - token.at(3).digitValue();
            QChar promo = (token.length() == 5 ? token.at(4) : QChar('\0'));
            ChessPosition* clonePos = new ChessPosition;
            clonePos->copyFrom(*tempMove->m_position);
            if (!clonePos->validateMove(sr, sc, dr, dc)) break; // error parsing
            clonePos->applyMove(sr, sc, dr, dc, promo);
            newMove = QSharedPointer<NotationMove>::create(token, *clonePos);
            newMove->moveText = tempMove->m_position->lanToSan(sr, sc, dr, dc, promo);
            if (!root){
                linkMoves(tempMove, newMove);
            } else {
                rootMove = newMove;
            }
            root = 0;
            token = "";
            tempMove = newMove;
        }
    }
    return rootMove;
}
