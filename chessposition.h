/*
March 20, 2025: File Creation
*/

#ifndef CHESSPOSITION_H
#define CHESSPOSITION_H

#include <QString>
#include <QVector>
#include <QObject>

#include "notation.h"
#include "pgngamedata.h"

struct CastlingRights {
    bool whiteKing  = false;
    bool whiteQueen = false;
    bool blackKing  = false;
    bool blackQueen = false;
};

// Represents a chess position
class ChessPosition: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector<QVector<QString>> boardData READ boardData WRITE setBoardData NOTIFY boardDataChanged)

public:
    explicit ChessPosition(QObject *parent = nullptr);

    QVector<QVector<QString>> boardData() const;
    void setBoardData(const QVector<QVector<QString>> &data);

    // Called from Qml when the user tries to make a new move
    Q_INVOKABLE void release(int oldRow, int oldCol, int newRow, int newCol);

    // Copies all internal state from another ChessPosition
    void copyFrom(const ChessPosition &other);
    QString positionToFEN() const;

    // Tries to make a new move from the current position given a SAN string
    bool tryMakeMove(QString san);
    void applyMove(int sr, int sc, int dr, int dc, QChar promotion);

    int plyCount;

signals:
    // Signals QML to update board display
    void boardDataChanged();
    // Signals ChessGameWindow to append new move to current selected move
    void moveMade(QSharedPointer<NotationMove> move);

private:
    bool validateMove(int oldRow, int oldCol, int newRow, int newCol) const;
    bool squareAttacked(int row, int col, QChar attacker) const;

    bool inCheck(QChar side) const;
    bool canCastleKingside(QChar side) const;
    bool canCastleQueenside(QChar side) const;

    // Finds possible origin squares for a given piece and destination`
    QVector<QPair<int,int>> findPieceOrigins(QChar piece, const QString &dest, const QString &sanDisamb) const;

    QVector<QVector<QString>> m_boardData;
    CastlingRights m_castling;
    QString m_enPassantTarget;
    int m_halfmoveClock;
    int m_fullmoveNumber;
    char m_sideToMove;
};

QVector<QVector<QString>> convertFenToBoardData(const QString &fen);
// Recursively builds a Notation tree from PgnGameData
void buildNotationTree(const QSharedPointer<VariationNode> varNode, QSharedPointer<NotationMove> parentMove);

#endif // CHESSPOSITION_H
