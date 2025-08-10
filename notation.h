/*
March 18, 2025: File Creation
*/

#ifndef NOTATION_H
#define NOTATION_H

#include <QString>
#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QMap>
#include <QKeySequence>

class ChessPosition;

class NotationMove
{
public:
    NotationMove(const QString &text, ChessPosition &position);

    QString FEN; // FEN string of chess position (after this move)

    QString commentBefore;
    QString moveText;
    QString lanText;
    QString annotation1;
    QString annotation2;
    QString commentAfter;

    bool isVarRoot = false; // Indicates if first move in variation

    ChessPosition *m_position; // Full chess position (after this move)
    QList<QSharedPointer<NotationMove>> m_nextMoves;
    QSharedPointer<NotationMove> m_previousMove;
};

struct AnnotationOption {
    QString text;
    bool secondary;
    QKeySequence seq;
};

struct CommentEntry {
    QString actionText;
    QString NotationMove::* member;
};

extern const QMap<int, QString> NUMERIC_ANNOTATION_MAP;
extern const QVector<AnnotationOption> ANNOTATION_OPTIONS ;
extern const QVector<CommentEntry> COMMENT_ENTRIES;

QSharedPointer<NotationMove> cloneNotationTree(QSharedPointer<NotationMove>& move);

// Appends a new NotationMove to the current
void linkMoves(const QSharedPointer<NotationMove>& parent, const QSharedPointer<NotationMove>& child);
// Deletes all NotationMoves after the current NotationMove
QSharedPointer<NotationMove> deleteMove(const QSharedPointer<NotationMove>& move);
// Deletes all comments and annotations from entire game tree
void deleteAllCommentary(QSharedPointer<NotationMove>& move);
// Promotes the variation containing the current NotationMove up one branch
void promoteVariation(const QSharedPointer<NotationMove>& move);
// Deletes all NotationMove in the variation containing the current NotationMove
QSharedPointer<NotationMove> deleteVariation(const QSharedPointer<NotationMove>& move);

#endif // NOTATION_H
