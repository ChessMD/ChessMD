#include "enginelinewidget.h"

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

    // 1) draw eval box
    int btnW = m_evalBtn->width();
    int btnH = m_evalBtn->height();
    // Position it at (4,4)
    m_evalBtn->move(4, 4);
    m_evalBtn->show();
    // Advance x by its width + spacing
    x += btnW + 8;

    // 2) draw arrow
    m_arrow->move(width()-m_arrow->width()-4, y);
    m_arrow->show();

    // 3) paint moves
    QVector<QSharedPointer<NotationMove>> moves;
    auto cur = m_rootMove;
    moves.append(cur);
    while (cur && !cur->m_nextMoves.isEmpty()) {
        cur = cur->m_nextMoves.first();
        moves.append(cur);
    }
    m_moveSegments.clear();
    int availW = width() - x - m_arrow->width() - 8;
    for (int i = 0; i < moves.size(); ++i) {
        QString tok = moves[i]->moveText + " ";
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

    int totalHeight = y + lineH + 4;    // 4px bottom margin
    if (minimumHeight() != totalHeight) {
        setMinimumHeight(totalHeight);
        updateGeometry();             // tell the layout to re‑calc
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
    bool positive = !text.startsWith(u'-');
    QString bg = positive ? QStringLiteral("white") : QStringLiteral("#333");
    QString fg = positive ? QStringLiteral("black") : QStringLiteral("white");
    QString sheet = QStringLiteral(R"(
        QPushButton {
            font-size: 14px;
            font-weight: bold;
            border: 1px solid #888;
            border-radius: 4px;
            padding: 4px 8px;
            min-width: 50px;
            background: %1;
            color: %2;
        }
    )").arg(bg, fg);

    m_evalBtn->setStyleSheet(sheet);
}

void EngineLineWidget::mouseMoveEvent(QMouseEvent* ev) {
    qDebug() << "hello";

    for (auto &seg : m_moveSegments) {
        if (seg.rect.contains(ev->pos())) {
            qDebug() << "dasd\n";
            emit moveHovered(seg.move);
            return;
        }
    }
    QWidget::mouseMoveEvent(ev);
}

void EngineLineWidget::leaveEvent(QEvent* ev) {
    emit moveHovered(nullptr);
    QWidget::leaveEvent(ev);
}

