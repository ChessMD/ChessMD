#include "chessposition.h"

#include "notation.h"

#include <QDebug>

ChessPosition::ChessPosition(QObject *parent)
    : QObject(parent)
{
    // Initialize with default starting position (optional)
    m_boardData = {
        QVector<QString>{"bR", "bN", "bB", "bQ", "bK", "bB", "bN", "bR"},
        QVector<QString>{"bP", "bP", "bP", "bP", "bP", "bP", "bP", "bP"},
        QVector<QString>{"", "", "", "", "", "", "", ""},
        QVector<QString>{"", "", "", "", "", "", "", ""},
        QVector<QString>{"", "", "", "", "", "", "", ""},
        QVector<QString>{"", "", "", "", "", "", "", ""},
        QVector<QString>{"wP", "wP", "wP", "wP", "wP", "wP", "wP", "wP"},
        QVector<QString>{"wR", "wN", "wB", "wQ", "wK", "wB", "wN", "wR"}
    };
    plyCount = 0;
}

QVector<QVector<QString>> ChessPosition::boardData() const
{
    return m_boardData;
}

bool ChessPosition::validateMove(int oldRow, int oldCol, int newRow, int newCol){
    // Movement and boundary check
    if (oldRow < 0 || oldRow >= 8 || oldCol < 0 || oldCol >= 8 ||
        newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8 ||
        (oldRow == newRow && oldCol == newCol))
    {
        return false;
    }

    if (m_boardData[oldRow][oldCol].isEmpty()){
        return false;
    }

    QChar color = m_boardData[oldRow][oldCol][0]; // 'w' or 'b'
    QChar piece = m_boardData[oldRow][oldCol][1];  // 'P', 'R', 'N', etc.

    // Move parity check
    if ((color == "b" && plyCount % 2 == 0) || (color == "w" && plyCount % 2 == 1)){
        return false;
    }

    // Piece color parity check
    const QString &dst = m_boardData[newRow][newCol];
    if (dst.size() >= 2 && dst[0] == color){
        return false;
    }

    int dr = newRow - oldRow, dc = newCol - oldCol;
    int adr = std::abs(dr), adc = std::abs(dc);

    // helper to check that every square between src and dst is empty
    auto pathClear = [&](int stepR, int stepC) {
        int r = oldRow + stepR, c = oldCol + stepC;
        while (r != newRow || c != newCol) {
            if (!m_boardData[r][c].isEmpty()) return false;
            r += stepR; c += stepC;
        }
        return true;
    };

    switch (piece.toLatin1()) {
        case 'P': {  // Pawn
            int dir = (color == 'w' ? -1 : +1);
            // forward move
            if (dc == 0) {
                // single
                if (dr == dir && dst.isEmpty())
                    return true;
                // double from starting rank
                int startRow = (color == 'w' ? 6 : 1);
                if (oldRow == startRow && dr == 2*dir && dst.isEmpty()
                    && m_boardData[oldRow + dir][oldCol].isEmpty())
                    return true;
            }
            // capture diagonally
            if (adr == 1 && dr == dir && !dst.isEmpty() && dst[0] != color)
                return true;
            return false;
        }
        case 'R': {  // Rook
            if ((dr == 0 && dc != 0) || (dr != 0 && dc == 0)) {
                int stepR = (dr == 0 ? 0 : dr/adr);
                int stepC = (dc == 0 ? 0 : dc/adc);
                return pathClear(stepR, stepC);
            }
            return false;
        }
        case 'B': {  // Bishop
            if (adr == adc && adr != 0) {
                int stepR = dr/adr, stepC = dc/adc;
                return pathClear(stepR, stepC);
            }
            return false;
        }
        case 'Q': {  // Queen
            if ( (adr == adc && adr != 0) ||
                (dr == 0 && dc != 0) ||
                (dr != 0 && dc == 0) ) {
                int stepR = (dr == 0 ? 0 : dr/std::max(1,adr));
                int stepC = (dc == 0 ? 0 : dc/std::max(1,adc));
                return pathClear(stepR, stepC);
            }
            return false;
        }
        case 'N': {  // Knight
            return ( (adr==2 && adc==1) || (adr==1 && adc==2) );
        }
        case 'K': {  // King
            return (adr <= 1 && adc <= 1);
        }
        default:
            return false;
    }

    return true;
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
