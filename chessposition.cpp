#include "chessposition.h"

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
    if (m_boardData[oldRow][oldCol][0] == "B" && plyCount % 2 == 0){
        return false;
    } if (m_boardData[oldRow][oldCol][0] == "W" && plyCount % 2 == 1){
        return false;
    }

    if (m_boardData[oldRow][oldCol][1] == "P"){ // Pawn

    } else     if (m_boardData[oldRow][oldCol][1] == "R"){ // Rook

    } else     if (m_boardData[oldRow][oldCol][1] == "N"){ // Knight

    } else     if (m_boardData[oldRow][oldCol][1] == "B"){ // Bishop

    } else     if (m_boardData[oldRow][oldCol][1] == "Q"){ // Queen

    } else     if (m_boardData[oldRow][oldCol][1] == "K"){ // King

    }

    return true;
}

void ChessPosition::release(int oldRow, int oldCol, int newRow, int newCol)
{
    // Ensure indices are within bounds
    if (oldRow < 0 || oldRow >= 8 || oldCol < 0 || oldCol >= 8 ||
        newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8)
    {
        qWarning() << "Invalid index";
        return;
    }

    if (oldRow == newRow && oldCol == newCol){
        return;
    }

    if (!validateMove(oldRow, oldCol, newRow, newCol)){
        return;
    }

    // Retrieve the piece from the old position
    QString piece = m_boardData[oldRow][oldCol];

    // If there's no piece at the source, exit
    if (piece.isEmpty())
        return;

    m_boardData[newRow][newCol] = piece;
    m_boardData[oldRow][oldCol] = "";  // Clear the old position

    // Notify QML about the update.
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
