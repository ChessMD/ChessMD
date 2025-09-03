/*
March 20, 2025: File Creation
*/

#ifndef CHESSPOSITION_H
#define CHESSPOSITION_H

#include <QString>
#include <QVector>
#include <QObject>
#include <QDebug>

#include "notation.h"
#include "pgngamedata.h"

struct CastlingRights {
    bool whiteKing  = false;
    bool whiteQueen = false;
    bool blackKing  = false;
    bool blackQueen = false;
};

struct SimpleMove {
    int sr, sc, dr, dc;
    char promo;
};

// Represents a chess position
class ChessPosition: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector<QVector<QString>> boardData READ boardData WRITE setBoardData NOTIFY boardDataChanged)
    Q_PROPERTY(double evalScore READ evalScore WRITE setEvalScore NOTIFY evalScoreChanged)
    Q_PROPERTY(bool isPreview READ isPreview WRITE setIsPreview NOTIFY isPreviewChanged)
    Q_PROPERTY(int lastMove   READ lastMove   NOTIFY lastMoveChanged)

public:
    explicit ChessPosition(QObject *parent = nullptr);

    QVector<QVector<QString>> boardData() const;
    void setBoardData(const QVector<QVector<QString>> &data);

    // Called from Qml when the user tries to make a new move
    Q_INVOKABLE void release(int sr, int sc, int dr, int dc);
    Q_INVOKABLE void promote(int sr, int sc, int dr, int dc, QChar promo);

    bool isPreview() const { return m_isPreview; }
    void setIsPreview(bool p) {
        if (m_isPreview == p) return;
        m_isPreview = p;
        emit isPreviewChanged(p);
    }
    int lastMove() const { return m_lastMove; }

    double evalScore() const { return m_evalScore; }
    void setEvalScore(double v) {
        m_evalScore = v;
        emit evalScoreChanged();
    }

    int getPlyCount() const {return m_plyCount;}

    // Copies all internal state from another ChessPosition
    void copyFrom(const ChessPosition &other);
    QString positionToFEN() const;
    quint64 computeZobrist() const;

    // Tries to make a new move from the current position given a SAN string
    bool tryMakeMove(QString san, QSharedPointer<NotationMove> move);
    void applyMove(int sr, int sc, int dr, int dc, QChar promotion);
    bool validateMove(int oldRow, int oldCol, int newRow, int newCol) const;

    QString lanToSan(int sr, int sc, int dr, int dc, QChar promo) const;

    QVector<SimpleMove> generateLegalMoves() const;

    char m_sideToMove;

signals:
    // Signals QML to update board display
    void boardDataChanged();
    void requestPromotion(int sr, int sc, int dr, int dc);
    // Signals ChessGameWindow to append new move to current selected move
    void moveMade(QSharedPointer<NotationMove>& move);
    void isPreviewChanged(bool);
    void lastMoveChanged();
    void evalScoreChanged();

private:
    void buildUserMove(int sr, int sc, int dr, int dc, QChar promo);

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
    int m_plyCount;

    int m_lastMove = -1;
    bool m_isPreview = false;
    double m_evalScore = 0;
};

void initZobristTables();

QString buildMoveText(const QSharedPointer<NotationMove>& move);
void writeMoves(const QSharedPointer<NotationMove>& move, QTextStream& out, int plyCount);

QSharedPointer<NotationMove> parseEngineLine(const QString& line, QSharedPointer<NotationMove> startMove);
QVector<QVector<QString>> convertFenToBoardData(const QString &fen);
// Recursively builds a Notation tree from PgnGameData
void buildNotationTree(const QSharedPointer<VariationNode> varNode, QSharedPointer<NotationMove> parentMove);

#endif // CHESSPOSITION_H
