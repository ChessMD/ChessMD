#include "notationviewer.h"
#include "variationdialogue.h"

#include <QPainter>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>

NotationViewer::NotationViewer(QWidget* parent) : QAbstractScrollArea(parent)
{
    m_indentStep = 20;
    m_lineSpacing = 4;
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void NotationViewer::setRootMove(const QSharedPointer<NotationMove>& notation)
{
    m_rootMove = notation;
    m_selectedMove = m_rootMove;
    clearLayout();
    layoutNotation();
    viewport()->update();
}

QSharedPointer<NotationMove> NotationViewer::getRootMove()
{
    return m_rootMove;
}

QSharedPointer<NotationMove> NotationViewer::getSelectedMove()
{
    return m_selectedMove;
}

void NotationViewer::clearLayout()
{
    m_moveSegments.clear();
}

void NotationViewer::layoutNotation()
{
    // Temporary painter to compute text layout
    QPixmap dummy(viewport()->size());
    QPainter painter(&dummy);
    painter.setFont(font());
    int x = 0, y = 0;
    if (m_rootMove)
        drawNotation(painter, m_rootMove, 0, x, y);
    // Set the viewport's height based on total text height
    verticalScrollBar()->setRange(0, qMax(0, y - viewport()->height()));
    verticalScrollBar()->setPageStep(viewport()->height());
}

void NotationViewer::drawMove(QPainter &painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y)
{
    QFontMetrics fm(painter.font());
    int availableWidth = viewport()->width() - indent - 10; // some padding
    int lineHeight = fm.height() + m_lineSpacing;

    QString preComment = currentMove->commentBefore.isEmpty() ? "" : currentMove->commentBefore + " ";
    QString moveStr = currentMove->moveText;
    QString annotations = currentMove->annotation1 + currentMove->annotation2;
    QString postComment = currentMove->commentAfter;
    QString fullMove = preComment + moveStr + annotations + postComment + " ";

    // Line wrap
    if (x + fm.horizontalAdvance(fullMove) > viewport()->width() - 10) {
        x = indent;
        y += lineHeight;
    }

    painter.drawText(x, y + fm.ascent(), fullMove);

    // Record the bounding rectangle for the move text portion (we want the move text clickable).
    int moveTextStart = x + fm.horizontalAdvance(preComment);
    int moveTextWidth = fm.horizontalAdvance(moveStr);
    QRect moveRect(moveTextStart, y, moveTextWidth, lineHeight);
    x += fm.horizontalAdvance(fullMove); // Update x for next move.

    // Store all segments to use when highlighting moves
    MoveSegment seg;
    seg.rect = moveRect;
    seg.move = currentMove;
    m_moveSegments.append(seg);
}

void NotationViewer::drawNotation(QPainter &painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y, bool shouldDrawMove)
{
    QFontMetrics fm(painter.font());
    int lineHeight = fm.height() + m_lineSpacing;

    // Draw brackets around the variation if it is not a root
    if (currentMove->isVarRoot) {
        // Draw opening bracket before the first move
        painter.drawText(x, y + fm.ascent(), "( ");
        x += fm.horizontalAdvance("( "); // Adjust x after drawing bracket
    }

    if (shouldDrawMove){
        drawMove(painter, currentMove, indent, x, y);
    }

    if (currentMove->m_nextMoves.size() == 1){
        // Continue down current variation
        drawNotation(painter, currentMove->m_nextMoves.front(), indent, x, y);
    } else if (currentMove->m_nextMoves.size() > 1){
        drawMove(painter, currentMove->m_nextMoves.front(), indent, x, y);
        // Go through variations
        y += lineHeight;
        for (int i = 1; i < currentMove->m_nextMoves.size(); i++) {
            // Use DFS search to explore all moves in the game tree
            x = indent + m_indentStep;
            drawNotation(painter, currentMove->m_nextMoves[i], indent + m_indentStep, x, y);
            x = indent;
        }
        drawNotation(painter, currentMove->m_nextMoves.front(), indent, x, y, false);
    }

    if (indent && !currentMove->m_nextMoves.size()) {
        // Variation ended, draw the closing bracket and add a new line.
        painter.drawText(x, y + fm.ascent(), ")");
        x += fm.horizontalAdvance(")");
        y += lineHeight;
    }
}

void NotationViewer::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(viewport());
    painter.setFont(font());

    // Translate painter by scroll offset.
    painter.translate(0, -verticalScrollBar()->value());

    // Clear background.
    painter.fillRect(viewport()->rect().translated(0, verticalScrollBar()->value()), Qt::white);

    // Clear and recalc layout (in a more optimized version, do this only when data changes).
    clearLayout();
    int x = 0, y = 0;
    if (m_rootMove)
        drawNotation(painter, m_rootMove, 0, x, y);

    // Highlight the selected move if any.
    if (!m_selectedMove.isNull()) {
        for (const MoveSegment &seg : m_moveSegments) {
            if (seg.move == m_selectedMove) {
                painter.setBrush(QColor(200, 200, 255, 128));
                painter.setPen(Qt::NoPen);
                painter.drawRect(seg.rect);
            }
        }
    }
}

void NotationViewer::mousePressEvent(QMouseEvent *event)
{
    // Adjust for scroll offset
    QPoint pos = event->pos();
    pos.setY(pos.y() + verticalScrollBar()->value());

    // Check for clicked move segment
    for (const MoveSegment &seg : m_moveSegments) {
        if (seg.rect.contains(pos)) {
            m_selectedMove = seg.move;
            emit moveSelected(m_selectedMove);
            viewport()->update();
            return;
        }
    }
    QAbstractScrollArea::mousePressEvent(event);
}

void NotationViewer::selectPreviousMove()
{
    if (m_selectedMove != nullptr && m_selectedMove->m_previousMove != nullptr){
        m_selectedMove = m_selectedMove->m_previousMove;
        emit moveSelected(m_selectedMove);
        viewport()->update();
    }
}

void NotationViewer::selectNextMove()
{
    if (m_selectedMove != nullptr && !m_selectedMove->m_nextMoves.isEmpty()){
        if (m_selectedMove->m_nextMoves.size() == 1){
            m_selectedMove = m_selectedMove->m_nextMoves.front();
        } else {
            VariationDialogue dialog(this);
            dialog.setVariations(m_selectedMove);
            if (dialog.exec() == QDialog::Accepted) {
                auto selected = dialog.selectedMove();
                if (selected) {
                    m_selectedMove = selected;
                }
            }
        }
        emit moveSelected(m_selectedMove);
        viewport()->update();
    }
}

void NotationViewer::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    layoutNotation();
    viewport()->update();
}

void NotationViewer::refresh()
{
    layoutNotation();
    viewport()->update();
}

void NotationViewer::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        selectPreviousMove();
        break;
    case Qt::Key_Right:
        selectNextMove();
        break;
    default:
        QAbstractScrollArea::keyPressEvent(event);  // Pass on unhandled keys
        break;
    }
}
