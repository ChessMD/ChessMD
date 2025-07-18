/*
March 18, 2025: File Creation
*/

#ifndef NOTATIONVIEWER_H
#define NOTATIONVIEWER_H

#include <QAbstractScrollArea>
#include <QList>
#include <QSharedPointer>
#include <QRect>

#include "notation.h"
#include "pgngamedata.h"

// Holds layout info for each clickable move segment
struct MoveSegment {
    QRect rect; // Bounding rectangle of the move text
    QSharedPointer<NotationMove> move;
};

class NotationViewer : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit NotationViewer(PGNGame game, QWidget* parent = nullptr);

    void setRootMove(const QSharedPointer<NotationMove>& notation);
    QSharedPointer<NotationMove> getRootMove();
    QSharedPointer<NotationMove> getSelectedMove();

    void refresh();

    bool m_isEdited;
    PGNGame m_game;
    QSharedPointer<NotationMove> m_selectedMove; // Current move

public slots:
    void selectPreviousMove();
    void selectNextMove();
    void onEngineMoveClicked(QSharedPointer<NotationMove> &move);

signals:
    void moveSelected(const QSharedPointer<NotationMove>& move);
    void moveHovered(QSharedPointer<NotationMove> move);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onActionAddAnnotation();
    void onActionDeleteVariation();
    void onActionDeleteMovesAfter();
    void onActionPromoteVariation();

private:
    // DFS traverse notation tree
    void drawNotation(QPainter& painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y, bool isMain, bool shouldDrawMove = true);

    // Draw individual move
    void drawMove(QPainter& painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y, bool isMain);

    void clearLayout();
    void layoutNotation();

    void drawTextSegment(QPainter &painter, const QString &text, int x, int &y, int indent, int availableWidth, QRect &outRect);

    QSharedPointer<NotationMove> m_rootMove;
    QSharedPointer<NotationMove> m_contextMenuMove;
    QList<MoveSegment> m_moveSegments;   // Clickable segments    

    // Parameters for drawing
    int m_indentStep;
    int m_lineSpacing;

    QSharedPointer<NotationMove> m_lastHoveredMove;

};

#endif // NOTATIONVIEWER_H
