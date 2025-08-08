/*
March 18, 2025: File Creation
*/

#include "notationviewer.h"
#include "variationdialogue.h"
#include "chessposition.h"

#include <QPainter>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>
#include <QMenu>
#include <QTextLayout>
#include <QInputDialog>
#include <QRegularExpression>

NotationViewer::NotationViewer(PGNGame game, QWidget* parent)
    : QAbstractScrollArea(parent)
    , m_game(game)
    , m_rootMove(game.rootMove)
    , m_isEdited(false)
{
    QFont f = font();
    f.setPointSize(f.pointSize() + 2);
    setFont(f);
    viewport()->setFont(f);

    setMouseTracking(true);
    viewport()->setMouseTracking(true);
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
    emit moveSelected(m_selectedMove);
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
    if (m_rootMove){
        drawNotation(painter, m_rootMove, 0, x, y, true, false);
    }
    // Set the viewport's height based on total text height
    verticalScrollBar()->setRange(0, qMax(0, y - viewport()->height()));
    verticalScrollBar()->setPageStep(viewport()->height());
}

void NotationViewer::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    QFontMetrics fm(painter.font());
    painter.setFont(font());
    painter.translate(0, -verticalScrollBar()->value());
    painter.fillRect(viewport()->rect().translated(0, verticalScrollBar()->value()), Qt::white);
    clearLayout();
    int x = 0, y = 0, lineHeight = fm.height() + m_lineSpacing;
    if (m_rootMove) {
        drawNotation(painter, m_rootMove, 0, x, y, true, false);
    }

    if (!m_selectedMove.isNull()) {
        for (const MoveSegment &seg : m_moveSegments) {
            if (seg.move == m_selectedMove) {
                painter.setBrush(QColor(200, 200, 255, 128)); //hcc
                painter.setPen(Qt::NoPen);
                painter.drawRect(seg.rect);
            }
        }
    }
    y += lineHeight + fm.ascent();
    QFont boldFont = painter.font();
    boldFont.setBold(true);
    painter.setFont(boldFont);
    painter.setPen(Qt::black);
    painter.setBrush(Qt::NoBrush);
    painter.drawText(0, y, m_game.result);
}

void drawManuallyWrappedText(QPainter &painter, const QString &text, int indent, int &x, int &y, int availWidth, int lineSpacing)
{
    QFontMetrics fm(painter.font());
    int lineHeight = fm.height() + lineSpacing;
    QStringList tokens = text.split(' ', Qt::SkipEmptyParts);
    for (int i = 0; i < tokens.size(); ++i) {
        QString tok = tokens[i];
        int tokWidth = fm.horizontalAdvance(tok + ' ');
        if (x - indent + tokWidth > availWidth) {
            x = indent;
            y += lineHeight;
        }
        painter.setPen(QColor(126,158,126)); //hcc
        painter.drawText(x, y + fm.ascent(), tok + ' ');
        x += tokWidth;
    }
    painter.setPen(Qt::black);
}


void NotationViewer::drawMove(QPainter &painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y, bool isMain)
{
    QFont normal = painter.font();
    QFontMetrics fm(normal);
    int lineHeight = fm.height() + m_lineSpacing, availWidth = viewport()->width() - indent - 10;
    if (isMain) {
        normal.setBold(true);
        painter.setFont(normal);
        fm = QFontMetrics(normal);
        lineHeight = fm.height() + m_lineSpacing;
    }

    QString numPrefix;
    int moveNum = currentMove->m_position->getPlyCount()/2 + 1;
    if (currentMove->m_position->m_sideToMove == 'b') {
        numPrefix = QString::number(moveNum) + ".";
    } else if (currentMove->isVarRoot) {
        numPrefix = QString::number(moveNum) + "...";
    }

    QString moveStr = numPrefix + currentMove->moveText + currentMove->annotation1 + currentMove->annotation2;
    QString preComment = currentMove->commentBefore.isEmpty() ? "" : currentMove->commentBefore + " ";
    QString postComment = currentMove->commentAfter;
    QString fullMove = preComment + moveStr + postComment + " ";

    drawManuallyWrappedText(painter, preComment, indent, x, y, availWidth, m_lineSpacing);

    if (x - indent + fm.horizontalAdvance(moveStr + ' ') > availWidth) {
        x = indent;
        y += lineHeight;
    }

    QPen oldPen = painter.pen();
    QPen colorPen = oldPen;
    if (currentMove->annotation1 == "?!")  colorPen.setColor(QColor(247,198,49)); //hcc
    else if (currentMove->annotation1 == "?") colorPen.setColor(QColor(255,164,89)); //hcc
    else if (currentMove->annotation1 == "??") colorPen.setColor(QColor(250,65,45)); //hcc
    else if (currentMove->annotation1 == "!!") colorPen.setColor(QColor(38,194,163)); //hcc
    else if (currentMove->annotation1 == "!") colorPen.setColor(QColor(116,155,191)); //hcc
    painter.setPen(colorPen);

    painter.drawText(x, y + fm.ascent(), moveStr + ' ');

    // Store all segments to use when highlighting moves
    MoveSegment seg = {QRect(x, y, fm.horizontalAdvance(moveStr + ' '), lineHeight), currentMove};
    m_moveSegments.append({seg});
    x += fm.horizontalAdvance(moveStr + ' ');

    drawManuallyWrappedText(painter, postComment, indent, x, y, availWidth, m_lineSpacing);

    if (isMain){
        QFont unbold = normal;
        unbold.setBold(false);
        painter.setFont(unbold);
    }
}

void NotationViewer::drawNotation(QPainter &painter, const QSharedPointer<NotationMove>& currentMove, int indent, int& x, int& y, bool isMain, bool shouldDrawMove)
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
        drawMove(painter, currentMove, indent, x, y, isMain);
    }

    if (currentMove->m_nextMoves.size() == 1){
        // Continue down current variation
        drawNotation(painter, currentMove->m_nextMoves.front(), indent, x, y, isMain);
    } else if (currentMove->m_nextMoves.size() > 1){
        drawMove(painter, currentMove->m_nextMoves.front(), indent, x, y, isMain);
        // Go through variations
        y += lineHeight;
        for (int i = 1; i < currentMove->m_nextMoves.size(); i++) {
            // Use DFS search to explore all moves in the game tree
            x = indent + m_indentStep;
            drawNotation(painter, currentMove->m_nextMoves[i], indent + m_indentStep, x, y, false);
            x = indent;
        }
        drawNotation(painter, currentMove->m_nextMoves.front(), indent, x, y, isMain, false);
    }

    if (indent && !currentMove->m_nextMoves.size()) {
        // Variation ended, draw the closing bracket and add a new line.
        painter.drawText(x, y + fm.ascent(), ")");
        x += fm.horizontalAdvance(")");
        y += lineHeight;
    }
}

void NotationViewer::mouseMoveEvent(QMouseEvent* event) {
    bool overSegment = false;
    QPoint pos = event->pos();
    pos.setY(pos.y() + verticalScrollBar()->value());

    for (auto &seg : m_moveSegments) {
        if (seg.rect.contains(pos)) {
            overSegment = true;
            break;
        }
    }
    if (overSegment) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QWidget::mouseMoveEvent(event);
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


void NotationViewer::onEngineMoveClicked(QSharedPointer<NotationMove> &move) {
    m_isEdited = true;
    QSharedPointer<NotationMove> tempMove = move;
    while(tempMove->m_previousMove){
        tempMove = tempMove->m_previousMove;
    }
    linkMoves(m_selectedMove, tempMove);
    move->m_nextMoves.clear();
    m_selectedMove = move;
    emit moveSelected(m_selectedMove);
    refresh();
}

void NotationViewer::contextMenuEvent(QContextMenuEvent *event) {
    // Adjust for scroll offset
    QPoint pos = event->pos();
    pos.setY(pos.y() + verticalScrollBar()->value());

    // Find which segment (if any) was clicked
    QSharedPointer<NotationMove> clickedMove;
    for (const MoveSegment &seg : m_moveSegments) {
        if (seg.rect.contains(pos)) {
            clickedMove = seg.move;
            break;
        }
    }

    if (!clickedMove) {
        // Nothing under the click → default behavior
        return QAbstractScrollArea::contextMenuEvent(event);
    }

    // Remember the move for the slot handlers
    m_contextMenuMove = clickedMove;
    m_selectedMove = clickedMove;

    // Build the menu
    QMenu menu(this);

    menu.setToolTipsVisible(false);
    menu.setSeparatorsCollapsible(false);

    // —— “Add Annotation…” submenu ——
    struct Annot { QString text; bool secondary; QKeySequence seq; };
    static const QVector<Annot> annots = {
        { "(none)",    false, QKeySequence() },
        { "!",         false, QKeySequence() },
        { "?",         false, QKeySequence() },
        { "!?",        false, QKeySequence() },
        { "?!",        false, QKeySequence() },
        { "!!",        false, QKeySequence() },
        { "??",        false, QKeySequence() },
    };

    QMenu *annotMenu = menu.addMenu(tr("Add Annotation"));
    for (auto &a : annots) {
        QAction *act = new QAction(a.text, annotMenu);
        act->setShortcut(a.seq);
        act->setShortcutVisibleInContextMenu(true);
        annotMenu->addAction(act);

        connect(act, &QAction::triggered, this, [this, a]() {
            m_isEdited = true;
            if (!m_contextMenuMove) return;
            if (a.text == "(none)") {
                m_contextMenuMove->annotation1.clear();
                m_contextMenuMove->annotation2.clear();
            } else if (!a.secondary) {
                m_contextMenuMove->annotation1 = a.text;
            } else {
                m_contextMenuMove->annotation2 = a.text;
            }
            refresh();
        });
    }

    struct CommentEntry {
        QString actionText;
        QString NotationMove::* member;
    };

    const QVector<CommentEntry> commentEntries = {
        { tr("Enter Comment Before"), &NotationMove::commentBefore },
        { tr("Enter Comment After"),  &NotationMove::commentAfter  }
    };

    for (auto &ce : commentEntries) {
        QAction *act = menu.addAction(ce.actionText);
        connect(act, &QAction::triggered, this, [this, ce]() {
            if (!m_contextMenuMove) return;
            bool ok = false;
            // fetch the current comment to use as the default text
            QString initial = (m_contextMenuMove.data()->*(ce.member));
            QString text = QInputDialog::getText(
                this,
                ce.actionText,                            // dialog title
                tr("Enter %1").arg(ce.actionText),       // prompt
                QLineEdit::Normal,
                initial,
                &ok
                );
            if (ok) {
                m_isEdited = true;
                (m_contextMenuMove.data()->*(ce.member)) = text.trimmed();
                refresh();
            }
        });
    }

    QAction *delVar =  new QAction(tr("Delete Variation"), &menu);
    delVar->setShortcut(QKeySequence("Ctrl+D"));
    delVar->setShortcutVisibleInContextMenu(true);
    connect(delVar, &QAction::triggered, this, &NotationViewer::onActionDeleteVariation);
    menu.addAction(delVar);

    QAction *delMovesAfter =  new QAction(tr("Delete Moves After"), &menu);
    delMovesAfter->setShortcut(QKeySequence("Delete"));
    delMovesAfter->setShortcutVisibleInContextMenu(true);
    connect(delMovesAfter, &QAction::triggered, this, &NotationViewer::onActionDeleteMovesAfter);
    menu.addAction(delMovesAfter);

    QAction *promoteVar =  new QAction(tr("Promote Variation"), &menu);
    promoteVar->setShortcut(QKeySequence("Ctrl+Up"));
    promoteVar->setShortcutVisibleInContextMenu(true);
    connect(promoteVar, &QAction::triggered, this, &NotationViewer::onActionPromoteVariation);
    menu.addAction(promoteVar);

    menu.exec(event->globalPos());
}

void NotationViewer::onActionDeleteVariation() {
    m_isEdited = true;
    m_selectedMove = deleteVariation(m_selectedMove);
    emit moveSelected(m_selectedMove);
    refresh();
}

void NotationViewer::onActionDeleteMovesAfter() {
    m_isEdited = true;
    deleteMovesAfter(m_selectedMove);
    refresh();
}


void NotationViewer::onActionPromoteVariation() {
    m_isEdited = true;
    promoteVariation(m_selectedMove);
    refresh();
}


void NotationViewer::onActionAddAnnotation() {

}

