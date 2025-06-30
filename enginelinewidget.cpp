#include "enginelinewidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QTimer>
#include <QShowEvent>

EngineLineWidget::EngineLineWidget(const QString &eval, const QString &pv, QWidget *parent)
    : QWidget(parent)
    , m_fullText(pv)
    , m_evalBtn(new QPushButton(this))
    , m_truncLabel(new QLabel(this))
    , m_arrow(new QToolButton(this))
{
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

void EngineLineWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &EngineLineWidget::doElide);
}

void EngineLineWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (!m_expanded) {
        QTimer::singleShot(0, this, &EngineLineWidget::doElide);
    }
}

void EngineLineWidget::doElide() {
    if (!m_expanded) updateElided();
}

void EngineLineWidget::updateEval(const QString &newEval) {
    m_evalBtn->setText(newEval);
    restyleEval(newEval);
}

void EngineLineWidget::toggleExpanded() {
    m_expanded = !m_expanded;
    if (m_expanded) {
        m_truncLabel->setWordWrap(true);
        m_truncLabel->setText(m_fullText);
        m_arrow->setText("v");
    } else {
        m_truncLabel->setWordWrap(false);
        updateElided();
        m_arrow->setText(">");
    }
}

void EngineLineWidget::updateElided() {
    int avail = m_truncLabel->width();
    if (avail < 10) avail = 10;
    QFontMetrics fm(m_truncLabel->font());
    m_truncLabel->setText(
        fm.elidedText(m_fullText, Qt::ElideRight, avail)
        );
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
