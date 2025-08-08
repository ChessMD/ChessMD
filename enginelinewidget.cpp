#include "enginelinewidget.h"
#include "chessposition.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QPainter>
#include <QTimer>
#include <QShowEvent>

EngineLineWidget::EngineLineWidget(const QString &eval, const QString &pv, const QSharedPointer<NotationMove> &rootMove, QWidget *parent)
    : QWidget(parent)
    , m_fullText(pv)
    , m_evalBtn(new QPushButton(this))
    , m_truncLabel(new QLabel(this))
    , m_arrow(new QToolButton(this))
{
    QFont f = font();
    f.setPointSize(f.pointSize() + 2);
    setFont(f);

    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    m_evalBtn->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_truncLabel->setAttribute(Qt::WA_TransparentForMouseEvents);


    m_rootMove = rootMove;

    m_evalBtn->setFlat(true);
    m_evalBtn->setEnabled(false);
    m_evalBtn->setFocusPolicy(Qt::NoFocus);

    m_truncLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_truncLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_truncLabel->setWordWrap(false);

    m_arrow->setText(">");
    m_arrow->setAutoRaise(true);
    m_arrow->setFixedSize(16,16);
    m_arrow->setFocusPolicy(Qt::NoFocus);
    m_arrow->setStyleSheet("background: transparent;");
    connect(m_arrow, &QToolButton::clicked, this, &EngineLineWidget::toggleExpanded);

    auto *h = new QHBoxLayout;
    h->setContentsMargins(4,4,4,4);
    h->setSpacing(8);
    h->addWidget(m_evalBtn);
    h->addWidget(m_truncLabel, 1);
    h->addWidget(m_arrow, 0, Qt::AlignRight | Qt::AlignVCenter);

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(0,0,0,0);
    v->setSpacing(2);
    v->addLayout(h);

    updateEval(eval);
}

void EngineLineWidget::updateEval(const QString &newEval) {
    m_evalTxt = newEval;
    m_evalBtn->setText(newEval);
    restyleEval(newEval);
    update();
}

void EngineLineWidget::toggleExpanded() {
    m_expanded = !m_expanded;
    m_arrow->setText(m_expanded ? "v" : ">");
    update();
}

void EngineLineWidget::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setFont(font());
    QFontMetrics fm(font());
    int lineH = fm.height() + 4;
    int x = 4, y = 4;

    // draw eval box
    int btnW = m_evalBtn->width();
    int btnH = m_evalBtn->height();
    m_evalBtn->move(4, 4);
    m_evalBtn->show();
    x += btnW + 8;

    // draw arrow
    m_arrow->move(width()-m_arrow->width()-4, y);
    m_arrow->show();

    // paint moves
    QVector<QSharedPointer<NotationMove>> moves;
    auto curMove = m_rootMove;
    while (curMove && !curMove->m_nextMoves.isEmpty()) {
        moves.append(curMove);
        curMove = curMove->m_nextMoves.first();
    }
    m_moveSegments.clear();
    int availW = width() - x - m_arrow->width() - 8;
    for (int i = 0; i < moves.size(); ++i) {
        QString numPrefix;
        int moveNum = moves[i]->m_position->getPlyCount()/2 + 1;
        if (moves[i]->m_position->m_sideToMove == 'b') {
            numPrefix = QString::number(moveNum) + ".";
        } else if (moves[i]->isVarRoot) {
            numPrefix = QString::number(moveNum) + "...";
        }
        QString tok = numPrefix + moves[i]->moveText + " ";
        int w = fm.horizontalAdvance(tok);
        if (!m_expanded && x + w - 4 > m_evalBtn->width()+availW) break;
        if (m_expanded && x + w > width()-4) {
            x = m_evalBtn->geometry().right()+8;
            y += lineH;
        }
        p.setPen(Qt::black);
        p.drawText(x, y + fm.ascent(), tok);
        MoveSegment seg;
        seg.rect = QRect(x, y, w, lineH);
        seg.move = moves[i];
        m_moveSegments.append(seg);
        x += w;
    }

    if (m_hoveredSegment) {
        QColor col(255, 255, 0, 100);
        QPen pen(Qt::black, 1);
        p.setPen(pen);
        p.setBrush(col);
        QRect r = m_hoveredSegment->rect.adjusted(-1,0,1,0);
        p.drawRect(r);
    }

    int totalHeight = y + lineH + 4;    // 4px bottom margin
    if (minimumHeight() != totalHeight) {
        setMinimumHeight(totalHeight);
        updateGeometry();
    }
}

void EngineLineWidget::mousePressEvent(QMouseEvent *ev) {
    for (MoveSegment &seg : m_moveSegments) {
        if (seg.rect.contains(ev->pos())) {
            emit moveClicked(seg.move);
            return;
        }
    }
    QWidget::mousePressEvent(ev);
}


void EngineLineWidget::restyleEval(const QString &text) {
    bool positive = !text.startsWith('-');
    QString bg = positive ? QStringLiteral("white") : QStringLiteral("#333"); //hcc
    QString fg = positive ? QStringLiteral("black") : QStringLiteral("white"); //hcc
    QString sheet = QStringLiteral(R"(
        QPushButton {
            font-size: 14px;
            font-weight: bold;
                    border: 1px solid #888; /*hcc*/
            border-radius: 4px;
            padding: 4px 8px;
            min-width: 50px;
            background: %1;
            color: %2;
        }
    )").arg(bg, fg);

    m_evalBtn->setStyleSheet(sheet);
}

void EngineLineWidget::mouseMoveEvent(QMouseEvent* event) {
    MoveSegment* newHover = nullptr;

    for (auto &seg : m_moveSegments) {
        if (seg.rect.contains(event->pos())) {
            emit moveHovered(seg.move);
            newHover = &seg;
            break;
        }
    }
    if (!newHover) {
        emit moveHovered(nullptr);
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::PointingHandCursor);
    }

    if (m_hoveredSegment != newHover) {
        m_hoveredSegment = newHover;
        update();
    }

    QWidget::mouseMoveEvent(event);
}

void EngineLineWidget::leaveEvent(QEvent* event) {
    if (m_hoveredSegment) {
        m_hoveredSegment = nullptr;
        update();
    }
    emit moveHovered(nullptr);
    QWidget::leaveEvent(event);
}

