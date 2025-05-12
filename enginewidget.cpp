/*
April 11, 2025: File Creation
*/

#include "enginewidget.h"
#include "chessposition.h"
#include "chessqsettings.h"

#include "QFile"
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>

EngineWidget::EngineWidget(QWidget *parent)
    : QWidget(parent),
    m_engine(new UciEngine(this)),
    m_tree(new QTreeWidget(this)),
    m_multiPv(new QSpinBox(this)),
    m_console(new QTextEdit(this))
{
    // Tree setup
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({ tr("Eval"), tr("Variation") });
    m_tree->setRootIsDecorated(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setIndentation(12);

    // Console setup
    m_console->setReadOnly(true);
    m_console->setFixedHeight(150);

    // Controls
    m_multiPv->setRange(1, 10);
    m_multiPv->setValue(3);

    auto *ctrl = new QHBoxLayout;
    ctrl->addWidget(new QLabel(tr("Lines:")));
    ctrl->addWidget(m_multiPv);

    auto *lay = new QVBoxLayout(this);
    lay->addLayout(ctrl);
    lay->addWidget(m_tree);
    lay->addWidget(m_console);

    // m_console->hide();

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(200); // 200 ms debounce
    connect(m_debounceTimer, &QTimer::timeout, this, &EngineWidget::doPendingAnalysis);

    // Engine start & signals
    ChessQSettings s; s.loadSettings();
    m_engine->startEngine(s.getEngineFile());

    connect(m_engine, &UciEngine::pvUpdate, this, &EngineWidget::onPvUpdate);
    connect(m_engine, &UciEngine::infoReceived, this, &EngineWidget::onInfoLine);
    connect(m_engine, &UciEngine::commandSent, this, &EngineWidget::onCmdSent);   
}

void EngineWidget::onMoveSelected(const QSharedPointer<NotationMove>& move) {
    if (!move.isNull() && move->m_position) {
        m_sideToMove = (move->m_position->plyCount % 2 == 0 ? 'w' : 'b');
        m_currentFen = move->m_position->positionToFEN();
        m_debounceTimer->start();
    }
}

void EngineWidget::doPendingAnalysis() {
    qDebug() << m_currentFen;
    m_engine->setPosition(m_currentFen);
    analysePosition();
}

void EngineWidget::analysePosition() {
    if (m_currentFen.isEmpty()) return;

    m_engine->stopSearch();
    m_engine->requestReady();

    m_tree->clear();
    m_lineItems.clear();
    m_console->clear();

    int n = m_multiPv->value();
    for (int i = 1; i <= n; ++i) {
        auto *root = new QTreeWidgetItem(m_tree);
        root->setText(0, tr("Line %1…").arg(i));
        root->setText(1, tr("(waiting)"));
        root->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        m_lineItems[i] = root;
    }

    m_engine->startInfiniteSearch(n);
}

void EngineWidget::onPvUpdate(const PvInfo &info) {
    auto it = m_lineItems.find(info.multipv);
    if (it == m_lineItems.end()) return;
    QTreeWidgetItem *root = it.value();

    QString evalTxt = info.isMate ? tr("Mate in %1").arg((int)info.score) : QString("%1").arg((m_sideToMove == 'w' ? info.score : -info.score), 0, 'f', 2);
    root->setText(0, evalTxt);

    const int maxLen = 40;
    QString pv = info.pvLine;
    QString disp = (pv.length() > maxLen) ? pv.left(maxLen) + " …" : pv;
    root->setText(1, disp);

    root->takeChildren();
    auto *child = new QTreeWidgetItem(root);
    child->setFirstColumnSpanned(true);
    child->setText(0, pv);
    root->setExpanded(true);
}

void EngineWidget::onInfoLine(const QString &line) {
    m_console->append(QStringLiteral("<< %1").arg(line));
}

void EngineWidget::onCmdSent(const QString &cmd) {
    m_console->append(QStringLiteral(">> %1").arg(cmd));
}
