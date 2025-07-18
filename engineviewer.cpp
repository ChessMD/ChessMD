/*
April 11, 2025: File Creation
*/

#include "engineviewer.h"
#include "chessposition.h"
#include "chessqsettings.h"

#include "QFile"
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QEvent>
#include <QFileDialog>
#include <QThread>
#include <QOperatingSystemVersion>
#include <QCoreApplication>

EngineWidget::EngineWidget(QWidget *parent)
    : QWidget(parent),
    m_engine(new UciEngine(this)),
    m_multiPv(3),
    m_console(new QTextEdit(this)),
    m_isHovering(false),
    m_ignoreHover(false)
{
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);

    m_evalButton = new QPushButton("0.00", this);
    m_evalButton->setEnabled(false);
    m_evalButton->setFlat(true);
    m_evalButton->setStyleSheet(R"(
        QPushButton {
            font-size: 24px;
            font-weight: bold;
            border: 1px solid #888;
            border-radius: 4px;
            padding: 8px 16px;
        }
    )");

    QString buttonStyle = R"(
        QToolButton {
            font-size: 24px;
            border: 1px solid #888;
            border-radius: 4px;
            padding: 8px 16px;
        }
        QToolButton:hover {
            background: #f0f0f0;
        }
    )";

    QAction* configAction = new QAction(QIcon(":/resource/img/engineupload.png"), tr("Configure engine"), this);
    connect(configAction, &QAction::triggered, this, &EngineWidget::onConfigEngineClicked);

    QToolButton* configBtn = new QToolButton(this);
    configBtn->setDefaultAction(configAction);
    configBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    configBtn->setAutoRaise(true);
    configBtn->setStyleSheet(buttonStyle);
    configBtn->setMinimumSize(m_evalButton->minimumSizeHint());
    configBtn->setIconSize(QSize(32, 32));

    QAction* debugAction = new QAction(QIcon(":/resource/img/enginedebug.png"), tr("Show/Hide UCI debug console"), this);
    connect(debugAction, &QAction::triggered, [this](){
        m_console->setVisible(!m_console->isVisible());
    });

    QToolButton* debugBtn = new QToolButton(this);
    debugBtn->setDefaultAction(debugAction);
    debugBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    debugBtn->setAutoRaise(true);
    debugBtn->setStyleSheet(buttonStyle);
    debugBtn->setMinimumSize(m_evalButton->minimumSizeHint());
    debugBtn->setIconSize(QSize(32, 32));

    auto* topBar = new QHBoxLayout;
    topBar->addWidget(configBtn);
    topBar->addStretch();
    topBar->addWidget(m_evalButton, 1);
    topBar->addStretch();
    topBar->addWidget(debugBtn);

    auto* linesTitle = new QLabel(tr("Engine Lines"), this);
    linesTitle->setStyleSheet("font-weight: bold;");
    auto* btnDec = new QPushButton("−", this);
    auto* btnInc = new QPushButton("+", this);
    btnDec->setFixedSize(20, 20);
    btnInc->setFixedSize(20, 20);
    connect(btnDec, &QPushButton::clicked, [this](){
        m_multiPv--;
        m_multiPv = qMax(1, m_multiPv);
        analysePosition();
    });
    connect(btnInc, &QPushButton::clicked, [this](){
        m_multiPv++;
        analysePosition();
    });

    auto* linesHeader = new QHBoxLayout;
    linesHeader->addStretch();
    linesHeader->addWidget(linesTitle);
    linesHeader->addStretch();
    linesHeader->addWidget(btnDec);
    linesHeader->addWidget(btnInc);

    btnDec->setFixedSize(20, 20);
    btnInc->setFixedSize(20, 20);

    auto* engineFrame = new QFrame(this);
    engineFrame->setFrameShape(QFrame::StyledPanel);
    auto* engineLayout = new QVBoxLayout(engineFrame);
    engineLayout->setContentsMargins(4,4,4,4);
    engineLayout->addLayout(linesHeader);

    m_container = new QWidget(this);
    m_containerLay = new QVBoxLayout(m_container);
    m_containerLay->setContentsMargins(0,0,0,0);
    m_containerLay->setSpacing(4);

    m_scroll = new QScrollArea(this);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setWidget(m_container);
    engineLayout->addWidget(m_scroll, 1);

    m_engineLabel = new QLabel(tr("No engine selected!"), this);
    m_engineLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_engineLabel->setStyleSheet("padding:4px; font-style:italic;");
    engineLayout->addWidget(m_engineLabel);

    m_console->setReadOnly(true);
    m_console->setFixedHeight(150);
    m_console->hide();

    auto* mainLay = new QVBoxLayout(this);
    mainLay->addLayout(topBar);
    mainLay->addWidget(engineFrame, 1);
    mainLay->addWidget(m_console);

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(200);
    connect(m_debounceTimer, &QTimer::timeout, this, &EngineWidget::doPendingAnalysis);

    ChessQSettings s; s.loadSettings();
    m_engine->startEngine(s.getEngineFile());

    connect(m_engine, &UciEngine::pvUpdate, this, &EngineWidget::onPvUpdate);
    connect(m_engine, &UciEngine::infoReceived, this, &EngineWidget::onInfoLine);
    connect(m_engine, &UciEngine::commandSent, this, &EngineWidget::onCmdSent);

    // doPendingAnalysis();
}

void EngineWidget::onConfigEngineClicked()
{
    QOperatingSystemVersion osVersion = QOperatingSystemVersion::current();
    QString binary;
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);
    if (dir.cd("engine")) {
        // path is "<parent_of_exe>/engine"
    } else {
        dir = QDir(exeDir);
    }

    if (osVersion.type() == QOperatingSystemVersion::Windows)
        binary = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), dir.absolutePath(), tr("(*.exe)"));
    else
        binary = QFileDialog::getOpenFileName(this, tr("Select a chess engine file"), dir.absolutePath(), tr("(*)"));
    if (binary.isEmpty()) return;

    ChessQSettings s;
    s.loadSettings();
    s.setEngineFile(binary);
    s.saveSettings();

    m_engine->quitEngine();
    m_engine->startEngine(binary);

    m_engineLabel->setText(tr("No engine selected!"));

    doPendingAnalysis();
}

void EngineWidget::onMoveSelected(const QSharedPointer<NotationMove>& move)
{
    if (!move.isNull() && move->m_position) {
        m_ignoreHover = true;
        m_isHovering = false;
        m_sideToMove = move->m_position->m_sideToMove;
        m_currentFen = move->m_position->positionToFEN();
        m_currentMove = move;
        m_debounceTimer->start();
    }
}

void EngineWidget::onEngineMoveClicked(QSharedPointer<NotationMove>& move)
{
    emit engineMoveClicked(move);

    // reset all engine lines
    QLayoutItem *child;
    while ((child = m_containerLay->takeAt(0)) != nullptr) {
        if (auto *w = child->widget()) {
            w->deleteLater();
        }
        delete child;
    }
    m_lineWidgets.clear();
    for (int i = 1; i <= m_multiPv; i++) {
        ChessPosition* dummyPos = new ChessPosition;
        auto temp = QSharedPointer<NotationMove>::create(QString(), *dummyPos);
        auto *lineW = new EngineLineWidget("...", "", temp, this);
        lineW->installEventFilter(this);
        m_containerLay->addWidget(lineW);
        m_lineWidgets[i] = lineW;
    }
    m_containerLay->addStretch();
}

void EngineWidget::doPendingAnalysis()
{
    m_engine->setPosition(m_currentFen);
    analysePosition();
}

void EngineWidget::analysePosition() {
    if (m_currentFen.isEmpty()) return;

    m_engine->stopSearch();
    m_engine->requestReady();
    m_console->clear();
    QLayoutItem *child;
    while ((child = m_containerLay->takeAt(0)) != nullptr) {
        if (auto *w = child->widget()) {
            w->deleteLater();
        }
        delete child;
    }
    m_lineWidgets.clear();

    for (int i = 1; i <= m_multiPv; i++) {
        ChessPosition* dummyPos = new ChessPosition;
        auto temp = QSharedPointer<NotationMove>::create(QString(), *dummyPos);
        auto *lineW = new EngineLineWidget("...", "", temp, this);
        lineW->installEventFilter(this);
        m_containerLay->addWidget(lineW);
        m_lineWidgets[i] = lineW;
    }
    m_containerLay->addStretch();
    m_engine->startInfiniteSearch(m_multiPv);
}

void EngineWidget::onPvUpdate(PvInfo &info) {
    // store the latest info for this line
    m_bufferedInfo[info.multipv] = info;

    // if hovering, bail out and let the engine run
    if (m_isHovering && !m_ignoreHover) {
        return;
    }

    auto it = m_lineWidgets.find(info.multipv);
    if (it == m_lineWidgets.end()) return;
    EngineLineWidget *lineW = it.value();

    info.positive = ((m_sideToMove == 'w' && info.score >= 0) || (m_sideToMove == 'b' && info.score < 0) ? true : false);
    info.score = abs(info.score);

    QSharedPointer<NotationMove> rootMove = parseEngineLine(info.pvLine, m_currentMove); // parse LAN into a notation tree
    QString evalTxt = (info.positive ? "" : "-") + (info.isMate ? tr("M%1").arg(info.score) : QString("%1").arg(info.score, 0, 'f', 2));

    if (!rootMove){
        return;
    }

    EngineLineWidget *newW = new EngineLineWidget(evalTxt, info.pvLine, rootMove, this);

    newW->installEventFilter(this);
    connect(newW, &EngineLineWidget::moveClicked, this, &EngineWidget::onEngineMoveClicked);
    connect(newW, &EngineLineWidget::moveHovered, this, &EngineWidget::moveHovered);

    int index = m_containerLay->indexOf(lineW);
    m_containerLay->insertWidget(index, newW);
    m_containerLay->removeWidget(lineW);
    lineW->deleteLater();
    m_lineWidgets[info.multipv] = newW;

    if (info.multipv == 1) {
        QString bigEval = evalTxt;
        m_evalButton->setText(bigEval);

        double evalScore = 0.0;
        if (!info.isMate) evalScore = (info.positive ? info.score : -info.score);
        else evalScore = (info.positive ? 4.0 : -4.0);
        m_currentMove->m_position->setEvalScore(qMax(4.0, qMin(-4.0, evalScore)));
        emit engineEvalScoreChanged(evalScore);

        QString bg = info.positive ? QStringLiteral("white") : QStringLiteral("#333");
        QString fg = info.positive ? QStringLiteral("black") : QStringLiteral("white");
        m_evalButton->setStyleSheet(QStringLiteral(R"(
            QPushButton {
                font-size: 24px;
                font-weight: bold;
                border: 1px solid #888;
                border-radius: 4px;
                padding: 8px 16px;
                background: %1;
                color: %2;
            }
        )").arg(bg, fg));
    }
}

void EngineWidget::flushBufferedInfo()
{
    // Apply the newest info for each multipv, in order
    for (auto it = m_bufferedInfo.begin(); it != m_bufferedInfo.end(); ++it) {
        onPvUpdate(it.value());
    }
    m_bufferedInfo.clear();
}

bool EngineWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (auto *line = qobject_cast<EngineLineWidget*>(watched)) {
        if (event->type() == QEvent::Enter) {
            m_isHovering = true;
            return false; // let EngineLineWidget also get the event
        }
        if (event->type() == QEvent::Leave) {
            m_isHovering = false;
            // now that we’ve left, flush the deepest info:
            flushBufferedInfo();
            return false;
        }
        if (event->type() == QEvent::MouseMove){
            m_ignoreHover = false;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void EngineWidget::onInfoLine(const QString &line)
{
    m_console->append(QStringLiteral("<< %1").arg(line));
    if (line.startsWith("id name ")) {
        // everything after "id name " is the engine's name
        QString name = line.mid(QStringLiteral("id name ").length());
        m_engineLabel->setText(tr("Engine: %1").arg(name));
    }
}

void EngineWidget::onCmdSent(const QString &cmd)
{
    m_console->append(QStringLiteral(">> %1").arg(cmd));
    m_console->viewport()->repaint(); // force the UI to catch up
}


