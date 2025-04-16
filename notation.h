#ifndef NOTATION_H
#define NOTATION_H

#include <QString>
#include <QList>
#include <QSharedPointer>

#include "chessposition.h"

class NotationMove
{
public:
    NotationMove() = default;
    NotationMove(const QString &moveText) : moveText(moveText) {}

    ChessPosition m_position;
    QString FEN; // Chess position after this move

    QString commentBefore; // Optional comment that appears before the move
    QString moveText; // The move text (e.g. "e4", "Nf3")
    QString annotation1; // Optional annotations, e.g. "!" or "?"
    QString annotation2;
    QString commentAfter; // Optional comment that appears after the move

    bool isVarRoot = false; // First move in variation

    QList<QSharedPointer<NotationMove>> m_nextMoves; // A move may be followed by one or more moves
    QSharedPointer<NotationMove> m_previousMove; // A move will only be preceded by a single move
};

void linkMoves(const QSharedPointer<NotationMove>& parent, const QSharedPointer<NotationMove>& child);
void deleteMovesAfter(const QSharedPointer<NotationMove>& move);
QSharedPointer<NotationMove> deleteVariation(const QSharedPointer<NotationMove>& move);

#endif // NOTATION_H
