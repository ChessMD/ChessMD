#include "enginewidget.h"
#include "uciengine.h"
#include "chessposition.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

EngineWidget::EngineWidget(QWidget *parent)
    : QWidget(parent),
    m_engine(new UciEngine(this)),
    m_output(new QTextEdit(this)),
    m_depth(new QSpinBox(this)),
    m_button(new QPushButton(tr("Analyse"), this))
{
    m_output->setReadOnly(true);

    m_depth->setRange(1, 50);
    m_depth->setValue(15);

    connect(m_button, &QPushButton::clicked,
            this,      &EngineWidget::onAnalyseClicked);
    connect(m_engine, &UciEngine::infoReceived,
            this,      &EngineWidget::onEngineInfo);
    connect(m_engine, &UciEngine::bestMove,
            this,      &EngineWidget::onBestMove);

    auto *controls = new QHBoxLayout;
    controls->addWidget(new QLabel(tr("Depth:")));
    controls->addWidget(m_depth);
    controls->addWidget(m_button);

    auto *lay = new QVBoxLayout(this);
    lay->addLayout(controls);
    lay->addWidget(m_output);

    m_engine->startEngine("C:/Chess/stockfish-windows-x86-64-avx2/stockfish/stockfish-windows-x86-64-avx2.exe");
}

void EngineWidget::setPosition(const QString &fen) {
    m_currentFen = fen;
    m_engine->setPosition(fen);
}

void EngineWidget::onAnalyseClicked() {
    m_output->clear();
    m_engine->setPosition(m_currentFen);
    m_engine->goDepth(m_depth->value());
}


void EngineWidget::onEngineInfo(const QString &info) {
    if (!info.startsWith("info depth"))
        return;

    // Extract cp or mate score
    static const QRegularExpression reScore(R"(score (cp|mate) (-?\d+))");
    auto m = reScore.match(info);
    if (!m.hasMatch())
        return;

    QString type = m.captured(1);
    int val      = m.captured(2).toInt();

    if (type == "cp") {
        double cp = val / 100.0;
        m_output->setPlainText(QString("Eval: %1").arg(QString::number(cp, 'f', 2)));
    } else {
        m_output->setPlainText(QString("Mate in %1").arg(val));
    }
}


void EngineWidget::onBestMove(const QString &move) {
    // Append best move at top
    m_output->append(QString("Best: %1").arg(move));
}

void EngineWidget::onMoveSelected(const QSharedPointer<NotationMove>& move) {
    if (!move.isNull() && move->m_position) {
        m_output->clear();
        QString fen = move->m_position->positionToFEN();
        setPosition(fen);
        m_engine->goDepth(m_depth->value());
    }
}
