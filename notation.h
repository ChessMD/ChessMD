/*
March 18, 2025: File Creation
*/

#ifndef NOTATION_H
#define NOTATION_H

#include <QString>
#include <QList>
#include <QSharedPointer>

class ChessPosition;

class NotationMove
{
public:
    NotationMove(const QString &text, ChessPosition &position);

    QString FEN; // FEN string of chess position (after this move)

    QString commentBefore;
    QString moveText;
    QString annotation1;
    QString annotation2;
    QString commentAfter;

    bool isVarRoot = false; // Indicates if first move in variation

    ChessPosition *m_position; // Full chess position (after this move)
    QList<QSharedPointer<NotationMove>> m_nextMoves;
    QSharedPointer<NotationMove> m_previousMove;
};

// Appends a new NotationMove to the current
void linkMoves(const QSharedPointer<NotationMove>& parent, const QSharedPointer<NotationMove>& child);
// Deletes all NotationMoves after the current NotationMove
void deleteMovesAfter(const QSharedPointer<NotationMove>& move);
// Promotes the variation containing the current NotationMove up one branch
void promoteVariation(const QSharedPointer<NotationMove>& move);
// Deletes all NotationMove in the variation containing the current NotationMove
QSharedPointer<NotationMove> deleteVariation(const QSharedPointer<NotationMove>& move);

#endif // NOTATION_H
