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

class ChessPosition: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector<QVector<QString>> boardData READ boardData WRITE setBoardData NOTIFY boardDataChanged)

public:
    explicit ChessPosition(QObject *parent = nullptr);

    QVector<QVector<QString>> boardData() const;
    void setBoardData(const QVector<QVector<QString>> &data);

    Q_INVOKABLE void release(int oldRow, int oldCol, int newRow, int newCol);
    void copyFrom(const ChessPosition &other);
    QString positionToFEN() const;

    bool makeMove(QString san);

    int plyCount;

signals:
    void boardDataChanged();

    void moveMade(QSharedPointer<NotationMove> move);

private:
    bool validateMove(int oldRow, int oldCol, int newRow, int newCol) const;
    bool isLegalPseudo(int sr, int sc, int dr, int dc, QChar promo) const;
    bool applyMove(int sr, int sc, int dr, int dc, QChar promotion);

    bool inCheck(QChar side) const;
    bool canCastleKingside(QChar side) const;
    bool canCastleQueenside(QChar side) const;
    QVector<QPair<int,int>> findPieceOrigins(QChar piece, const QString &dest, const QString &sanDisamb) const;

    QVector<QVector<QString>> m_boardData;
    CastlingRights m_castling;
    QString m_enPassantTarget;
    int m_halfmoveClock;
    int m_fullmoveNumber;
    char m_sideToMove;
};

QVector<QVector<QString>> convertFenToBoardData(const QString &fen);
void buildNotationTree(const QSharedPointer<VariationNode> varNode, QSharedPointer<NotationMove> parentMove);

#endif // CHESSPOSITION_H
