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
#include <QtGlobal>

class ChessPosition;

// Individual node inside the chess game tree, containing information of the position that is reached after playing a move
class NotationMove
{
public:
    NotationMove(const QString &text, ChessPosition &position);

    QString FEN;
    quint64 m_zobristHash = 0;

    QString commentBefore;
    QString moveText;
    QString lanText;
    QString annotation1;
    QString annotation2;
    QString commentAfter;

    bool isVarRoot = false;

    QSharedPointer<ChessPosition> m_position;
    QList<QSharedPointer<NotationMove>> m_nextMoves;
    QWeakPointer<NotationMove> m_previousMove;
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

void linkMoves(const QSharedPointer<NotationMove>& parent, const QSharedPointer<NotationMove>& child);
QSharedPointer<NotationMove> deleteMove(const QSharedPointer<NotationMove>& move);
void deleteAllCommentary(QSharedPointer<NotationMove>& move);
void promoteVariation(const QSharedPointer<NotationMove>& move);
QSharedPointer<NotationMove> deleteVariation(const QSharedPointer<NotationMove>& move);

#endif // NOTATION_H
