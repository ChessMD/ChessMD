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

    // Set the starting position
    void setRootMove(const QSharedPointer<NotationMove>& notation);
    QSharedPointer<NotationMove> getRootMove();
    QSharedPointer<NotationMove> getSelectedMove();

    void refresh();

    QSharedPointer<NotationMove> m_selectedMove; // Currently selected move

public slots:
    void selectPreviousMove();
    void selectNextMove();

signals:
    // Emitted when a move is selected (clicked or navigated).
    void moveSelected(const QSharedPointer<NotationMove>& move);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    // Recursively layout and draw the notation.
    void drawNotation(QPainter& painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y, bool shouldDrawMove = true);
    void drawMove(QPainter& painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y);

    // Clear the cached layout information.
    void clearLayout();

    // Rebuild layout (calculate MoveSegment positions).
    void layoutNotation();

    // Returns the next X position given current text, wrapping if necessary.
    void drawTextSegment(QPainter &painter, const QString &text, int x, int &y, int indent, int availableWidth, QRect &outRect);

    // Data members.
    QSharedPointer<NotationMove> m_rootMove;
    QList<MoveSegment> m_moveSegments;   // Collected clickable segments

    // Parameters for drawing.
    int m_indentStep;   // pixels to indent for each variation level
    int m_lineSpacing;  // additional line spacing
};


#endif // NOTATIONVIEWER_H
