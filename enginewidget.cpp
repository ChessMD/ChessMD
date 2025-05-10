/*
April 11, 2025: File Creation
*/

#include "enginewidget.h"
#include "chessposition.h"
#include "chessqsettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>

EngineWidget::EngineWidget(QWidget *parent)
    : QWidget(parent),
    m_engine(new UciEngine(this)),
    m_tree(new QTreeWidget(this)),
    m_multiPv(new QSpinBox(this)),
    m_buttonAnalyse(new QPushButton(tr("Analyse"), this)),
    m_buttonStop(new QPushButton(tr("Stop"), this)),
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
    m_buttonStop->setEnabled(false);

    auto *ctrl = new QHBoxLayout;
    ctrl->addWidget(new QLabel(tr("Lines:")));
    ctrl->addWidget(m_multiPv);
    ctrl->addWidget(m_buttonAnalyse);
    ctrl->addWidget(m_buttonStop);

    auto *lay = new QVBoxLayout(this);
    lay->addLayout(ctrl);
    lay->addWidget(m_tree);
    lay->addWidget(new QLabel(tr("UCI Log:")));
    lay->addWidget(m_console);

    // Engine start & signals
    ChessQSettings s; s.loadSettings();
    m_engine->startEngine(s.getEngineFile());

    connect(m_buttonAnalyse, &QPushButton::clicked, this, &EngineWidget::onAnalyseClicked);
    connect(m_buttonStop, &QPushButton::clicked, this, &EngineWidget::onStopClicked);

    connect(m_engine, &UciEngine::pvUpdate, this, &EngineWidget::onPvUpdate);
    connect(m_engine, &UciEngine::bestMove, this, &EngineWidget::onBestMove);
    connect(m_engine, &UciEngine::infoReceived, this, &EngineWidget::onInfoLine);
    connect(m_engine, &UciEngine::commandSent, this, &EngineWidget::onCmdSent);
}

void EngineWidget::setPosition(const QString &fen) {
    m_currentFen = fen;
    m_engine->setPosition(fen);
}

void EngineWidget::onMoveSelected(const QSharedPointer<NotationMove>& move) {
    if (!move.isNull() && move->m_position) {
        QString fen = move->m_position->positionToFEN();
        setPosition(fen);
        onAnalyseClicked();
    }
}

void EngineWidget::onAnalyseClicked() {
    if (m_currentFen.isEmpty()) return;

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

    m_buttonAnalyse->setEnabled(false);
    m_buttonStop->setEnabled(true);

    m_engine->startInfiniteSearch(n);
}

void EngineWidget::onStopClicked() {
    m_engine->stopSearch();
    m_buttonStop->setEnabled(false);
}

void EngineWidget::onPvUpdate(const PvInfo &info) {
    auto it = m_lineItems.find(info.multipv);
    if (it == m_lineItems.end()) return;
    QTreeWidgetItem *root = it.value();

    QString evalTxt = info.isMate ? tr("Mate in %1").arg((int)info.score) : QString("%1").arg(info.score, 0, 'f', 2);
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

void EngineWidget::onBestMove(const QString &move) {
    m_buttonAnalyse->setEnabled(true);
}

void EngineWidget::onInfoLine(const QString &line) {
    m_console->append(QStringLiteral("<< %1").arg(line));
}

void EngineWidget::onCmdSent(const QString &cmd) {
    m_console->append(QStringLiteral(">> %1").arg(cmd));
}
