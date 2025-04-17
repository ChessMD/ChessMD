#ifndef NOTATIONVIEWER_H
#define NOTATIONVIEWER_H

#include <QAbstractScrollArea>
#include <QList>
#include <QSharedPointer>
#include <QRect>
#include "notation.h"

// A structure to hold layout info for each clickable move segment.
struct MoveSegment {
    QRect rect;                         // Bounding rectangle of the move text.
    QSharedPointer<NotationMove> move;  // Pointer to the move data.
};

class NotationViewer : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit NotationViewer(QWidget* parent = nullptr);

    void setRootMove(const QSharedPointer<NotationMove>& notation);
    QSharedPointer<NotationMove> getRootMove();
    QSharedPointer<NotationMove> getSelectedMove();

    void refresh();

    QSharedPointer<NotationMove> m_selectedMove; // Current move

public slots:
    void selectPreviousMove();
    void selectNextMove();

signals:
    void moveSelected(const QSharedPointer<NotationMove>& move);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // DFS traverse notation tree
    void drawNotation(QPainter& painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y, bool shouldDrawMove = true);

    // Draw individual move
    void drawMove(QPainter& painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y);

    void clearLayout();
    void layoutNotation();

    void drawTextSegment(QPainter &painter, const QString &text, int x, int &y, int indent, int availableWidth, QRect &outRect);

    QSharedPointer<NotationMove> m_rootMove;
    QList<MoveSegment> m_moveSegments;   // Clickable segments

    // Parameters for drawing
    int m_indentStep;
    int m_lineSpacing;
};


#endif // NOTATIONVIEWER_H
